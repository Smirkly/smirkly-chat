import json
import time
import uuid
from datetime import datetime

import pytest
import websockets


def _create_conversation(pgsql, user_id=None):
    connection = pgsql["chat"]
    cursor = connection.cursor()
    cursor.execute("INSERT INTO conversations DEFAULT VALUES RETURNING id")
    conversation_id = cursor.fetchone()[0]
    if user_id is not None:
        cursor.execute(
            """
            INSERT INTO conversation_members (conversation_id, user_id)
            VALUES (%s, %s)
            """,
            (conversation_id, user_id),
        )
    return conversation_id


async def _send_message(
    websocket, *, request_id, conversation_id, client_message_id, text
):
    await websocket.send(
        json.dumps(
            {
                "type": "message.send",
                "request_id": request_id,
                "payload": {
                    "conversation_id": conversation_id,
                    "client_message_id": client_message_id,
                    "text": text,
                },
            }
        )
    )
    return json.loads(await websocket.recv())


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
                    "type": "future.unknown",
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


async def test_message_send_creates_and_persists_message(
    websocket_client, authenticated_identity, pgsql
):
    token, user_id, _ = authenticated_identity
    conversation_id = _create_conversation(pgsql, user_id)
    client_message_id = str(uuid.uuid4())
    request_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        response = await _send_message(
            websocket,
            request_id=request_id,
            conversation_id=conversation_id,
            client_message_id=client_message_id,
            text="Hello, Smirkly!",
        )

    assert response["type"] == "message.created"
    assert response["request_id"] == request_id
    assert response["payload"] == {
        "message_id": response["payload"]["message_id"],
        "client_message_id": client_message_id,
        "conversation_id": conversation_id,
        "author_id": user_id,
        "text": "Hello, Smirkly!",
        "created_at": response["payload"]["created_at"],
    }
    uuid.UUID(response["payload"]["message_id"])
    assert response["payload"]["created_at"].endswith("+00:00")

    cursor = pgsql["chat"].cursor()
    cursor.execute(
        """
        SELECT id, client_message_id, conversation_id, author_id, text,
               created_at
        FROM messages
        WHERE id = %s
        """,
        (response["payload"]["message_id"],),
    )
    row = cursor.fetchone()
    assert row is not None
    assert row[:5] == (
        response["payload"]["message_id"],
        client_message_id,
        conversation_id,
        user_id,
        "Hello, Smirkly!",
    )
    assert row[5] == datetime.fromisoformat(response["payload"]["created_at"])


async def test_message_send_exact_retry_returns_original_message(
    websocket_client, authenticated_identity, pgsql
):
    token, user_id, _ = authenticated_identity
    conversation_id = _create_conversation(pgsql, user_id)
    client_message_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        first = await _send_message(
            websocket,
            request_id="send-attempt-1",
            conversation_id=conversation_id,
            client_message_id=client_message_id,
            text="Idempotent message",
        )
        repeated = await _send_message(
            websocket,
            request_id="send-attempt-2",
            conversation_id=conversation_id,
            client_message_id=client_message_id,
            text="Idempotent message",
        )

    assert first["type"] == "message.created"
    assert repeated["type"] == "message.created"
    assert first["request_id"] == "send-attempt-1"
    assert repeated["request_id"] == "send-attempt-2"
    assert repeated["payload"] == first["payload"]

    cursor = pgsql["chat"].cursor()
    cursor.execute(
        """
        SELECT COUNT(*)
        FROM messages
        WHERE author_id = %s AND client_message_id = %s
        """,
        (user_id, client_message_id),
    )
    assert cursor.fetchone()[0] == 1


async def test_message_send_rejects_idempotency_conflict(
    websocket_client, authenticated_identity, pgsql
):
    token, user_id, _ = authenticated_identity
    conversation_id = _create_conversation(pgsql, user_id)
    client_message_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        created = await _send_message(
            websocket,
            request_id="original",
            conversation_id=conversation_id,
            client_message_id=client_message_id,
            text="Original text",
        )
        conflict = await _send_message(
            websocket,
            request_id="conflict",
            conversation_id=conversation_id,
            client_message_id=client_message_id,
            text="Changed text",
        )

    assert created["type"] == "message.created"
    assert conflict["type"] == "error"
    assert conflict["request_id"] == "conflict"
    assert conflict["payload"]["code"] == "chat.idempotency_conflict"

    cursor = pgsql["chat"].cursor()
    cursor.execute(
        """
        SELECT text, COUNT(*) OVER ()
        FROM messages
        WHERE author_id = %s AND client_message_id = %s
        """,
        (user_id, client_message_id),
    )
    assert cursor.fetchone() == ("Original text", 1)


async def test_message_send_rejects_retry_in_another_conversation(
    websocket_client, authenticated_identity, pgsql
):
    token, user_id, _ = authenticated_identity
    first_conversation_id = _create_conversation(pgsql, user_id)
    second_conversation_id = _create_conversation(pgsql, user_id)
    client_message_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        await _send_message(
            websocket,
            request_id="original",
            conversation_id=first_conversation_id,
            client_message_id=client_message_id,
            text="Original text",
        )
        conflict = await _send_message(
            websocket,
            request_id="conflict",
            conversation_id=second_conversation_id,
            client_message_id=client_message_id,
            text="Original text",
        )

    assert conflict["type"] == "error"
    assert conflict["request_id"] == "conflict"
    assert conflict["payload"]["code"] == "chat.idempotency_conflict"


async def test_message_send_denies_non_member(
    websocket_client, authenticated_identity, pgsql
):
    token, user_id, _ = authenticated_identity
    conversation_id = _create_conversation(pgsql)
    client_message_id = str(uuid.uuid4())

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        response = await _send_message(
            websocket,
            request_id="access-denied",
            conversation_id=conversation_id,
            client_message_id=client_message_id,
            text="Must not be stored",
        )

    assert response["type"] == "error"
    assert response["request_id"] == "access-denied"
    assert response["payload"]["code"] == "chat.conversation_access_denied"

    cursor = pgsql["chat"].cursor()
    cursor.execute(
        """
        SELECT COUNT(*)
        FROM messages
        WHERE author_id = %s AND client_message_id = %s
        """,
        (user_id, client_message_id),
    )
    assert cursor.fetchone()[0] == 0


@pytest.mark.parametrize(
    ("text", "expected_code"),
    [
        ("", "chat.invalid_payload"),
        ("a" * 4097, "chat.message_too_long"),
    ],
    ids=["empty", "too-long"],
)
async def test_message_send_validates_text(
    websocket_client, authenticated_identity, pgsql, text, expected_code
):
    token, user_id, _ = authenticated_identity
    conversation_id = _create_conversation(pgsql, user_id)

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        response = await _send_message(
            websocket,
            request_id="invalid-text",
            conversation_id=conversation_id,
            client_message_id=str(uuid.uuid4()),
            text=text,
        )

    assert response["type"] == "error"
    assert response["request_id"] == "invalid-text"
    assert response["payload"]["code"] == expected_code


async def test_message_send_rejects_invalid_payload(
    websocket_client, authenticated_identity
):
    token, _, _ = authenticated_identity

    async with websocket_client.get(
        "chat/v0/ws", extra_headers={"Authorization": f"Bearer {token}"}
    ) as websocket:
        await websocket.recv()
        await websocket.send(
            json.dumps(
                {
                    "type": "message.send",
                    "request_id": "invalid-payload",
                    "payload": {
                        "conversation_id": "not-a-uuid",
                        "client_message_id": str(uuid.uuid4()),
                        "text": "Hello",
                    },
                }
            )
        )
        response = json.loads(await websocket.recv())

    assert response["type"] == "error"
    assert response["request_id"] == "invalid-payload"
    assert response["payload"]["code"] == "chat.invalid_payload"


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
