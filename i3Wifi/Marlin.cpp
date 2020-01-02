#include "Marlin.hpp"

template <typename Fn>
void readLines(Fn&& emitLine, bool oneLine = false)
{
  constexpr size_t kMaxLength = 96;
  static char line[kMaxLength];
  static size_t lineLen = 0;

  do
  {
    while(Serial.available() > 0)
    {
      char c = Serial.read();
      if (c == '\r' || c == '\n')
      {
        line[lineLen++] = 0;
        emitLine(line);
        lineLen = 0;

        if (oneLine)
          return;
      }
      else
      {
        line[lineLen++] = c;
      }
    }
  } while (oneLine);
}

namespace Marlin
{

void init()
{
  // Increase baud
//  Serial.begin(250000);
//  if (command(String("M575 ") + String(1000000)))
//    Serial.begin(1000000);
}

void update()
{
  readLines([](const char* s)
    {
      // TODO
    });
}

void lcdMessage(const char* s)
{
  command(String("M117 ") + s);
}

void write(uint8_t* buf, size_t len)
{
  Serial.write(buf, len);
}

bool command(String s)
{
  return command(s.c_str());
}

bool command(const char* s)
{
  bool success = false;
  Serial.println(s);
  readLines([&](const char* s) { success = (String(s) == "ok"); }, true);
  return success;
}

}
