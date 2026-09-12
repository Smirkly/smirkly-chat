import uuid

import psycopg2.errors
import pytest


def _uuid():
    return str(uuid.uuid4())


def _create_conversation(cursor):
    cursor.execute(
        "INSERT INTO conversations DEFAULT VALUES RETURNING id, created_at"
    )
    return cursor.fetchone()


def _insert_message(
    cursor,
    *,
    conversation_id,
    author_id,
    client_message_id,
    text,
):
    cursor.execute(
        """
        INSERT INTO messages (
            conversation_id,
            author_id,
            client_message_id,
            text
        )
        VALUES (%s, %s, %s, %s)
        RETURNING id, created_at
        """,
        (conversation_id, author_id, client_message_id, text),
    )
    return cursor.fetchone()


def test_conversation_generates_id_and_created_at(pgsql):
    cursor = pgsql["chat"].cursor()

    conversation_id, created_at = _create_conversation(cursor)

    assert str(uuid.UUID(conversation_id)) == conversation_id
    assert created_at is not None


def test_user_can_join_multiple_conversations(pgsql):
    cursor = pgsql["chat"].cursor()
    first_conversation_id, _ = _create_conversation(cursor)
    second_conversation_id, _ = _create_conversation(cursor)
    user_id = _uuid()

    cursor.execute(
        """
        INSERT INTO conversation_members (conversation_id, user_id)
        VALUES (%s, %s), (%s, %s)
        """,
        (
            first_conversation_id,
            user_id,
            second_conversation_id,
            user_id,
        ),
    )

    cursor.execute(
        "SELECT COUNT(*) FROM conversation_members WHERE user_id = %s",
        (user_id,),
    )
    assert cursor.fetchone()[0] == 2


def test_rejects_duplicate_conversation_membership(pgsql):
    cursor = pgsql["chat"].cursor()
    conversation_id, _ = _create_conversation(cursor)
    user_id = _uuid()
    cursor.execute(
        """
        INSERT INTO conversation_members (conversation_id, user_id)
        VALUES (%s, %s)
        """,
        (conversation_id, user_id),
    )

    with pytest.raises(psycopg2.errors.UniqueViolation):
        cursor.execute(
            """
            INSERT INTO conversation_members (conversation_id, user_id)
            VALUES (%s, %s)
            """,
            (conversation_id, user_id),
        )


def test_membership_requires_existing_conversation(pgsql):
    cursor = pgsql["chat"].cursor()

    with pytest.raises(psycopg2.errors.ForeignKeyViolation):
        cursor.execute(
            """
            INSERT INTO conversation_members (conversation_id, user_id)
            VALUES (%s, %s)
            """,
            (_uuid(), _uuid()),
        )


def test_message_generates_id_and_created_at(pgsql):
    cursor = pgsql["chat"].cursor()
    conversation_id, _ = _create_conversation(cursor)

    message_id, created_at = _insert_message(
        cursor,
        conversation_id=conversation_id,
        author_id=_uuid(),
        client_message_id=_uuid(),
        text="Hello, Smirkly!",
    )

    assert str(uuid.UUID(message_id)) == message_id
    assert created_at is not None


def test_message_requires_existing_conversation(pgsql):
    cursor = pgsql["chat"].cursor()

    with pytest.raises(psycopg2.errors.ForeignKeyViolation):
        _insert_message(
            cursor,
            conversation_id=_uuid(),
            author_id=_uuid(),
            client_message_id=_uuid(),
            text="Hello",
        )


def test_message_text_is_limited_by_utf8_byte_length(pgsql):
    cursor = pgsql["chat"].cursor()
    conversation_id, _ = _create_conversation(cursor)
    accepted_text = "я" * 2048
    rejected_text = "я" * 2049

    _insert_message(
        cursor,
        conversation_id=conversation_id,
        author_id=_uuid(),
        client_message_id=_uuid(),
        text=accepted_text,
    )

    with pytest.raises(psycopg2.errors.CheckViolation):
        _insert_message(
            cursor,
            conversation_id=conversation_id,
            author_id=_uuid(),
            client_message_id=_uuid(),
            text=rejected_text,
        )


def test_rejects_empty_message_text(pgsql):
    cursor = pgsql["chat"].cursor()
    conversation_id, _ = _create_conversation(cursor)

    with pytest.raises(psycopg2.errors.CheckViolation):
        _insert_message(
            cursor,
            conversation_id=conversation_id,
            author_id=_uuid(),
            client_message_id=_uuid(),
            text="",
        )


def test_message_idempotency_is_scoped_by_author(pgsql):
    cursor = pgsql["chat"].cursor()
    conversation_id, _ = _create_conversation(cursor)
    first_author_id = _uuid()
    second_author_id = _uuid()
    client_message_id = _uuid()

    _insert_message(
        cursor,
        conversation_id=conversation_id,
        author_id=first_author_id,
        client_message_id=client_message_id,
        text="first",
    )

    with pytest.raises(psycopg2.errors.UniqueViolation):
        _insert_message(
            cursor,
            conversation_id=conversation_id,
            author_id=first_author_id,
            client_message_id=client_message_id,
            text="duplicate",
        )

    _insert_message(
        cursor,
        conversation_id=conversation_id,
        author_id=second_author_id,
        client_message_id=client_message_id,
        text="same client ID, different author",
    )


def test_deleting_conversation_cascades_related_rows(pgsql):
    cursor = pgsql["chat"].cursor()
    conversation_id, _ = _create_conversation(cursor)
    user_id = _uuid()
    cursor.execute(
        """
        INSERT INTO conversation_members (conversation_id, user_id)
        VALUES (%s, %s)
        """,
        (conversation_id, user_id),
    )
    _insert_message(
        cursor,
        conversation_id=conversation_id,
        author_id=user_id,
        client_message_id=_uuid(),
        text="Hello",
    )

    cursor.execute(
        "DELETE FROM conversations WHERE id = %s", (conversation_id,)
    )

    cursor.execute(
        """
        SELECT
            (SELECT COUNT(*) FROM conversation_members),
            (SELECT COUNT(*) FROM messages)
        """
    )
    assert cursor.fetchone() == (0, 0)
