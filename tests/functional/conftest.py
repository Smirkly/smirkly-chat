import base64
import json
import os

import jwt
import pytest
from cryptography.hazmat.primitives.asymmetric import rsa
from testsuite.databases.pgsql import discover

os.environ["TESTSUITE_REDIS_HOSTNAME"] = "localhost"

pytest_plugins = [
    "pytest_userver.plugins.core",
    "pytest_userver.plugins.service",
    "pytest_userver.plugins.postgresql",
    "pytest_userver.plugins.redis",
]

USERVER_CONFIG_HOOKS = ["userver_config_auth_jwks"]


@pytest.fixture(scope="session")
def userver_config_auth_jwks(mockserver_info):
    def patch_config(config_yaml, _config_vars):
        components = config_yaml["components_manager"]["components"]
        components["chat-jwks-verifier"]["jwks-url"] = mockserver_info.url(
            "/auth/v0/.well-known/jwks.json"
        )

    return patch_config


@pytest.fixture(scope="session")
def pgsql_local(service_source_dir, pgsql_local_create):
    schemas = discover.find_schemas(
        None, [service_source_dir / "tests/functional/schemas"]
    )
    return pgsql_local_create(list(schemas.values()))


@pytest.fixture(scope="session")
def service_env(redis_standalone_node):
    secdist_config = {
        "kafka_settings": {},
        "redis_settings": {
            "chat-redis": {
                "password": "",
                "database_index": 0,
                "sentinels": [redis_standalone_node],
                "shards": [{"name": "test_standalone_master0"}],
            }
        }
    }
    return {"SECDIST_CONFIG": json.dumps(secdist_config)}


@pytest.fixture
def extra_client_deps(redis_standalone_store):
    pass


def _base64url_uint(value):
    size = (value.bit_length() + 7) // 8
    raw = value.to_bytes(size, "big")
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode()


@pytest.fixture(scope="session")
def jwt_keypair():
    private_key = rsa.generate_private_key(
        public_exponent=65537, key_size=2048
    )
    public_numbers = private_key.public_key().public_numbers()
    key_id = "smirkly-auth-tests-rs256"
    jwks = {
        "keys": [
            {
                "kty": "RSA",
                "use": "sig",
                "alg": "RS256",
                "kid": key_id,
                "n": _base64url_uint(public_numbers.n),
                "e": _base64url_uint(public_numbers.e),
            }
        ]
    }
    return private_key, key_id, jwks


@pytest.fixture
def access_token(jwt_keypair):
    private_key, key_id, _ = jwt_keypair

    def make_token(user_id, session_id, **claims):
        payload = {
            "sub": user_id,
            "sid": session_id,
            "type": "access",
            "iss": "smirkly-auth",
            "aud": "smirkly-api",
            **claims,
        }
        return jwt.encode(
            payload,
            private_key,
            algorithm="RS256",
            headers={"kid": key_id},
        )

    return make_token
