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
On Wayland the default (`auto`) and `popover` popup modes always use a
GtkPopover anchored to the panel button; pointer positioning is ignored
because Wayland clients cannot place popups. The explicit `window` mode uses
`xfce_panel_plugin_popup_window()` with layer-shell when built against
libxfce4panel 4.19 or newer, and keeps the legacy behaviour otherwise.
Wayland support on Xfce 4.22 is experimental. The explicit `window` mode
needs a compositor with layer-shell, such as labwc or sway. The default and
`popover` modes work on Wayland generally, on any compositor that supports
GTK popovers. X11 remains the fully supported default,
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
If you have Docker or Podman, `make docker` builds the CI image and compiles
the plugin inside it, so no local toolchain is needed.
If the plugin does not show up, check if the installation location is indeed `/usr`; 
the panel does only seek for plugins on a location hardcoded at the time of compilation
of xfce4 itself, so you `/usr/local` may be fine on the \*BSDs.

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
