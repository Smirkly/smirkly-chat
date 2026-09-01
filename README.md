# smirkly-chat

Chat service for Smirkly built with C++20 and userver.

The service currently implements access-token verification and an authenticated
WebSocket protocol. Message storage and delivery are still in progress.

## Development

Open `.devcontainer/devcontainer.json` with CLion Remote Development.

Run `smirkly-auth` on host port `8080` before connecting to the WebSocket
endpoint. The chat service uses its public keys to verify access tokens.

Build and run the tests inside the devcontainer:

```bash
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug
```

Run the service:

```bash
./build-debug/smirkly-chat --config ./configs/static_config.yaml
```

Local ports:

| Service | Port |
| --- | ---: |
| Chat API | `8081` |
| Monitor API | `8082` |
| PostgreSQL | `5433` |
| Redis | `6380` |

## WebSocket protocol

WebSocket endpoint:

```text
GET /chat/v0/ws
```

The handshake requires an access token:

```http
Authorization: Bearer <access-token>
```

Client events use a common JSON envelope:

```json
{
  "type": "ping",
  "request_id": "request-123",
  "payload": {}
}
```

The server returns the same `request_id` in its response:

```json
{
  "type": "pong",
  "request_id": "request-123",
  "payload": {}
}
```

Only `ping` is supported at the moment. Other valid event types return
`chat.unsupported_event`.

## Kafka

Kafka is disabled by default. Start the local broker with the Docker Compose
profile when it is needed:

```bash
docker compose -f docker-compose.devcontainer.yml --profile kafka up -d
```
