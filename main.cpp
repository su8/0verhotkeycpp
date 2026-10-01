/*
 * Copyright 10/01/2026 https://github.com/su8/0verhotkeycpp
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
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <map>
#include <set>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <algorithm>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sstream>
  #include <algorithm>
  #include <dirent.h>
  #include <cstring>
  #include <sys/ioctl.h>
  #include <linux/input.h>
#endif /* _WIN32 */

#include "json.hpp"

using json = nlohmann::json;

struct Combo {
  std::vector<std::string> keys;
  std::string command;
};

#ifdef __linux__
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
#endif /* __linux__ */

int main(void) {
  // Load JSON config
  std::ifstream cfgFile("config.json");
  if (!cfgFile) { std::cerr << "Could not open config.json\n"; return EXIT_FAILURE; }
  json cfg;
  cfgFile >> cfg;
  std::vector<Combo> combos;
  for (auto &c : cfg["combos"]) {
    Combo combo;
    combo.keys = c["keys"].get<std::vector<std::string>>();
    combo.command = c["command"].get<std::string>();
    combos.push_back(combo);
  }
  // Load keycodes from JSON
  std::map<std::string, int> keycodes;
  for (auto &kv : cfg["keycodes"].items()) {
    keycodes[kv.key()] =
#ifdef _WIN32
    kv.value()["windows"];
#else
    kv.value()["linux"];
#endif /* _WIN32 */
  }

#ifdef _WIN32
  std::cout << "Listening (Windows)...\n";
  while (true) {
    for (auto &combo : combos) {
      bool match = true;
      for (auto &k : combo.keys) { if (!(GetAsyncKeyState(keycodes[k]) & 0x8000)) { match = false; break; } }
      if (match) {
        std::system(combo.command.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // debounce
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
#else
  // See /usr/include/linux/input-event-codes.h
  std::string device = findKeyboardDevice();
  if (device.empty()) { std::cerr << "No keyboard device found. Try running as root.\n"; return EXIT_FAILURE; }
  std::cout << "Using device: " << device << "\n";
  int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) { perror("open"); return EXIT_FAILURE; }
  std::map<int,bool> keyState;
  struct input_event ev;
  std::cout << "Listening for (Linux /dev/input)...\n";
  while (true) {
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) continue;
      if (ev.type == EV_KEY) {
        keyState[ev.code] = (ev.value != 0);
        for (auto &combo : combos) {
          bool match = true;
          for (auto &k : combo.keys) { if (!keyState[keycodes[k]]) { match = false; break; } }
          if (match) {
            std::system(combo.command.c_str());
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // debounce
          }
        }
      }
    }
  close(fd);
#endif /* _WIN32 */
  return EXIT_SUCCESS;
}