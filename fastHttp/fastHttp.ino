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

#include <DNSServer.h>
#include <WiFiManager.h>    // tzapu WiFiManager

WiFiServer server(80);      // v2 Higher Bandwidth (no features)
WiFiClient client;          // single client

void setup(void)
{
  Serial.begin(1000000);
  
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  if (! wifiManager.autoConnect("i3Wifi"))
  {
    ESP.restart();
  }

  server.begin();
  server.setNoDelay(true);
  if (!MDNS.begin("i3Mini"))
    ESP.restart();
}

void accept()
{
  if (server.hasClient())
  {
    if (!client || !client.connected())
    {
      if(client)
        client.stop();
      client = server.available();
      return;
    }

    // drop connection
    WiFiClient dropClient = server.available();
    dropClient.stop();
  }
}

struct Octoprint
{
  enum class State
  {
    SkipWhitespace,

    Method,
    URI,
    Version,
    Header,
    Header_ContentLength,
    Header_Expect,
    Header_Skip,

    Content,

    Error,
  } state = State::Method;

  State nextState;
  enum Method
  {
    GET,
    POST
  } method;
  String uri;
  unsigned contentLength = 0;
  bool send100Continue = false;
  String accum;

  void reset()
  {
    state = State::Method;
    uri.clear();
    contentLength = 0;
    send100Continue = false;
    accum.clear();
  }

  void push(uint8_t* buf, size_t n, WiFiClient& client)
  {
    while (n)
    {
      uint8_t c = *buf;
      switch (state)
      {
        case State::SkipWhitespace:
        {
          if (c != ' ' && c != '\t')
          {
            state = nextState;
            continue;
          }
          break;
        }
        case State::Method:
        {
          if (isspace(c))
          {
            if (accum == "GET")
            {
              method = Method::GET;
              nextState = State::URI;
              state = State::SkipWhitespace;
            }
            else if (accum == "POST")
            {
              method = Method::POST;
              nextState = State::URI;
              state = State::SkipWhitespace;
            }
            else
            {
              state = State::Error;
            }
  
            accum.clear();
          }
          else
          {
            accum += (char)c;
          }
          break;
        }
        case State::URI:
        {
          if (isspace(c))
          {
            uri = accum;
            accum.clear();
            nextState = State::Version;
            state = State::SkipWhitespace;
          }
          else
          {
            accum += (char)c;
          }
          break;
        }
        case State::Version:
        {
          if (c == '\n')
            state = State::Header;
          break;
        }
        case State::Header:
        {
          if (c == ':')
          {
            if (accum == "Content-Length")
              nextState = State::Header_ContentLength;
            else if (accum == "Expect")
              nextState = State::Header_Expect;
            else
              nextState = State::Header_Skip;
            state = State::SkipWhitespace;
            accum.clear();
          }
          else if (c == '\n')
          {
            if (contentLength)
            {
              state = State::Content;
    
              if (send100Continue)
                client.write("HTTP/1.1 100 Continue\r\n\r\n", 25);
            }
            else
            {
              // TODO handle GET
            }
          }
          else
          {
            accum += (char)c;
          }
          break;
        }
        case State::Header_ContentLength:
        {
          if (c == '\n')
          {
            contentLength = atoi(accum.c_str());
            state = State::Header;
            accum.clear();
          }
          else if (c != '\r')
          {
            accum += (char)c;
          }
          break;
        }
        case State::Header_Expect:
        {
          if (c == '\n')
          {
            if (accum == "100-continue")
              send100Continue = true;
            state = State::Header;
            accum.clear();
          }
          else if (c != '\r')
          {
            accum += (char)c;
          }
          break;
        }
        case State::Header_Skip:
        {
          if (c == '\n')
            state = State::Header;
          break;
        }
        case State::Content:
        {
          pushContent(buf, n, client);
          return;
        }
        case State::Error:
        {
          client.write("HTTP/1.1 500 NOK\r\n\r\n", 20);
          client.stop();
          reset();
          break;
        }
      }

      --n;
      ++buf;
    }
  }

  void pushContent(uint8_t* buf, size_t n, WiFiClient& client)
  {
    if (contentLength == 0)
      state = State::Error;

    if (n > contentLength)
      n = contentLength;
    contentLength -= n;
    
    //Serial.write(buf, n);

    if (contentLength == 0)
    {
      client.write("HTTP/1.1 204 OK\r\n", 17);
      client.write("Connection: close\r\n", 19);
      client.write("\r\n", 2);
      reset();
      client.stop();
    }
  }
};

static Octoprint op;

void loop(void)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    MDNS.update();
    accept();

    if (client && client.connected())
    {
      int available = client.available();
      while (available)
      {
        constexpr int kBufferSize = 576*3;
        static uint8_t* buf = (uint8_t*)malloc(kBufferSize);
        if (available > kBufferSize)
          available = kBufferSize;
        int n = client.read(buf, available);
        op.push(buf, n, client);
        available = client.available();
      }
    }
  }
  delay(0);
}
