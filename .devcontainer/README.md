# CLion devcontainer

From the CLion welcome screen choose **Remote Development**, then
**Create Dev Container**, and select `.devcontainer/devcontainer.json`.

The CLion frontend runs on macOS. The backend, compiler, debugger, CMake,
PostgreSQL, Redis, and tests run in Linux as the `developer` user. CLion should
use the `debug` CMake preset. Run target `smirkly-chat` with arguments
`--config ./configs/static_config.yaml` and working directory `/workspace`.

Kafka is excluded from normal startup. Enable its Compose profile only when a
business event and transactional outbox have been designed.
