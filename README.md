# i3Wifi

Minimal ESP8266 arduino sketch to function as Wifi adapter for Wanhao Duplicator i3 Mini 'WIFI' UART port.

Mainboard: `i3Mini V1.3 WH`
WIFI (J1) pinout: (near board edge) `GND` `RX` `TX` `VCC` (5v)

Untested.

Early performance test, no flow control, no SD write (Wifi -> Protocol [1] -> UART only)

    6441899b/100s = 64.41899 kBps

Source gcode was 5850962b - protocol space overhead ~10.1%

Required Marlin configuration:

`SERIAL_XON_XOFF`

`BAUD_RATE_GCODE`

---

[1] Protocol stages:

* Minify - Strip whitespace, comments, & empty lines
* MinDigits - Strip trailing zeros from numbers - 3.4000 - 3.4
* Checksum - Add line number & checksum - Line number is capped at 9 to minimise protocol overhead
* RingBuffer - Buffered into a circular queue for transmission
