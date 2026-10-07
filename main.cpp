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
#include <mutex>
#include <filesystem>
#include <csignal>
#include <atomic>

#ifdef _WIN32
  #include <windows.h>
static std::string configHome = "C:\\MingW\\bin\\.0verhotkeycpp_config.json";
#else
  #include <algorithm>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sstream>
  #include <dirent.h>
  #include <cstring>
  #include <sys/ioctl.h>
  #include <linux/input.h>
static inline std::string findKeyboardDevice(void);
static std::string configHome = (std::getenv("HOME") ? std::string(std::getenv("HOME")) + std::string("/") : std::string("./")) + ".0verhotkeycpp_config.json";
#endif /* _WIN32 */

#include "json.hpp"

static inline void loadConfig(void);
static inline void checkIfConfigHasToBeReloaded(void);
static inline void launchCommandThread(const std::string &cmd);
static void signalHandler(void);
static void signalHandler2(int signum);

using json = nlohmann::json;
namespace fs = std::filesystem;
struct Combo {
  std::vector<std::string> keys;
  std::string command;
};
static std::vector<Combo> combos;
static std::map<std::string, int> keycodes;
static std::mutex cmdMutex;
static std::vector<std::thread> runningThreads;
static std::atomic<bool> stopFlag(false);
static int debounceMs = 500;

int main(int argc, char *argv[]) {
  static_cast<void>(argc); static_cast<void>(argv);
  std::atexit(signalHandler);
  std::signal(SIGINT, signalHandler2);
  loadConfig();
  uintmax_t x = 0U;
#ifdef _WIN32
  std::cout << "Listening (Windows)...\n";
  while (!stopFlag.load()) {
    checkIfConfigHasToBeReloaded();
    for (auto &combo : combos) {
      bool match = true;
      for (auto &k : combo.keys) { if (!(GetAsyncKeyState(keycodes[k]) & 0x8000)) { match = false; break; } }
      if (match) { launchCommandThread(combo.command.c_str()); std::this_thread::sleep_for(std::chrono::milliseconds(debounceMs)); pthread_cancel(runningThreads[x].native_handle()); runningThreads[x].detach(); x++; }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
#else
  std::string device;
  if (argc >= 2) { device = argv[1]; }
  else { device = findKeyboardDevice(); }
  if (device.empty()) { std::cerr << "No keyboard device found. Try running as root.\n"; return EXIT_FAILURE; }
  std::cout << "Using device: " << device << "\n";
  int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) { perror("open"); return EXIT_FAILURE; }
  std::map<int,bool> keyState;
  struct input_event ev;
  std::cout << "Listening (Linux)...\n";
  while (!stopFlag.load()) {
    checkIfConfigHasToBeReloaded();
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) { continue; }
    if (ev.type == EV_KEY) {
      keyState[ev.code] = (ev.value != 0);
      for (auto &combo : combos) {
        bool match = true;
        for (auto &k : combo.keys) { if (!keyState[keycodes[k]]) { match = false; break; } }
        if (match) { launchCommandThread(combo.command.c_str()); std::this_thread::sleep_for(std::chrono::milliseconds(debounceMs)); pthread_cancel(runningThreads[x].native_handle()); runningThreads[x].detach(); x++; }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  close(fd);
#endif /* _WIN32 */
  return EXIT_SUCCESS;
}

static void signalHandler(void) { stopFlag.store(true); }
static void signalHandler2(int signum) { static_cast<void>(signum); stopFlag.store(true); }

static inline void loadConfig(void) {
  std::ifstream cfgFile(configHome);
  if (!cfgFile) { std::cerr << "Could not open " << configHome << "\n"; exit(EXIT_FAILURE); }
  json cfg;
  cfgFile >> cfg;
  debounceMs = cfg["sleep"]["debounceMs"].get<int>();
  for (auto &c : cfg["combos"]) {
    Combo combo;
    combo.keys = c["keys"].get<std::vector<std::string>>();
    combo.command = c["command"].get<std::string>();
    combos.push_back(combo);
  }
  for (auto &kv : cfg["keycodes"].items()) {
    keycodes[kv.key()] =
#ifdef _WIN32
    kv.value()["windows"];
#else
    kv.value()["linux"];
#endif /* _WIN32 */
  }
}

static inline void checkIfConfigHasToBeReloaded(void) {
  static auto oldTime = fs::last_write_time(configHome);
  auto newTime = fs::last_write_time(configHome);
  if (newTime != oldTime) {
    combos.clear();
    keycodes.clear();
    loadConfig();
    oldTime = newTime;
  }
}

static inline void launchCommandThread(const std::string &cmd) {
  std::lock_guard<std::mutex> lock(cmdMutex);
  runningThreads.emplace_back([cmd]() {
    int ret = std::system(cmd.c_str());
    if (ret == -1) { std::cerr << "Failed to execute command: " << cmd << "\n"; }
  });
}

#ifdef __linux__
static inline std::string findKeyboardDevice(void) {
  char name[256] = {'\0'};
  for (const auto &entry : fs::directory_iterator(std::string("/dev/input/"))) {
    if (strncmp(entry.path().filename().string().c_str(), "event", 5) == 0) {
      int fd = open(entry.path().string().c_str(), O_RDONLY);
      if (fd >= 0) {
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
          std::string devName(name);
          if (devName.rfind("Keyboard") != std::string::npos || devName.rfind("keyboard") != std::string::npos) { close(fd); return entry.path().string(); }
        }
        close(fd);
      }
    }
  }
  for (const auto &entry : fs::directory_iterator(std::string("/dev/input/by-id/"))) { if (entry.path().string().rfind("kbd") != std::string::npos) { return entry.path().string(); } }
  return "";
}
#endif /* __linux__ */