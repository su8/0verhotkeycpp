/*
 * Copyright 10/01/2026 https://github.com/su8/0verhotkey++
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <set>
#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <algorithm>
  #include <fcntl.h>
  #include <unistd.h>
  #include <linux/input.h>
  #include <dirent.h>
  #include <cstring>
  #include <sys/ioctl.h>
#endif

void launchCommandAsync(const std::string &cmd);

// Launch a system command asynchronously
void launchCommandAsync(const std::string &cmd) {
  std::thread([cmd]() {
    int ret = std::system(cmd.c_str());
    if (ret == -1) { std::cerr << "Failed to execute command: " << cmd << "\n"; }
  }).detach();
}

#ifdef _WIN32
bool isKeyDown(int vkCode);
bool isKeyDown(int vkCode) {
  return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
}
#else
std::string findKeyboardDevice(void);
std::string findKeyboardDevice(void) {
  const char *devPath = "/dev/input/";
  DIR *dir = opendir(devPath);
  if (!dir) return "";

  struct dirent *entry;
  char name[256];
  while ((entry = readdir(dir)) != nullptr) {
    if (strncmp(entry->d_name, "event", 5) == 0) {
      std::string fullPath = std::string(devPath) + entry->d_name;
      int fd = open(fullPath.c_str(), O_RDONLY);
      if (fd >= 0) {
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
          std::string devName(name);
          if (devName.find("Keyboard") != std::string::npos || devName.find("keyboard") != std::string::npos) { close(fd); closedir(dir); return fullPath; }
        }
        close(fd);
      }
    }
  }
  closedir(dir);
  return "";
}
#endif

int main(void) {
#ifdef _WIN32
  // Map of keys → command
  std::map<std::set<int>, std::string> hotkeys = {
    {{VK_CONTROL, VK_SHIFT, 0x58}, "cmd /c echo Ctrl+Shift+X pressed!"}, // control shift x keys
    {{VK_MENU, 0x41}, "cmd /c echo Alt+A pressed!"} // Alt + A
  };

  while (true) {
    for (auto &pair : hotkeys) {
      bool allPressed = true;
      for (int key : pair.first) { if (!isKeyDown(key)) { allPressed = false; break; } }
      if (allPressed) { launchCommandAsync(pair.second); std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

#else
    // See /usr/include/linux/input-event-codes.h
  std::map<std::set<int>, std::string> hotkeys = {
    {{KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_X}, "echo Ctrl+Shift+X pressed!"},
    {{KEY_LEFTALT, KEY_A}, "echo Alt+A pressed!"}
  };

  std::string device = findKeyboardDevice();
  if (device.empty()) { std::cerr << "No keyboard device found. Try running as root.\n"; return EXIT_FAILURE; }

  std::cout << "Using device: " << device << "\n";
  int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) { perror("open"); return EXIT_FAILURE; }

  std::set<int> pressedKeys;
  struct input_event ev;
  while (true) {
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == (ssize_t)sizeof(ev)) {
      if (ev.type == EV_KEY) {
        if (ev.value == 1) { // key down
          pressedKeys.insert(ev.code);
        } else if (ev.value == 0) { // key up
          pressedKeys.erase(ev.code);
        }
        // Check hotkeys
        for (auto &pair : hotkeys) {
          if (std::includes(pressedKeys.begin(), pressedKeys.end(), pair.first.begin(), pair.first.end())) { launchCommandAsync(pair.second); std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  close(fd);
#endif
  return EXIT_SUCCESS;
}