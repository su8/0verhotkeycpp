HOTKEY
======

Hotkey is a simple program that listens on an evdev input device and reacts by
launching a command.

Usage
-----

Just add the following in your .xinitrc (if you are not using Gentoo) or other configuration file used to start your WM. Start `hotkey` with **&** at the end (**hotkey /dev/input/event5 &**)

```bash
sudo hotkey /dev/input/event5
# To get the hex code behind the keyboard input one
sudo printev /dev/input/event5
```

Dependencies
------------

Hotkey depends on Linux configured with `INPUT_EVDEV` and a computer with a
keyboard.

Configuration
-------------

Edit the `hotkey.c` file to your heart's desire. It contains a few examples and
an explanation of what the options mean. The configuration gets compiled into
the binary, which keeps the code simple and makes startup quick and secure.

Use the included printev program to see what values to put in the config file.

OpenRC support
--------------

Hotkey ships with an OpenRC init script. If you don't use OpenRC, you can just
skip this section and write your own init script. I'm happy to accept init
scripts for other service managers.

The openrc directory contains the files `hotkey.init` and `hotkey.conf`. The
first is the init script - rename it to `hotkey` and put it in `/etc/init.d`.
Likewise for the conf file, which goes in `/etc/conf.d`. You can use this to
start hotkey at boot. The conf file lets you change which user to run hotkey as.

Limitations
-----------

Hotkey can only listen on a single device file. If you want to listen on
multiple device files, just configure and install it multiple times under
different names. This way, you can have completely different configurations for
each keyboard you have.

Hotkey will only listen for `EV_KEY` events. This is by design; keyboards
generally don't emit any other events. You should probably use something like
acpid for listening to lid events and the like, but it's trivial to change which
event(s) Hotkey listens to.

Author and copyright
--------------------

Hotkey is Copyright (C) 2014 Wiktor W Brodlo.

Hotkey development continues under my (**su8**) repository too.

The files in the openrc directory are released under the GNU GPL version 2, as
required by Gentoo.


*Patches are welcome!*