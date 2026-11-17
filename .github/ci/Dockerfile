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
        xvfb \
        sway \
        wayland-utils \
    && rm -rf /var/lib/apt/lists/*

# runtime test tools: xvfb is the X11 server behind popup-x11, sway is the
# headless Wayland compositor with wlr-layer-shell behind popup-wayland, and
# wayland-utils provides wayland-info, which probes the compositor globals.
