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

#include <DNSServer.h>
#include <WiFiManager.h>    // tzapu WiFiManager

#include "Octoprint.hpp"
#include "Marlin.hpp"

ESP8266WebServer server(80);

void setup(void)
{
  // Swap RX/TX to prevent ESP8266 rom from disrupting Marlin
  Serial.begin(115200);
  //Serial.swap();

  Marlin::init();

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setAPCallback([&] (WiFiManager*) { Marlin::lcdMessage("Configure i3Wifi via AP"); });
  if (! wifiManager.autoConnect("i3Wifi"))
  {
    Marlin::lcdMessage("Wifi connection failed.");
    ESP.restart();
  }

  server.begin();
  if (!MDNS.begin("i3Mini"))
    ESP.restart();

  Octoprint::init();
}

void loop(void)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    MDNS.update();
    server.handleClient();
    Marlin::update();
  }
  delay(1);
}
