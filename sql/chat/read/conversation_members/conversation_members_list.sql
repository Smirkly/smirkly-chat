SELECT user_id
FROM conversation_members
WHERE conversation_id = $1::uuid
ORDER BY user_id
FOR KEY SHARE;
