SELECT id,
       client_message_id,
       conversation_id,
       author_id,
       text,
       created_at
FROM messages
WHERE messages.author_id = $1::uuid
  AND messages.client_message_id = $2::uuid
LIMIT 1;
