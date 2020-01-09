# i3Wifi

Minimal ESP8266 arduino sketch to function as Wifi adapter for Wanhao Duplicator i3 Mini ESP-01 port.

Mainboard: i3Mini V1.3 WH
WIFI (J1) pinout: (near board edge) `GND` `RX` `TX` `VCC` (5v)

Untested.

Early performance test, no flow control, no SD write (Wifi -> UART only)

    6441899b/100s = 64.41899 kBps

Required Marlin configuration:

`SERIAL_XON_XOFF`
`BAUD_RATE_GCODE`
