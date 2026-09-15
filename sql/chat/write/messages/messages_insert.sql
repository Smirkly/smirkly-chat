INSERT INTO messages
    (client_message_id, conversation_id, author_id, text)
VALUES ($1::uuid, $2::uuid, $3::uuid, $4::text) ON CONFLICT (author_id, client_message_id)
DO NOTHING
  RETURNING
    id,
    client_message_id,
    conversation_id,
    author_id,
    text,
    created_at
