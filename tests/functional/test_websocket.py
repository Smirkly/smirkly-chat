import json
import time
import uuid

import pytest
import websockets


@pytest.fixture
def authenticated_identity(mockserver, jwt_keypair, access_token):
    _, _, jwks = jwt_keypair

    @mockserver.json_handler("/auth/v0/.well-known/jwks.json")
    def jwks_handler(_request):
        return jwks

    user_id = str(uuid.uuid4())
    session_id = str(uuid.uuid4())
    token = access_token(
        user_id,
        session_id,
        exp=int(time.time()) + 300,
    )
    return token, user_id, session_id


async def test_authenticated_transport(websocket_client, authenticated_identity):
    token, user_id, session_id = authenticated_identity

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        assert json.loads(await websocket.recv()) == {
            "type": "system.connected",
            "payload": {"user_id": user_id, "session_id": session_id},
        }


async def test_ping_preserves_request_id(websocket_client, authenticated_identity):
    token, _, _ = authenticated_identity
    request_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        await websocket.send(
            json.dumps(
                {
                    "type": "ping",
                    "request_id": request_id,
                    "payload": {},
                }
            )
        )

        assert json.loads(await websocket.recv()) == {
            "type": "pong",
            "request_id": request_id,
            "payload": {},
        }


async def test_unsupported_event_preserves_request_id(
    websocket_client, authenticated_identity
):
    token, _, _ = authenticated_identity
    request_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        await websocket.send(
            json.dumps(
                {
                    "type": "message.send",
                    "request_id": request_id,
                    "payload": {},
                }
            )
        )

        assert json.loads(await websocket.recv()) == {
            "type": "error",
            "request_id": request_id,
            "payload": {
                "code": "chat.unsupported_event",
                "message": "event type is not supported",
            },
        }


async def test_malformed_event_has_no_request_id(
    websocket_client, authenticated_identity
):
    token, _, _ = authenticated_identity

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        await websocket.send('{"type":')

        response = json.loads(await websocket.recv())
        assert response == {
            "type": "error",
            "payload": {
                "code": "chat.invalid_event",
                "message": "invalid event envelope",
            },
        }
        assert "request_id" not in response


async def test_binary_frame_is_rejected(websocket_client, authenticated_identity):
    token, _, _ = authenticated_identity

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        await websocket.send(b"binary")

        response = json.loads(await websocket.recv())
        assert response == {
            "type": "error",
            "payload": {
                "code": "chat.unsupported_frame",
                "message": "only text frames are supported",
            },
        }
        assert "request_id" not in response


async def test_rejects_missing_token(websocket_client):
    with pytest.raises(websockets.exceptions.InvalidStatusCode) as error:
        async with websocket_client.get("chat/v0/ws"):
            pass
    assert error.value.status_code == 401


async def test_rejects_expired_token(
    websocket_client, mockserver, jwt_keypair, access_token
):
    _, _, jwks = jwt_keypair

    @mockserver.json_handler("/auth/v0/.well-known/jwks.json")
    def jwks_handler(_request):
        return jwks

    token = access_token(
        str(uuid.uuid4()),
        str(uuid.uuid4()),
        exp=int(time.time()) - 1,
    )
    with pytest.raises(websockets.exceptions.InvalidStatusCode) as error:
        async with websocket_client.get(
            "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
        ):
            pass
    assert error.value.status_code == 401
