# smirkly-chat

Infrastructure-first scaffold for the Smirkly chat service. It provides a
working userver v3.0 runtime, mandatory PostgreSQL and Redis connections,
Smirkly access-token verification through JWKS, and an authenticated WebSocket
transport. Chat domain behavior is intentionally not implemented yet.

## Current boundary

Implemented:

- C++20 and explicit CMake targets;
- debug ASan/UBSan and release presets;
- PostgreSQL and Redis components;
- optional Kafka producer component and local broker profile;
- RS256 access-token verification against `smirkly-auth` JWKS;
- authenticated `GET /chat/v0/ws` transport;
- `system.connected`, `ping`, `pong`, and structured error events;
- liveness, readiness, metrics, functional tests, and CI;
- native ARM64 non-root devcontainer.

Not implemented:

- chat, member, or message models;
- domain database tables and migrations;
- message persistence or delivery;
- Redis Pub/Sub fan-out;
- Kafka business events or an outbox;
- groups, attachments, presence, receipts, or editing.

## Development

Open `.devcontainer/devcontainer.json` through CLion Remote Development. The
workspace listens on host port `8081`, PostgreSQL on `5433`, Redis on `6380`,
and the monitor listener on `8082`.

Inside the container:

```bash
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug
./build-debug/smirkly-chat --config ./configs/static_config.yaml
```

Run `smirkly-auth` on host port `8080`. The chat service lazily loads signing
keys from:

```text
http://host.docker.internal:8080/auth/v0/.well-known/jwks.json
```

The WebSocket handshake requires an `Authorization: Bearer <access-token>`
header. A successful connection receives:

```json
{
  "type": "system.connected",
  "payload": {
    "user_id": "...",
    "session_id": "..."
  }
}
```

Only the application-level `{"type":"ping"}` event is accepted for now.

## Kafka

Kafka is compiled into the service but disabled with `kafka-enabled: false`.
Start the local broker only when designing the outbox integration:

```bash
docker compose -f docker-compose.devcontainer.yml --profile kafka up -d
```

Do not publish a domain event in the same code path as a PostgreSQL write.
Introduce a transactional outbox first, then publish versioned events such as
`chat.message.created.v1`.

## Next implementation order

1. Write the WebSocket event envelope contract.
2. Define the first chat use case and its invariants in tests.
3. Add only the PostgreSQL tables required by that use case.
4. Implement a repository adapter and transaction boundary.
5. Connect the use case to the transport.
6. Add Redis fan-out when more than one service instance is supported.
7. Add an outbox and Kafka events for other services.
