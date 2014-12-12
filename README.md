## About
This is a recreation of the gnome-shell hamster extension as a xfce4 panel plugin.
See also the hamster project: <https://github.com/projecthamster/hamster>

Dependencies: `xfce4-panel xfconf hamster-applet|hamster-time-tracker`

Build dependencies[debian, ubuntu]: `libxfce4ui-1-dev xfce4-panel-dev libxfconf-0-dev libxfce4util-dev`

Common: `build-essential autoconf automake intltool libtool`

Tested on ubuntu 14.04 and Arch with xfce 4.10; however 4.8 should also work.

## Translators
It is my wish that the string 'What goes on?' is to be translated with the following bias:
> I'm looking for a question that is half way between "too formal" and "too casual", 
> like something a coworker would ask, but not a boss or a kid; something that exceeds the simple "What are you doing?".
> Feel free to employ any appropriate figure of speech unique to your language.

## Packagers
This plug-in is useless without an activatable D-Bus implementation of 
`org.gnome.Hamster` and `org.gnome.Hamster.WindowServer`. Hence the 
providers of these interfaces should be a hard dependency, even if
any automated check might detect these as unused.
Binary distributions do not necessarily provide `D-Bus-Depends` and 
`D-Bus-provides` kind of tags for automatic dependecy resolution. 
If your distribution doesn't, maybe its time to push the issue.

The icon from hamster is reused and defaults to `hamster-applet`.
If your distribution already ships the non-gnome2-panel variety, force 
the usage of `hamster-time-tracker` (or any other named icon that fits)
by using e.g. `./configure --with-icon_name=hamster-time-tracker`.

The generated .lo files are best purged since no linkage or development 
packages are provided.

## Hacking
Regenerate hamster DBUS-Glib with:
```
gdbus-codegen --generate-c-code hamster --interface-prefix org.gnome. org.gnome.Hamster.xml
gdbus-codegen --generate-c-code windowserver --interface-prefix org.gnome.Hamster org.gnome.Hamster.WindowServer.xml
```

## Contributing

1. [Fork](https://github.com/projecthamster/xfce4-hamster-plugin/fork) this project
2. Create a topic branch - `git checkout -b my_branch`
3. Push to your branch - `git push origin my_branch`
4. Submit a [Pull Request](https://github.com/projecthamster/xfce4-hamster-plugin/pulls) with your branch
5. That's it!
