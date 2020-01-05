#pragma once
#include <Arduino.h>

namespace Marlin
{
  void init();
  void update();
  void update(int c);
  void lcdMessage(const char* s);

  enum class Completion : uint8_t
  {
    Indeterminate,
    Success,
    Error
  };
  using AsyncCompletion = void(String line, Completion complete, void* context);
  void setHandler(AsyncCompletion* complt, void* context);

  bool command(const char* s);
  bool command(const char* s, const char* arg);
}
