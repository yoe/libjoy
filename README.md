libjoy
======

This library, when ready, will provide a GObject interface to the Linux
joystick API. This should make the use of a joystick for a GTK application all
that much easier.

I actually already wrote such a library back when I was first learning
about GObject, now about 10 years ago. I accidentally _removed_ the code
at the time, after having spent about a day or so writing it.

The library also contains a small GTK application which queries the name
of the joystick on /dev/input/js0 and shows some information. That needs
some more work, too.
