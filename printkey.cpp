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
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>

int main(void) {
  std::cout << "Press any key (ESC to exit)...\n";
  while (true) {
    for (int vk = 1; vk <= 0xFE; ++vk) {
      SHORT state = GetAsyncKeyState(vk);
      if (state & 0x8000) {
        std::cout << "Key pressed: 0x" << std::hex << std::uppercase << vk << std::dec << "\n";
        if (vk == VK_ESCAPE) { return EXIT_SUCCESS; }
        Sleep(200);
      }
    }
    Sleep(10);
  }
  return EXIT_SUCCESS;
}