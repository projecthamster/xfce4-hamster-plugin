FROM debian:stable

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        pkg-config \
        gettext \
        intltool \
        libglib2.0-dev-bin \
        libxfce4panel-2.0-dev \
        libxfce4ui-2-dev \
        libxfconf-0-dev \
        libxfce4util-dev \
    && rm -rf /var/lib/apt/lists/*
