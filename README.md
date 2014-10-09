##About
This is a recreation of the gnome-shell hamster extension as a xfce4 panel plugin.
See also the hamster project: <https://github.com/projecthamster>

Dependencies: `xfce4-panel xfconf hamster-applet`

Build dependencies[debian, ubuntu]: `libxfce4ui-1-dev xfce4-panel-dev libxfconf-0-dev libxfce4util-dev`

Common: `build-essential autoconf automake intltool libtool`

Tested on ubuntu 14.04 and Arch with xfce 4.10; however 4.8 should also work.

##Translators
It is my wish that the string 'What goes on?' is to be translated with the following bias:
> I'm looking for a question that is half way between "too formal" and "too casual", 
> like something a coworker would ask, but not a boss or a kid; something that exceeds the simple "What are you doing?".
> Feel free to employ any appropriate figure of speech unique to your language.

##Hacking
Regenerate hamster DBUS-Glib with:
```
gdbus-codegen --generate-c-code hamster --interface-prefix org.gnome. org.gnome.Hamster.xml
gdbus-codegen --generate-c-code windowserver --interface-prefix org.gnome.Hamster org.gnome.Hamster.WindowServer.xml
```
