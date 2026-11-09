# Real-time message delivery

This document defines the first delivery milestone for `smirkly-chat`.
It is an implementation contract, not a claim that fan-out is already available.

## Current behavior

- Authenticated clients connect to `GET /chat/v0/ws`.
- The server accepts `ping` and `message.send`.
- `message.send` checks conversation membership and persists the message in PostgreSQL.
- The sender receives `message.created`.
- The server does not yet push the new message to other connected members.
- Message history retrieval is not implemented yet.

## First delivery milestone

Implement in-process delivery for a single Chat service instance while keeping
PostgreSQL as the source of truth.

### Send flow

1. Decode and validate `message.send`.
2. Verify the authenticated user is a member of the conversation.
3. Persist the message and commit the transaction.
4. Return `message.created` to the sender, preserving its `request_id`.
5. Publish `message.new` to every other active connection belonging to a
   member of the conversation.
6. Do not send the event back to the originating connection; the sender already
   receives the request-correlated `message.created` response.

The event payload for `message.new` uses the same message fields as
`message.created`: `message_id`, `client_message_id`, `conversation_id`,
`author_id`, `text`, and `created_at`. Server-initiated events omit
`request_id`.

### Connection lifecycle

- Register a connection only after the authenticated WebSocket handshake.
- Associate it with the authenticated user ID and a unique connection ID.
- Support multiple simultaneous connections for one user.
- Remove the connection on every exit path, including cancellation and errors.
- Never retain a raw WebSocket connection reference after its handler exits.
- Serialize writes per connection; only the connection-owning handler should
  write to its socket. Other tasks should enqueue outbound events for that
  handler rather than calling `SendText` concurrently.

### Authorization and reliability

- Resolve recipients from the conversation membership data, not from client
  supplied recipient IDs.
- A failed push must not roll back an already committed message.
- Offline users receive messages later through history synchronization.
- A reconnecting client must treat PostgreSQL history as authoritative.
- Do not claim durable delivery or read receipts in this milestone.
- Keep the delivery interface independent of the transport so Redis Pub/Sub
  can be added later for multi-instance fan-out.

## Acceptance tests

1. Two authenticated users are members of the same conversation.
2. Both connect to the WebSocket endpoint.
3. User A sends a message and receives `message.created`.
4. User B receives exactly one `message.new` with the persisted message ID
   and full payload, without sending a request.
5. A connection not belonging to the conversation receives no event.
6. Two connections owned by user B both receive the event.
7. A disconnected user receives no live event, but the message remains stored.
8. A retry with the same `client_message_id` does not create a duplicate and
   does not fan out a second new-message event.
9. Disconnecting a socket removes it from the connection registry.
10. Existing ping, authentication, persistence, and idempotency tests continue
    to pass.

## Follow-up work

After this milestone, implement cursor-based history retrieval. Only after the
single-instance delivery path is correct should cross-instance fan-out via
Redis Pub/Sub be introduced.
