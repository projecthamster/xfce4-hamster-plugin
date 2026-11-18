## About
This is a recreation of the gnome-shell hamster extension as a xfce4 panel plugin.
See also the hamster project: <https://github.com/projecthamster/hamster>

Dependencies: `xfce4-panel xfconf hamster-applet|hamster-time-tracker`

Build dependencies[debian, ubuntu]: `libxfce4ui-2-dev libxfce4panel-2.0-dev
libxfconf-0-dev libxfce4util-dev`

Common: `build-essential cmake intltool gettext`, and `gdbus-codegen`
(from `libglib2.0-dev-bin`, pulled in by `libglib2.0-dev`). `intltool` is kept for now; its removal is deferred.

Tested with Xfce 4.18, 4.20 and 4.22 (4.22 is built against
libxfce4panel 4.21.3-dev). Recommended: Xfce 4.18 or newer. Uses GTK+ 3 and
libxfce4panel-2.0. Older libxfce4panel versions still build and use the
legacy popup path. Also seen running on Alpine and void (musl).

### Wayland
On Wayland the default (`auto`) mode uses the panel-managed window via
`xfce_panel_plugin_popup_window()` when built against libxfce4panel 4.19 or
newer. This keeps the popup within the monitor on layer-shell compositors
such as labwc and sway. Older builds fall back to a GtkPopover. The explicit
`popover` mode always uses a GtkPopover anchored to the panel button, but GTK3
popovers can extend offscreen because they are subsurfaces rather than
compositor-positioned menu popups. Use Auto or Window mode on sway to avoid
this. The explicit `window` mode uses the panel-managed window on newer builds
and keeps the legacy behaviour otherwise. Pointer positioning is ignored on
Wayland. Wayland support on Xfce 4.22 is experimental; the panel-managed window
needs layer-shell support. X11 remains the fully supported default,
and its behaviour, including pointer popups (`-p`), is unchanged.

The popup mode is the xfconf property `/popup-mode` (`auto`, `popover`,
`window`) and can be forced with the `HAMSTER_POPUP_MODE` environment
variable.

## Translators
It is my wish that the string 'What goes on?' is to be translated with
the following bias:
> I'm looking for a question that is half way between "too formal" and
> "too casual", like something a coworker would ask, but not a boss or
> a kid; something that exceeds the simple "What are you doing?".
> Feel free to employ any appropriate figure of speech unique to
> your language.

## Compilation
Checkout as outlined under Contribution below, cd to the directory and
issue `cmake -B build -DCMAKE_INSTALL_PREFIX=/usr`. If this fails, install any missing
dependencies and you may need to rinse and repeat until success. 
Finally, issue `cmake --build build && sudo cmake --install build`. 
Restart the xfce4 panel with `xfce4-panel -r`.
With the convenience Makefile, use `make && sudo make install`: the install
target only installs existing build outputs and never configures or rebuilds
as root. Avoid `sudo make` and `sudo cmake --build build`, which can leave
root-owned build files. If an earlier privileged build caused this, restore
ownership once with `sudo chown -R "$(id -u):$(id -g)" build`.
If you have Docker or Podman, `make docker` builds the CI image and compiles
the plugin inside it, so no local toolchain is needed.
If the plugin does not show up, check if the installation location is indeed `/usr`; 
the panel does only seek for plugins on a location hardcoded at the time of compilation
of xfce4 itself, so you `/usr/local` may be fine on the \*BSDs.

## Testing
`make test` configures and builds in `build-test/` and runs CTest with the
local toolchain. If you have Docker or Podman, `make docker-test` does the
same inside the CI image (building it first, including Xvfb, sway and a real
xfce4-panel), so no local toolchain or display server is needed.

The suite has five tests. `popup-decision` is display-free and checks the
popup decision matrix for every mode, pointer request, backend and panel
capability, plus the mode-name and display-name parsing. `popup-x11` starts a
throwaway Xvfb and `popup-wayland` a headless sway (`WLR_BACKENDS=headless`);
both run `popup-probe`, which checks that the GDK backend is detected from the
display name and that a real GtkPopover and an undecorated utility toplevel
map. On the Wayland run `popup-runtime.sh` also asserts that the compositor
exports `zwlr_layer_shell_v1`.

`popup-panel-x11` and `popup-panel-wayland` run a live `xfce4-panel` with the
built plugin. They lay out a throwaway prefix containing `libhamster.so` and
`hamster.desktop`, a private HOME/XDG tree and a private session bus for
xfconf, then start the real panel (Xvfb plus xfwm4, or headless sway). Popups
are opened through the real `xfce4-popup-hamstermenu` script, i.e.
`xfce4-panel --plugin-event`, and through clicks on the plugin button. When the
container uid has no passwd entry, the test provides a synthetic one via
libnss-wrapper, which dbus-daemon needs for the private session bus.

The X11 test covers the whole window path: `-p` maps the popup at the pointer
(640,400), a real focus change (clicking an `xmessage` window) dismisses it,
the anchored popup maps below the panel and horizontally overlapping the
button, the button toggles the window with one click, and changing popup-mode
while the window is open is deferred. In popover and auto mode it only asserts
that no mapped utility toplevel is created, i.e. that the popover route was
chosen; its visibility is not asserted.

On Wayland the explicit popover routing is asserted by clicking the button and
checking that no new layer surface appears. The auto and window modes are
asserted end to end: a new `new layer surface` line in the `sway -d` log and
popup pixels in a grim screenshot below the panel, with its top at the panel
edge and its left edge over the button area, closed by the next click and
reopened by one click. A mode switch to popover while the window popup is open
is deferred and keeps both its pixels and its layer surface.

The limits are worth stating plainly. GtkPopover is not visibly rendered under
the test servers used here (Xvfb with xfwm4 and headless sway), even for a
plain GTK3 application, so the live tests never assert popover pixels or
visibility; they only assert that the popover route is selected (no utility
toplevel on X11, no new layer surface on Wayland). Popover visibility stays a
manual desktop check. Wayland has no global pointer grab, so click-outside
dismissal cannot be asserted there; only X11 tests it, through a real focus
change. Autohide remains a manual check.

The live panel tests are skipped when a required tool is missing: xfce4-panel,
xfwm4, xdotool, x11-utils (`xwininfo`, `xprop`, `xmessage`), xfconf
(`xfconf-query`), dbus-daemon, Xvfb, sway and `swaymsg`, grim, and
wayland-scanner/wayland-client for the Wayland click helper. `popup-x11` and
`popup-wayland` are skipped when Xvfb, sway or wayland-utils are missing;
`popup-decision` never skips.
`-DHAMSTER_REQUIRE_RUNTIME_TESTS=ON` makes any missing tool a configure error
(`make docker-test` and CI use it, and the CI image provides them all). CI makes
the tools mandatory only when the image carries the marker
`/usr/share/xfce4-hamster-plugin-ci/live-panel-tools`; an image built before the
live tests warns and skips the panel tests, and the run on the published image is
strict (5/5).

## Packagers
This plug-in is useless without an activatable D-Bus implementation of
`org.gnome.Hamster` and `org.gnome.Hamster.WindowServer`. Hence the
providers of these interfaces should be a hard dependency, even if
any automated check might detect these as unused.
Binary distributions do not necessarily provide `D-Bus-Depends` and
`D-Bus-provides` kind of tags for automatic dependency resolution.
If your distribution doesn't, maybe its time to push the issue.

The icon from hamster is reused as `org.gnome.Hamster.GUI`.

[![Packaging status](https://repology.org/badge/vertical-allrepos/xfce4-hamster-plugin.svg)](https://repology.org/project/xfce4-hamster-plugin/versions)

## Contributing

1. [Fork](https://github.com/projecthamster/xfce4-hamster-plugin/fork) this project
2. Create a topic branch - `git checkout -b my_branch`
3. Push to your branch - `git push origin my_branch`
4. Submit a [Pull Request](https://github.com/projecthamster/xfce4-hamster-plugin/pulls) with your branch
5. That's it!

Patches are most welcome, especially translations.
I'd like to thank the following translators:
- Pavel Borecki
- Sergey Panasenko
- Nicolas Reynolds
- fauno

## Development
The Compilation steps are available within a convenience `Makefile`. If CPack is 
installed, this Makefile may also generate `.deb` and `.rpm` packages 
with the `deb` and `rpm` make targets. Keeping debug info is achieved with `make CMAKE_BUILD_TYPE=DEBUG`. 
This will also make `DBG()` output visible in the console.
