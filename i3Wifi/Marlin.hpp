#pragma once
#include <Arduino.h>

namespace Marlin
{
  void init();
  void update();
  void lcdMessage(const char* s);

  void write(uint8_t* buf, size_t len);
  bool command(const char* s);
  bool command(const char* s, const char* arg);
}
