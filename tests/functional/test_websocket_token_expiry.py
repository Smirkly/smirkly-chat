import asyncio
import json
import time
import uuid

import pytest
import websockets
from websockets.frames import Frame, OP_CONT, OP_TEXT


@pytest.fixture
def expiring_identity(websocket_client, mockserver, jwt_keypair, access_token):
    _, _, jwks = jwt_keypair

    @mockserver.json_handler("/auth/v0/.well-known/jwks.json")
    def jwks_handler(_request):
        return jwks

    # Start the service before issuing a short-lived token.
    user_id = str(uuid.uuid4())
    expires_at = int(time.time()) + 5
    token = access_token(user_id, str(uuid.uuid4()), exp=expires_at)
    return token, user_id, expires_at


async def test_idle_connection_closes_when_token_expires(
    websocket_client, expiring_identity
):
    token, _, expires_at = expiring_identity

    async with websocket_client.get(
        "chat/v0/ws",
        extra_headers={"Authorization": f"Bearer {token}"},
        ping_interval=None,
    ) as websocket:
        assert json.loads(await websocket.recv())["type"] == "system.connected"
        await websocket.send(
            json.dumps(
                {"type": "ping", "request_id": "before-expiry", "payload": {}}
            )
        )
        assert json.loads(await websocket.recv()) == {
            "type": "pong",
            "request_id": "before-expiry",
            "payload": {},
        }

        with pytest.raises(websockets.exceptions.ConnectionClosedError) as error:
            await asyncio.wait_for(
                websocket.recv(), timeout=max(0, expires_at - time.time()) + 3
            )

        assert error.value.code == 1008
        assert time.time() >= expires_at


@pytest.mark.parametrize("stall_at", ["header", "payload", "continuation"])
@pytest.mark.parametrize(
    "complete_after_expiry", [False, True], ids=["stalled-frame", "completed-frame"]
)
async def test_partial_message_cannot_outlive_token(
    websocket_client, expiring_identity, pgsql, complete_after_expiry, stall_at
):
    token, user_id, expires_at = expiring_identity
    cursor = pgsql["chat"].cursor()
    cursor.execute("INSERT INTO conversations DEFAULT VALUES RETURNING id")
    conversation_id = cursor.fetchone()[0]
    cursor.execute(
        """
        INSERT INTO conversation_members (conversation_id, user_id)
        VALUES (%s, %s)
        """,
        (conversation_id, user_id),
    )
    client_message_id = str(uuid.uuid4())
    event = {
        "type": "message.send",
        "request_id": "across-expiry",
        "payload": {
            "conversation_id": conversation_id,
            "client_message_id": client_message_id,
            "text": "This message must not be persisted",
        },
    }
    payload = json.dumps(event).encode()
    frame = Frame(OP_TEXT, payload).serialize(mask=True)
    if stall_at == "header":
        prefix, remainder = frame[:1], frame[1:]
    elif stall_at == "payload":
        prefix, remainder = frame[:-1], frame[-1:]
    else:
        middle = len(payload) // 2
        prefix = Frame(OP_TEXT, payload[:middle], fin=False).serialize(mask=True)
        remainder = Frame(OP_CONT, payload[middle:]).serialize(mask=True)

    async with websocket_client.get(
        "chat/v0/ws",
        extra_headers={"Authorization": f"Bearer {token}"},
        compression=None,
        ping_interval=None,
    ) as websocket:
        assert json.loads(await websocket.recv())["type"] == "system.connected"
        assert time.time() < expires_at

        try:
            # Withhold the remaining bytes or continuation frame until after exp.
            websocket.transport.write(prefix)
            await asyncio.sleep(max(0, expires_at - time.time()) + 0.1)
            if complete_after_expiry and not websocket.closed:
                websocket.transport.write(remainder)

            await asyncio.wait_for(websocket.wait_closed(), timeout=3)
            assert websocket.close_code == 1008
        finally:
            # A failed deadline assertion must not leave the server waiting for data.
            websocket.transport.abort()
            await asyncio.wait_for(websocket.wait_closed(), timeout=3)

    cursor.execute(
        """
        SELECT COUNT(*) FROM messages
        WHERE author_id = %s AND client_message_id = %s
        """,
        (user_id, client_message_id),
    )
    assert cursor.fetchone()[0] == 0


async def test_control_frames_and_fragmented_message_before_expiry(
    websocket_client, expiring_identity
):
    token, _, _ = expiring_identity
    event = json.dumps(
        {"type": "ping", "request_id": "fragmented", "payload": {}}
    )

    async def fragments():
        middle = len(event) // 2
        yield event[:middle]
        await asyncio.sleep(0.05)
        yield event[middle:]

    async with websocket_client.get(
        "chat/v0/ws",
        extra_headers={"Authorization": f"Bearer {token}"},
        ping_interval=None,
    ) as websocket:
        assert json.loads(await websocket.recv())["type"] == "system.connected"
        pong_received = await websocket.ping(b"control-ping")
        await asyncio.wait_for(pong_received, timeout=3)

        await websocket.send(fragments())
        assert json.loads(await asyncio.wait_for(websocket.recv(), timeout=3)) == {
            "type": "pong",
            "request_id": "fragmented",
            "payload": {},
        }

        await websocket.close(code=1000)
        assert websocket.close_code == 1000
