FROM ghcr.io/cirruscontainers/macos/sequoia:latest

COPY devcontainer/setup.sh /usr/local/bin/layer-cut-setup
RUN chmod +x /usr/local/bin/layer-cut-setup \
    && /usr/local/bin/layer-cut-setup
