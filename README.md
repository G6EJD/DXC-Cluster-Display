# DXC-Cluster-Display
Reads data from a DX Cluster server and displays on a CYD Yellow Display

Uses the ESP32 Cheap Yellow Displays to display DX Cluster entries

The example works for either the 4.0" or 2.8" display modules.

Uses the TFT_eSPI display library.

Choose the User_Setup.h and be sure to repace the original library file with the relevant User_Setup.h file contents suited for the display chosen.

Set the processor type as ESP32 Dev Module in the Arduino Framework. Do-not use the CYD variant.

4.0" ![Architecture Diagram](https://m.media-amazon.com/images/I/71KNmj2Oq1L._AC_SL1500_.jpg)
2.8" ![Architecture Diagram](https://m.media-amazon.com/images/I/71Pb+sLyMyL._AC_SL1500_.jpg)
