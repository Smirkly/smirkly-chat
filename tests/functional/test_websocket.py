import json
import time
import uuid

import pytest
import websockets


async def test_authenticated_transport(
    websocket_client, mockserver, jwt_keypair, access_token
):
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

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        connected = json.loads(await websocket.recv())
        assert connected == {
            "type": "system.connected",
            "payload": {"user_id": user_id, "session_id": session_id},
        }

        await websocket.send(json.dumps({"type": "ping"}))
        assert json.loads(await websocket.recv()) == {"type": "pong"}

        await websocket.send(json.dumps({"type": "message.send"}))
        response = json.loads(await websocket.recv())
        assert response["type"] == "error"
        assert response["payload"]["code"] == "chat.not_implemented"


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
