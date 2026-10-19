CREATE TABLE conversations
(
    id         UUID PRIMARY KEY     DEFAULT gen_random_uuid(),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE conversation_members
(
    conversation_id UUID        NOT NULL
        REFERENCES conversations (id) ON DELETE CASCADE,
    user_id         UUID        NOT NULL,
    joined_at       TIMESTAMPTZ NOT NULL DEFAULT now(),

    PRIMARY KEY (conversation_id, user_id)
);

CREATE TABLE messages
(
    id                UUID PRIMARY KEY     DEFAULT gen_random_uuid(),
    client_message_id UUID        NOT NULL,
    conversation_id   UUID        NOT NULL
        REFERENCES conversations (id) ON DELETE CASCADE,
    author_id         UUID        NOT NULL,
    text              TEXT        NOT NULL
        CHECK (octet_length(text) BETWEEN 1 AND 4096),
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now(),

    UNIQUE (author_id, client_message_id)
);

CREATE INDEX conversation_members_user_idx
    ON conversation_members (user_id, conversation_id);

CREATE INDEX messages_history_idx
    ON messages (conversation_id, created_at DESC, id DESC);