ARG DEV_BASE_IMAGE=ubuntu:24.04@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90

FROM ${DEV_BASE_IMAGE} AS dev

ARG DEBIAN_FRONTEND=noninteractive

COPY docker/userver-v3.0-ubuntu-24.04-packages.txt /tmp/userver-packages.txt

RUN apt-get update \
    && xargs --arg-file=/tmp/userver-packages.txt \
        apt-get install --no-install-recommends --yes \
    && apt-get install --no-install-recommends --yes \
        ca-certificates \
        curl \
        libfreetype6 \
        libxi6 \
        libxext6 \
        libxrender1 \
        libxtst6 \
        locales \
        postgresql-16 \
        procps \
        python3-pip \
        sudo \
        unzip \
    && locale-gen en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/* /tmp/userver-packages.txt \
    && usermod --login developer --home /home/developer --move-home ubuntu \
    && groupmod --new-name developer ubuntu \
    && rm -f /etc/sudoers.d/ubuntu \
    && printf 'developer ALL=(ALL) NOPASSWD:ALL\n' > /etc/sudoers.d/developer \
    && chmod 0440 /etc/sudoers.d/developer \
    && install -d -o developer -g developer \
        /workspace/build-debug \
        /workspace/build-release \
        /workspace/.ccache

RUN apt-get update \
    && apt-get install --no-install-recommends --yes redis-server \
    && rm -rf /var/lib/apt/lists/* \
    && install -d -o developer -g developer \
        /.jbdevcontainer \
        /.jbdevcontainer/config \
        /.jbdevcontainer/data

WORKDIR /workspace

ENV HOME=/home/developer \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    CCACHE_DIR=/workspace/.ccache \
    CPM_SOURCE_CACHE=/workspace/.ccache/cpm \
    USERVER_ENABLE_STACK_USAGE_MONITOR=0

USER developer

CMD ["sleep", "infinity"]
