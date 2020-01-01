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
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);
constexpr const char* ssid = "";
constexpr const char* password = "";


void emitLines(uint8_t* buf, size_t bufLen, bool last=false)
{
  constexpr size_t kMaxLength = 128; // Marlin default line length + room for line & checksum
  static uint8_t line[kMaxLength];
  static size_t lineLen = 0;

  static size_t lineNumber = 0;
  static int checksum = 0;

  auto appendChar = [&](uint8_t c)
  {
    line[lineLen++] = c;
    checksum = checksum ^ c;
  };

  auto appendNumber = [&](unsigned n)
  {
    if (n == 0)
      appendChar('0');
    while (n)
    {
      appendChar('0' + (n%10));
      n /= 10;
    }
  };

  auto nextLine = [&]
  {
    lineLen = 0;
    checksum = 0;

    appendChar('N');
    appendNumber(++lineNumber);
    appendChar(' ');
  };

  auto emitLine = [&]
  {
    appendChar(' ');
    uint8_t cs = checksum & 0xFF;
    appendChar('*');
    appendNumber(cs);
    appendChar('\n');

    Serial.write(line, lineLen);
    nextLine();
  };

  if (lineLen == 0)
    nextLine();

  while(bufLen)
  {
    if (*buf == '\r' || *buf == '\n')
      emitLine();
    else
      appendChar(*buf);
    
    ++buf;
    --bufLen;
  }

  if (last)
  {
    emitLine();
    lineNumber = 0;
  }
}

namespace Octoprint
{
namespace API
{

void Version()
{
  server.send(200, "application/json", R"({
  "api": "0.1",
  "server": "0.1",
  "text": "OctoPrint (compatible) (miniPrint) 0.1"
})");

}

void Files()
{
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    Serial.println("M110 N0");
    Serial.print("M28 ");
    Serial.println(upload.filename);
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    emitLines(upload.buf, upload.currentSize);
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    emitLines(upload.buf, upload.currentSize, /*last*/true);
    
    Serial.println("M29");
    server.sendHeader("Location","/api/files/local/{todofilename}");
    server.send(200, "application/json", "todojson");

    bool print = server.arg("print") == "true";
    if (print) {
      Serial.print("M23 ");
      Serial.println(upload.filename);
      Serial.println("M24");
    }
  }
}

}
}

void setup(void)
{
  Serial.begin(250000);

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  static auto stationGotIP = WiFi.onStationModeGotIP([&](const WiFiEventStationModeGotIP&)
  {
    server.begin();
    
    if (!MDNS.begin("i3Mini"))
    {
      while (1) delay(1000);
    }

    MDNS.addService("octoprint", "tcp", 80);
    MDNS.addServiceTxt("octoprint", "tcp", "path", "/");
    MDNS.addServiceTxt("octoprint", "tcp", "api", "0.1");
    MDNS.addServiceTxt("octoprint", "tcp", "version", "0.1");
  });

  server.onNotFound([]{ server.send(404, "text/plain", "Not Found"); });
  server.on("/api/version", HTTP_GET, Octoprint::API::Version);
  server.on("/api/files/local", HTTP_POST, [](){ server.send(200); }, Octoprint::API::Files);
  
  WiFi.begin(ssid, password);
}

void loop(void)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    MDNS.update();
    server.handleClient();
  }
  else
  {
    delay(1000);
  }
}
