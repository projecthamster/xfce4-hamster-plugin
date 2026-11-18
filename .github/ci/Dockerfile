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
        libwayland-bin \
        libwayland-dev \
        xvfb \
        sway \
        wayland-utils \
        xfce4-panel \
        xfwm4 \
        xfconf \
        xdotool \
        x11-utils \
        grim \
        dbus-daemon \
        libnss-wrapper \
    && rm -rf /var/lib/apt/lists/*

# Read by .github/workflows/ci.yml: this image generation contains every tool
# the live panel tests need, so the workflow can make them mandatory.
RUN install -d /usr/share/xfce4-hamster-plugin-ci \
    && touch /usr/share/xfce4-hamster-plugin-ci/live-panel-tools

# Runtime test tools: xvfb is the X11 server behind popup-x11 and popup-panel-x11,
# sway the headless Wayland compositor with wlr-layer-shell behind popup-wayland
# and popup-panel-wayland, and wayland-utils provides wayland-info, which probes
# the compositor globals. The live panel tests additionally start a real
# xfce4-panel in a throwaway prefix, so they need xfwm4, xfconf (for
# xfconf-query), dbus-daemon for a private session bus, xdotool and x11-utils to
# click and inspect windows, and grim to compare screenshots. libwayland-dev
# and libwayland-bin are build dependencies of the Wayland click helper: they
# provide the wayland-client headers and the wayland-scanner that generates its
# protocol glue.

# dbus-daemon resolves the current uid through the password database before it
# will start a session bus. The live panel tests start a private session bus for
# xfconf, and make docker-test runs as an arbitrary host uid, so the image
# cannot bake that uid in: the test supplies a synthetic passwd entry with
# libnss-wrapper when the uid is not in the image.
