0verhotkeycpp  [![C/C++ CI](https://github.com/su8/0verhotkeycpp/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/su8/0verhotkeycpp/actions/workflows/c-cpp.yml)
======

Hotkey is a simple program that listens on an evdev input device and reacts by
launching a command.

Configuration
-------------

Edit the `.0verhotkeycpp_config.json` or the `BSD` config one, edit the file(s) to your heart's desire. Use the provided **printkey.cpp** if on `Windows` to get the numbers behind the desired keys, if on linux see `/usr/include/linux/input-event-codes.h` , optionally you can install `evtest` and press the desired keyboard key(s).

In **Linux** you can `sudo visudo` and type the `0verhotkeycpp` in there to launch it without the need for the root password. You can start the program within `.xinitrc` or other file used to start your Window Manager. Eventually you can have it auto start from your `init` system.

Compile
-------

If on **Linux** or **BSD** or **Mac** compile with:

```bash
# will default to XCB
make XCB -j8 # 8 cores/threads to use in parallel compile

# will default to e.g. /dev/input/event0 listener and keyboard detection (Linux and Windows)
make devInput -j8

# BSD/Mac, on BSD you can still use the XCB version
make BSD -j8 # 8 cores/threads to use in parallel compile

# to isntall the binary
sudo make install

# to uninstall it
sudo make uninstall
```

If you choose the `XCB` version you must install xcb and x11/xorg, in Debian it's `sudo apt install libxcb1-dev libxcb-keysyms1-dev`

---

## Don't forget to copy `.0verhotkeycpp_config.json` or the **BSD** config to your /root/ folder if on Linux/BSD/Mac

---

Windows users
-------------

Tested with [Visual Studio Code Editor](https://code.visualstudio.com/download), but you need to install [MingW](https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev0/x86_64-12.2.0-release-posix-seh-rt_v10-rev0.7z), once downloaded extract it to **C:\MingW**, then re-open [Visual Studio Code Editor](https://code.visualstudio.com/download), you might want to install C\C++ extensions if you plan to write C\C++ code with the editor. If you plan to contribute to this project go to **File->Preferences->Settings** and type to search for **cppStandard** and set it to c17 to both C++ and C.

I use **One Monokai** theme for the [VScode Editor](https://code.visualstudio.com/download)

In [Visual Studio Code Editor](https://code.visualstudio.com/download), go to **Terminal->Configure Tasks...->Create tasks.json from template** and copy and paste this into it:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
        "type": "cppbuild",
        "label": "C/C++",
        "command": "C:\\MingW\\bin\\g++.exe",
        "args": [
            "-fdiagnostics-color=always",
            "-std=c++20",
            "-ggdb",
            "-lpthread",
            "-D_DEFAULT_SOURCE",
            "-Wall",
            "-Wextra",
            "-O2",
            "-pipe",
            "-pedantic",
            "-Wundef",
            "-Wshadow",
            "-W",
            "-Wwrite-strings",
            "-Wcast-align",
            "-Wstrict-overflow=5",
            "-Wconversion",
            "-Wpointer-arith",
            "-Wformat=2",
            "-Wsign-compare",
            "-Wendif-labels",
            "-Wredundant-decls",
            "-Winit-self",
            "-luser32",
            "${file}",
            "-o",
            "${fileDirname}/${fileBasenameNoExtension}"
        ],
        "options": {
            "cwd": "C:\\MingW\\bin"
        },
        "problemMatcher": [
            "$gcc"
        ],
        "group": {
            "kind": "build",
            "isDefault": true
        },
        "detail": "compiler: C:\\MingW\\bin\\g++.exe"
    }
]
}
```

### To compile the main.cpp press **CTRL** + **SHIFT** + **B**

Optioanlly if you want to play around with the code from VSCode's console -- wait until it compiles, after that press **CTRL** + **SHIFT** + **\`** and paste this `cp -r C:\Users\YOUR_USERNAME_GOES_HERE\Desktop\main.exe C:\MingW\bin;cd C:\MingW\bin;.\main.exe C:\`

Copy `.0verhotkeycpp_config.json` to `C:\MingW\bin` before starting the program for very first time. If you do other changes to `.0verhotkeycpp_config.json` make sure to copy it to the `MingW\bin` folder again, or just edit the **MingW** config one.

There is a **.bat** script that you can use to launch the program, instead doing it manually or from the VSCode editor.

Limitations
-----------

Hotkey can only listen on a single device file. If you want to listen on
multiple device files, just configure and install it multiple times under
different names. This way, you can have completely different configurations for
each keyboard you have. Or just supply argument with the keyboard device in 
**Linux** like this `sudo 0verhotkey++ /dev/input/event13` where `event13` is your other keyboard.

Hotkey will only listen for `EV_KEY` events. This is by design; keyboards
generally don't emit any other events. You should probably use something like
acpid for listening to lid events and the like, but it's trivial to change which
event(s) Hotkey listens to.
