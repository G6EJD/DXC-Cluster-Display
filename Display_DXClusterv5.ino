#include <WiFi.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>
#include "credentials.h"

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
// 2.8" is 320x240
// 4.0" is 480x320

WiFiClient client;

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

/* Network Settings */
char SVR_NAME[] = "dxc.hamserve.uk";
#define SVR_PORT 7300

int LED_BUILTIN = 2;
int yline, lastY, xoffset = 3;
int lineSpacing  = TFT_HEIGHT == 320 ? 15  : 20;   // 15 for 2.8", 20 for 4.0"
int Bandoffset   = TFT_HEIGHT == 320 ? 67  : 100;
int Modeoffset   = TFT_HEIGHT == 320 ? 310 : 445;
int TxTimeoffset = TFT_HEIGHT == 320 ? 213 : 320;
int screenWidth  = TFT_HEIGHT == 320 ? 320 : 480; // Define screen height, noting the display is set to Portrait mode
int screenHeight = TFT_WIDTH  == 240 ? 240  : 320; // Define screen width, noting the display is set to Portrait mode
int textSize     = TFT_HEIGHT == 320 ? 1 : 2;
int lineLength = 100; // limit of returned line length from server
String callSign, Band, Mode, TxTime, lastCallsign, lastBand, lastMode, lastTxTime;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(__FILE__);

  tft.init();
  tft.setRotation(3); // 1 and 3 are landscape modes
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  //  tft.setTextColor(TFT_YELLOW, TFT_BLUE); // Foreground: Yellow, Background: Blue
  tft.setTextSize(textSize);
  // Set the text wrap mode to true for horizontal wrapping and false for vertical wrapping
  tft.setTextWrap(false, false);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  yline = lineSpacing;
  lastY = yline;
}

void loop() {
  Serial.println("Connecting to server...");
  if (client.connect(SVR_NAME, SVR_PORT)) {
    Serial.println("DX Cluster - connected");
    tft.setTextColor(TFT_YELLOW, TFT_BLUE);  // Foreground: Yellow, Background: Blue
    tft.drawString("DX Cluster - connected", xoffset, 0);
    tft.setTextColor(TFT_YELLOW);  // Foreground: Yellow, Background: Blue
    Stream* serverpromt = &client;
    if (ReadStream(serverpromt, "login:")) {
      client.println(callsign);
      client.println("set/name " + (String)Name);
      client.println("set/qth " + (String)Locator);
      client.println("set/location " + (String)LatLon);
      // Read all the lines of the reply from server and print them to Serial
      while (ReadStream(&client, "Hello")) {
        while (true) {
          String line = client.readStringUntil('\r');
          if (ReadStream(&client, "DX de ")) {
            line.replace("DX de ", "");
            line.replace("     ", " ");
            line.replace("    ", " ");
            line.replace("   ", " ");
            line.replace("  ", " ");
            Serial.print(line);
            if (line.length() > 20) {
              tft.fillRect(0, yline, screenWidth, lineSpacing * 2, TFT_BLACK);
              tft.setCursor(xoffset, yline);
              line.trim();
              // https://doc-tft-espi.readthedocs.io/tft_espi/methods/drawstring/
              if (line.indexOf(":") > 0) callSign = line.substring(0, line.indexOf(":"));
              callSign.trim();
              line = line.substring(line.indexOf(":") + 1, lineLength);
              line.trim();              
              Band = GetBand(line); // Line starts with the band frequency
              Mode = ReportMode(line);
              TxTime = GetTxTime(line);
              tft.setTextColor(TFT_YELLOW);  // Foreground: Yellow, Background: Blue
              tft.fillRect(0, lastY, screenWidth, lineSpacing * 2, TFT_BLACK);
              if (callSign != "") {
                tft.drawString(lastCallsign, 0, lastY);
                tft.drawString(lastBand, Bandoffset, lastY);  //DX de EA4FME: 7074.6 EA3CFV IN80<>JN11 FT8 Sent: -01 Rcv 1311Z
                tft.drawString(lastTxTime, TxTimeoffset, lastY);  //DX de EA4FME: 7074.6 EA3CFV IN80<>JN11 FT8 Sent: -01 Rcv 1311Z
                tft.drawString(lastMode, Modeoffset - lastMode.length() * 4, lastY);  //DX de EA4FME: 7074.6 EA3CFV IN80<>JN11 FT8 Sent: -01 Rcv 1311Z
                tft.setTextColor(TFT_BLACK, TFT_GREEN);                         // Foreground: Yellow, Background: Blue
                tft.drawString(callSign, 0, yline);
                tft.drawString(Band, Bandoffset, yline);  //DX de EA4FME: 7074.6 EA3CFV IN80<>JN11 FT8 Sent: -01 Rcv 1311Z
                tft.drawString(TxTime, TxTimeoffset, yline);  //DX de EA4FME: 7074.6 EA3CFV IN80<>JN11 FT8 Sent: -01 Rcv 1311Z
                tft.drawString(Mode, Modeoffset - Mode.length() * 4, yline);  //DX de EA4FME: 7074.6 EA3CFV IN80<>JN11 FT8 Sent: -01 Rcv 1311Z
              }
              lastY = yline;
              lastCallsign = callSign;
              if (lastCallsign != ""){
                lastBand = Band;
                lastTxTime = TxTime;
                lastMode = Mode;
                yline = yline + lineSpacing;
              } 
              if (yline > screenHeight - lineSpacing) yline = lineSpacing;
            }
          }
        }
      }
      client.stop();
      Serial.println("Server disconnected");
    }  //  if prompt
    else {
      Serial.println("No prompt from the server.");
    }
  }  //  if connect
  else {
    Serial.println("Cannot connect to the server.");
  }
  delay(1000);
}

boolean ReadStream(Stream* stream, const char* target) {
  size_t index = 0;  // maximum target string length is 64k bytes!
  int c;
  boolean result = false;
  unsigned long timeBegin;
  delay(50);
  timeBegin = millis();
  while (true) {
    //  wait and read one byte
    while (!stream->available()) {
      if (millis() - timeBegin > 10000) {
        break;
      }
      delay(2);
    }
    if (stream->available()) {
      c = stream->read();
      Serial.write(c);
      if (c == target[index]) {
        index++;
        if (!target[index])
        // return true if all chars in the target match
        {
          result = true;
          break;
        }
      } else if (c >= 0) {
        index = 0;  // reset index if any char does not match
      } else        // timed-out for one byte
      {
        break;
      }
    } else  //  timed-out
    {
      break;
    }
  }
  return result;
}

String GetBand(String line){
  Band = " N/A";
  String freq = line.substring(0, line.indexOf(" "));
  if (freq.toInt() > 144000 & freq.toInt() < 146000) Band = "  2M";
  if (freq.toInt() > 70000  & freq.toInt() < 70500)  Band = "  4M";
  if (freq.toInt() > 50000  & freq.toInt() < 53000)  Band = "  6M";
  if (freq.toInt() > 28000  & freq.toInt() < 29700)  Band = " 10M";
  if (freq.toInt() > 24000  & freq.toInt() < 25000)  Band = " 12M";
  if (freq.toInt() > 21000  & freq.toInt() < 21350)  Band = " 15M";
  if (freq.toInt() > 18000  & freq.toInt() < 18350)  Band = " 17M";
  if (freq.toInt() > 14000  & freq.toInt() < 14350)  Band = " 20M";
  if (freq.toInt() > 10000  & freq.toInt() < 10200)  Band = " 30M";
  if (freq.toInt() > 7000   & freq.toInt() < 7200)   Band = " 40M";
  if (freq.toInt() > 5258   & freq.toInt() < 5407)   Band = " 60M";
  if (freq.toInt() > 3500   & freq.toInt() < 3800)   Band = " 80M";
  if (freq.toInt() > 1800   & freq.toInt() < 2000)   Band = "160M";
  if (freq.toInt() > 470    & freq.toInt() <  480)   Band = "600M";
  if (freq.toInt() < 470 || freq.toInt() > 146000 )  freq = "Error";
  return Band + " " + freq + " kHz";
}

String ReportMode(String line){
  String Mode = "";
  line.toUpperCase();
  if (line.indexOf(" FT8 ")  > 0)       Mode = " FT8";
  if (line.indexOf(" FT4 ")  > 0)       Mode = " FT4";
  if (line.indexOf(" FT2 ")  > 0)       Mode = " FT2";
  if (line.indexOf(" USB ")  > 0)       Mode = " USB";
  if (line.indexOf(" LSB ")  > 0)       Mode = " LSB";
  if (line.indexOf(" SSB ")  > 0)       Mode = " SSB";
  if (line.indexOf(" SES ")  > 0)       Mode = " SES";
  if (line.indexOf(" DME ")  > 0)       Mode = " DME";
  if (line.indexOf(" CW " )  > 0)       Mode = "  CW";
  if (line.indexOf(" FM " )  > 0)       Mode = "  FM";
  if (line.indexOf(" AM " )  > 0)       Mode = "  AM";
  if (line.indexOf(" RTTY ") > 0)       Mode = "RTTY";
  if (line.indexOf(" Feld Hell ") > 0)  Mode = "  FH";
  return Mode;
}

// 1254Z is a typical end of line 
String GetTxTime(String line){
  return line.substring(line.indexOf("Z") - 4, line.length() - 2);
}