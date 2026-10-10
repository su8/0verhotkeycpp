/*
 * Copyright 10/10/2026 https://github.com/su8/0verhotkeycpp
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
#include <sstream>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "json.hpp"

struct Combo {
  std::string name;
  uint16_t modifiers;
  xcb_keycode_t keycode;
  std::string command;
};

static void atExitSignalHandler(void);
static void OnSIGINTsignalHandler(int signum);
static inline uint16_t modifierNameToMask(const std::string &mod);
static inline void checkIfConfigHasToBeReloaded(void);
static inline void launchCommandThread(const std::string &cmd);
static std::vector<Combo> loadConfig(void);

static std::string configHome = (std::getenv("HOME") ? std::string(std::getenv("HOME")) + std::string("/") : std::string("./")) + ".0verhotkeycpp_XCB_config.json";
static std::mutex cmdMutex;
static std::vector<std::thread> runningThreads;
static std::vector<Combo> combos;
static std::atomic<bool> stopFlag(false);
static int debounceMs = 500;
static xcb_connection_t *conn;
static xcb_key_symbols_t *keysyms;
namespace fs = std::filesystem;
using json = nlohmann::json;

int main(void) {
  std::atexit(atExitSignalHandler);
  std::signal(SIGINT, OnSIGINTsignalHandler);
  uintmax_t x = 0U;
  int screen_num;
  conn = xcb_connect(nullptr, &screen_num);
  if (xcb_connection_has_error(conn)) { std::cerr << "Cannot connect to X server\n"; return EXIT_FAILURE; }
  const xcb_setup_t *setup = xcb_get_setup(conn);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  for (int z = 0; z < screen_num; z++) { xcb_screen_next(&iter); }
  xcb_screen_t *screen = iter.data;
  keysyms = xcb_key_symbols_alloc(conn);
  combos = loadConfig();
  for (auto &sc : combos) {
    xcb_grab_key(conn, 1, screen->root, sc.modifiers, sc.keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(conn, 1, screen->root, sc.modifiers | XCB_MOD_MASK_LOCK, sc.keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(conn, 1, screen->root, sc.modifiers | XCB_MOD_MASK_2, sc.keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  }
  xcb_flush(conn);
  std::cout << "Listening for keys...\n";
  xcb_generic_event_t *event;
  while ((event = xcb_wait_for_event(conn)) && !stopFlag.load()) {
    uint8_t type = event->response_type & ~0x80;
    if (type == XCB_KEY_PRESS) {
      xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
      for (auto &sc : combos) {
        if (kp->detail == sc.keycode && (kp->state & (XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_1)) == sc.modifiers) {
          std::cout << "Key(s) detected: " << sc.name << " → launching " << sc.command << "\n";
          launchCommandThread(sc.command.c_str()); std::this_thread::sleep_for(std::chrono::milliseconds(debounceMs)); pthread_cancel(runningThreads[x].native_handle()); runningThreads[x].detach(); x++;
        }
      }
    }
    free(event);
  }
  xcb_key_symbols_free(keysyms);
  xcb_disconnect(conn);
  return EXIT_SUCCESS;
}

static void atExitSignalHandler(void) { stopFlag.store(true); }
static void OnSIGINTsignalHandler(int signum) { static_cast<void>(signum); stopFlag.store(true); }

uint16_t modifierNameToMask(const std::string &mod) {
  if (mod == "CTRL") return XCB_MOD_MASK_CONTROL;
  if (mod == "SHIFT") return XCB_MOD_MASK_SHIFT;
  if (mod == "ALT") return XCB_MOD_MASK_1;
  return 0;
}

static inline void checkIfConfigHasToBeReloaded(void) {
  static auto oldTime = fs::last_write_time(configHome);
  auto newTime = fs::last_write_time(configHome);
  if (newTime != oldTime) {
    combos.clear();
    combos = loadConfig();
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

static inline std::vector<Combo> loadConfig(void) {
  std::ifstream cfgFile(configHome);
   if (!cfgFile) { std::cerr << "Error: Could not open " << configHome << "\n"; exit(EXIT_FAILURE); }
  json config;
  cfgFile >> config;
  std::vector<Combo> combosLoad;
  debounceMs = config["sleep"]["debounceMs"].get<int>();
  for (auto &sc : config["combos"]) {
    uint16_t mods = 0;
    KeySym ks = 0;
    for (auto &key : sc["keys"]) {
      std::string k = key.get<std::string>();
      uint16_t mask = modifierNameToMask(k);
      if (mask) { mods |= mask; }
      else { ks = XStringToKeysym(k.c_str()); }
    }
    xcb_keycode_t *codes = xcb_key_symbols_get_keycode(keysyms, ks);
    if (!codes) { std::cerr << "Invalid key: " << ks << "\n"; continue; }
    combosLoad.push_back({sc["name"], mods, codes[0], sc["command"].get<std::string>()});
    free(codes);
  }
  return combosLoad;
}