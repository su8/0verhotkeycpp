HOTKEY
======

Hotkey is a simple program that listens on an evdev input device and reacts by
launching a command.

Usage
-----

```bash
while true; do
  sudo 0verhotkey++
done
```

Configuration
-------------

Edit the `main.cpp` file to your heart's desire.

If on Windows edit the following line(s):

https://github.com/su8/0verhotkeycpp/blob/5347d96a4737cdb842e6e5d316b0d9dfa265a8fd/main.cpp#L84

If on Unix edit the following line(s):

https://github.com/su8/0verhotkeycpp/blob/5347d96a4737cdb842e6e5d316b0d9dfa265a8fd/main.cpp#L100

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


Compile
-------

If on **Linux/\*BSD/Mac** compile with:

```bash
make -j8 # 8 cores/threads to use in parallel compile
sudo/doas make install

# to uninstall it
sudo/doas make uninstall
```

---

## Windows users

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

To overwrite some files with the `0vershred.bat` script, copy the name of the destination folder or once the script is started you can provide the desired folder, but you will have to use `Double Quotes` if the folder has spaces in it's name, after that right click it in the opened CMD window and press enter.

### To compile the main.cpp press **CTRL** + **SHIFT** + **B**

Optioanlly if you want to play around with the code from VSCode's console -- wait until it compiles, after that press **CTRL** + **SHIFT** + **\`** and paste this `cp -r C:\Users\YOUR_USERNAME_GOES_HERE\Desktop\main.exe C:\MingW\bin;cd C:\MingW\bin;.\main.exe C:\`