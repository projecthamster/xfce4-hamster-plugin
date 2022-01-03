## About
This is a recreation of the gnome-shell hamster extension as a xfce4 panel plugin.
See also the hamster project: <https://github.com/projecthamster/hamster>

Dependencies: `xfce4-panel xfconf hamster-applet|hamster-time-tracker`

Build dependencies[debian, ubuntu]: `libxfce4ui-2-dev libxfce4panel-2.0-dev
libxfconf-0-dev libxfce4util-dev`

Common: `build-essential cmake intltool`

Tested on Arch with xfce 4.16, Ubuntu 20.04 with xfce 4.14 and
Debian Buster with xfce 4.12. Uses GTK+3 only, requires APIs that are
not available before xfce 4.10. Support for older versions are in
available in the git history. Also seen running on Alpine and void (musl).

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
dependencies and repeat until success. 
Finally, issue `cd build && make && sudo make install`. 
Restart the xfce4 panel with `xfce4-panel -r`.

## Packagers
This plug-in is useless without an activatable D-Bus implementation of
`org.gnome.Hamster` and `org.gnome.Hamster.WindowServer`. Hence the
providers of these interfaces should be a hard dependency, even if
any automated check might detect these as unused.
Binary distributions do not necessarily provide `D-Bus-Depends` and
`D-Bus-provides` kind of tags for automatic dependency resolution.
If your distribution doesn't, maybe its time to push the issue.

The icon from hamster is reused as `org.gnome.Hamster.GUI`.

The generated .lo files are best purged since no linkage or development
packages are provided.

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


