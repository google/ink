// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iomanip>
#include <iostream>

int main() {
  // Optimize for I/O speed.
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  unsigned char byte;
  int count = 0;
  while (std::cin.read(reinterpret_cast<char*>(&byte), 1)) {
    std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte) << ", ";
    if (count++ % 12 == 0) {
      std::cout << "\n";
    }
  }
  return 0;
}
