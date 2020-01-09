/* 
 * Copyright (C) 2020  Nicholas Gill
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Marlin.hpp"

namespace Marlin
{

Completion completion = Completion::Indeterminate;
AsyncCompletion* completionCallback = nullptr;
void* completionContext = nullptr;

void init()
{
  // Increase baud
//  Serial.begin(250000);
//  if (command(String("M575 ") + String(1000000)))
//    Serial.begin(1000000);
}

void handleAsyncMessage(const String& line)
{
  // TODO
}

void update()
{
  while(Serial.available() > 0)
    update(Serial.read());
}

void handleLine(const String& line)
{
  if (line == "ok")
  {
    if (completionCallback)
    {
      if (completion == Completion::Indeterminate)
        completion = Completion::Success;

      completionCallback(line, completion, completionContext);
      completion = Completion::Indeterminate;
    }
  }
  else if (completionCallback)
  {
    if (line.startsWith("Error:"))
      completion = Completion::Error;

    completionCallback(line, Completion::Indeterminate, completionContext);
  }
  else
  {
    handleAsyncMessage(line);
  }
}

void update(int c)
{
  constexpr size_t kMaxLength = 96;
  static char line[kMaxLength];
  static size_t lineLen = 0;

  if (c == -1)
    return;

  if (c == '\r' || c == '\n')
  {
    line[lineLen++] = 0;
    handleLine(line);
    lineLen = 0;
  }
  else
  {
    line[lineLen++] = c;
  }
}

void lcdMessage(const char* s)
{
  command("M117 ", s);
}

void setHandler(AsyncCompletion* complt, void* context)
{
  completionCallback = complt;
  completionContext = context;
}

bool command(const char* s)
{
  return command(s, nullptr);
}

bool command(const char* s, const char* arg)
{
  Serial.print(s);
  if (arg)
    Serial.println(arg);
  auto commandCompletion = [](String line, Completion complete, void* context)
  {
    *static_cast<Completion*>(context) = complete;
  };

  Completion complete;
  setHandler(commandCompletion, &complete);
  while (complete == Completion::Indeterminate)
    update();
  setHandler(nullptr, nullptr);
  return complete == Completion::Success;
}

}
