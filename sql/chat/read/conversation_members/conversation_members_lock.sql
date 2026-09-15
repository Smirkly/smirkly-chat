SELECT 1
FROM conversation_members
WHERE conversation_id = $1::uuid
    AND user_id = $2::uuid
  FOR KEY SHARE;
