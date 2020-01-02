#include "Octoprint.hpp"
#include "Marlin.hpp"
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void emitGCode(uint8_t* buf, size_t bufLen, bool last=false)
{
  constexpr size_t kMarlinMaxLength = 96;
  constexpr size_t kMaxLength = 128; // Marlin default line length + room for line & checksum
  static uint8_t line[kMaxLength];
  static size_t lineLen = 0;

  static size_t lineNumber = 0;
  static int checksum = 0;

  auto appendChar = [&](uint8_t c, bool internal = false)
  {
    if (internal || lineLen < kMarlinMaxLength)
    {
      line[lineLen++] = c;
      checksum = checksum ^ c;
    }
  };

  auto appendNumber = [&](unsigned n, bool internal = false)
  {
    auto s = String(n);
    for (auto c : s)
      appendChar(c, internal);
  };

  auto nextLine = [&]
  {
    lineLen = 0;
    checksum = 0;

    appendChar('N', true);
    appendNumber(++lineNumber, true);
    appendChar(' ', true);
  };

  auto emitLine = [&]
  {
    appendChar(' ', true);
    uint8_t cs = checksum & 0xFF;
    appendChar('*', true);
    appendNumber(cs, true);
    appendChar('\n', true);

    Marlin::write(line, lineLen);
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
    Marlin::command("M110 N0");
    Marlin::command(String("M28 ") + upload.filename);
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    emitGCode(upload.buf, upload.currentSize);
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    emitGCode(upload.buf, upload.currentSize, /*last*/true);
    
    Marlin::command("M29");
    server.sendHeader("Location","/api/files/local/{todofilename}");
    server.send(200, "application/json", "todojson");

    bool print = server.arg("print") == "true";
    if (print)
    {
      Marlin::command(String("M23 ") + upload.filename);
      Marlin::command("M24");
    }
  }
}

}

void init()
{
  MDNS.addService("octoprint", "tcp", 80);
  MDNS.addServiceTxt("octoprint", "tcp", "path", "/");
  MDNS.addServiceTxt("octoprint", "tcp", "api", "0.1");
  MDNS.addServiceTxt("octoprint", "tcp", "version", "0.1");

  server.onNotFound([]{ server.send(404, "text/plain", "Not Found"); });
  server.on("/api/version", HTTP_GET, Octoprint::API::Version);
  server.on("/api/files/local", HTTP_POST, [](){ server.send(200); }, Octoprint::API::Files);
}

}
