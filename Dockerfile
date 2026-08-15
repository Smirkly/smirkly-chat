ARG USERVER_IMAGE=ghcr.io/userver-framework/ubuntu-24.04-userver:v3.0@sha256:3477357e5d6b874d69676b38d084dc153097c9d7a85f09fb5b059427dcfee990

FROM ${USERVER_IMAGE} AS builder

USER root
COPY . /src
WORKDIR /src

RUN cmake --preset release \
        -DSMIRKLY_CHAT_ENABLE_TESTSUITE=OFF \
        -DSMIRKLY_CHAT_ENABLE_TEST_CONTROL=OFF \
    && cmake --build --preset release --parallel --target smirkly-chat \
    && cmake --install build-release --component smirkly-chat \
        --prefix /opt/smirkly-chat

FROM ${USERVER_IMAGE} AS prod

USER root
RUN useradd --system --uid 10001 --home-dir /app --create-home smirkly-chat

WORKDIR /app
COPY --from=builder /opt/smirkly-chat/bin/smirkly-chat /app/bin/smirkly-chat
COPY --from=builder /opt/smirkly-chat/etc/smirkly-chat /app/configs

ENV USERVER_ENABLE_STACK_USAGE_MONITOR=0
EXPOSE 8080 8082

USER smirkly-chat
ENTRYPOINT ["/app/bin/smirkly-chat"]
CMD ["--config", "/app/configs/static_config.prod.yaml"]
