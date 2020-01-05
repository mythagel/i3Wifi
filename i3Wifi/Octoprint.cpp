#include "Octoprint.hpp"
#include "Marlin.hpp"
#include "GCode.hpp"
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

class Transmit
{
public:
    Transmit()
        : bl(thunk, this),
        cs(bl),
        m(cs)
    {}

    void process(uint8_t* buffer, size_t length)
    {
        m.process(buffer, length);
    }
    void flush()
    {
        uint8_t c = '\n';
        m.process(&c, 1);
    }
private:
    static void thunk(void* context)
    {
      static_cast<Transmit*>(context)->callback();
    }
    void callback()
    {
      // TODO move logic to Marlin class
      constexpr uint8_t xon = 17;
      constexpr uint8_t xoff = 19;
      auto outc = [](uint8_t ch)
      {
        Serial.write(ch);
        int c = Serial.read();
        if (c == xoff)
        {
          while (1)
          {
            c = Serial.read();
            if (c == xon)
              break;
            Marlin::update(c);
          }
          c = -1;
        }
        Marlin::update(c);
      };

      auto commandCompletion = [](String line, Marlin::Completion complete, void* context)
      {
        auto self = static_cast<Transmit*>(context);
        if (complete == Marlin::Completion::Success)
        {
          self->bl.buffer().pop();
        }
        else if (complete == Marlin::Completion::Error)
        {
          self->bl.buffer().reset_read();
        }
      };
      Marlin::setHandler(commandCompletion, this);
      bl.buffer().emit(outc);
      bl.buffer().pop_read();
    }

    GCode::BufferLine bl;
    GCode::Checksum cs;
    GCode::Minify m;
};

Transmit transmit;

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
    // TODO reset transmit, which will send M110N0
    //Marlin::command("M110 N0");
    Marlin::command("M28 ", upload.filename.c_str());
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    transmit.process(upload.buf, upload.currentSize);
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    transmit.flush();
    
    Marlin::command("M29");
    server.sendHeader("Location","/api/files/local/{todofilename}");
    server.send(200, "application/json", "todojson");

    bool print = server.arg("print") == "true";
    if (print)
    {
      Marlin::command("M23 ", upload.filename.c_str());
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
