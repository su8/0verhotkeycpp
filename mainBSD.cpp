/*
 * Copyright 10/09/2026 https://github.com/su8/0verhotkeycpp
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
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include "json.hpp"

struct termios origTermios;
struct KeyCombo {
  std::vector<unsigned char> sequence;
  std::string command;
};
static std::string configHome = (std::getenv("HOME") ? std::string(std::getenv("HOME")) + std::string("/") : std::string("./")) + ".0verhotkeycpp_BSD_config.json";
static std::vector<KeyCombo> combos;
std::mutex cmdMutex;
static std::vector<std::thread> runningThreads;
static std::atomic<bool> stopFlag(false);
namespace fs = std::filesystem;
using json = nlohmann::json;

static inline bool match_sequence(const std::vector<unsigned char> &input, const std::vector<unsigned char> &pattern);
static inline void rawMode(void);
static inline std::vector<unsigned char> keyToSequence(const std::string &s);
static inline std::vector<KeyCombo> loadCombosJson(const std::string &filename);
static inline void checkIfConfigHasToBeReloaded(void);
static void atExitSignalHandler(void);
static void OnSIGINTsignalHandler(int signum);
static inline void launchCommandThread(const std::string &cmd);

int main(void) {
  std::atexit(atExitSignalHandler);
  std::signal(SIGINT, OnSIGINTsignalHandler);
  combos = loadCombosJson(configHome);
  struct termios raw;
  uintmax_t x = 0U;
  if (tcgetattr(STDIN_FILENO, &origTermios) == -1) { perror("tcgetattr"); return EXIT_FAILURE; }
  raw = origTermios;
  raw.c_lflag &= ~(ICANON | ECHO | IEXTEN);
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) { perror("tcsetattr"); return EXIT_FAILURE; }
  std::cout << "Press keys (Ctrl+C to quit)\n";
  while (!stopFlag.load()) {
    checkIfConfigHasToBeReloaded();
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 0.5 sec timeout
    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    if (ret == -1) {
      perror("select");
      break;
    } else if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
      std::vector<unsigned char> input_seq;
      unsigned char ch;
      ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
      if (bytesRead > 0) {
        input_seq.push_back(ch);
        // If ESC, read more bytes quickly (escape sequence)
        if (ch == 0x1B) {
          while (!stopFlag.load()) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            struct timeval t2 = {0, 20000}; // 20ms
            int r2 = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &t2);
            if (r2 > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
              unsigned char ch2;
              if (read(STDIN_FILENO, &ch2, 1) > 0) { input_seq.push_back(ch2); } 
            } else { break; }
          }
        }
        for (const auto &combo : combos) {
          if (match_sequence(input_seq, combo.sequence)) {
            std::cout << "Matched combo -> launching: " << combo.command << "\n";
            launchCommandThread(combo.command.c_str()); std::this_thread::sleep_for(std::chrono::milliseconds(500)); pthread_cancel(runningThreads[x].native_handle()); runningThreads[x].detach(); x++;
          }
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

static void atExitSignalHandler(void) { tcsetattr(STDIN_FILENO, TCSANOW, &origTermios); stopFlag.store(true); }
static void OnSIGINTsignalHandler(int signum) { static_cast<void>(signum); tcsetattr(STDIN_FILENO, TCSANOW, &origTermios); stopFlag.store(true); }

void rawMode(void) {
  struct termios raw;
  if (tcgetattr(STDIN_FILENO, &origTermios) == -1) { perror("tcgetattr"); exit(EXIT_FAILURE); }
  raw = origTermios;
  raw.c_lflag &= ~(ICANON | ECHO | IEXTEN);
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) { perror("tcsetattr"); exit(EXIT_FAILURE); }
}

bool match_sequence(const std::vector<unsigned char> &input, const std::vector<unsigned char> &pattern) { return input == pattern; }

// Convert key string to byte sequence
std::vector<unsigned char> keyToSequence(const std::string &s) {
  std::vector<unsigned char> seq;
  if (s.size() == 1) {
    seq.push_back(static_cast<unsigned char>(s[0]));
  } else if (s.rfind("Ctrl+", 0) == 0 && s.size() == 6) {
    char letter = s[5];
    seq.push_back(static_cast<unsigned char>(letter & 0x1F));
  } else if (s.find("\\u") != std::string::npos) {
    // Unicode escape sequence like "\u001B[1;5A"
    std::string temp = s;
    for (size_t x = 0; x < temp.size();) {
      if (temp[x] == '\\' && x + 1 < temp.size() && temp[x + 1] == 'u') {
        unsigned int code;
        sscanf(temp.substr(x + 2, 4).c_str(), "%x", &code);
        seq.push_back(static_cast<unsigned char>(code));
        x += 6; // skip \uXXXX
      } else {
        seq.push_back(static_cast<unsigned char>(temp[x]));
        x++;
      }
    }
  } else {
    try {
      int code = std::stoi(s);
      seq.push_back(static_cast<unsigned char>(code));
    } catch (...) {
      std::cerr << "Invalid key format: " << s << "\n";
      exit(EXIT_FAILURE);
    }
  }
  return seq;
}

// Load combos from JSON config
std::vector<KeyCombo> loadCombosJson(const std::string &filename) {
  std::ifstream file(filename);
  if (!file) { std::cerr << "Error: Cannot open config file " << filename << "\n"; exit(EXIT_FAILURE); }
  json j;
  file >> j;
  std::vector<KeyCombo> combos;
  for (auto &combo : j["combos"]) {
    KeyCombo kc;
    for (auto &key : combo["keys"]) {
      if (key.is_string()) {
        auto seq = keyToSequence(key.get<std::string>());
        kc.sequence.insert(kc.sequence.end(), seq.begin(), seq.end());
      } else if (key.is_number()) {
        kc.sequence.push_back(static_cast<unsigned char>(key.get<int>()));
      }
    }
    kc.command = combo["command"].get<std::string>();
    combos.push_back(kc);
  }
  return combos;
}

static inline void checkIfConfigHasToBeReloaded(void) {
  static auto oldTime = fs::last_write_time(configHome);
  auto newTime = fs::last_write_time(configHome);
  if (newTime != oldTime) {
    combos.clear();
    combos = loadCombosJson(configHome);
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