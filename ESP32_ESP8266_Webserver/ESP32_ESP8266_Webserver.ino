/*
 * ESP32 WebServer
 * 
 * Demonstrates how to create functionality that can be controlled over the internet
 * via an exposed web interface
 */

// INCLUDES
#if defined(ESP32)
  // For network connection
  #include <WiFi.h>
  // For operating as a server to deliver webpages to clients
  #include <WebServer.h>
  // For retrieving webpages from the internet
  #include <HTTPClient.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>
#endif
#include <WiFiClient.h>

// For OLED display
#include <Wire.h>
#include "lcdgfx.h"

// CONSTANTS
const char* ssid = "VodafoneConnect53686628";
const char* password = "8p2ty6329x2mk6v";

// GLOBALS
#if defined(ESP32)
// The webserver object
WebServer server(8081);
#elif defined(ESP8266)
ESP8266WebServer server(8081);
#endif
// The OLED display. Using I2C there is no data pin, so specify -1
DisplaySSD1306_128x64_I2C display(-1);

// CALLBACKS
void handleRoot() {
  if(server.hasArg("value") && server.arg("value") != NULL) {
    // Display the submitted value on the OLED
    display.clear();
    display.printFixed(0, 8, server.arg("value").c_str(), STYLE_NORMAL);
    // Display the submitted value on the serial monitor
    Serial.println(server.arg("value").c_str());
  }
  server.send(200, "text/html", "<form action='/' method='POST'><input type='text' name='value'><input type='submit' value='Submit'/></form>");
}
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  // For debugging purposes
  Serial.begin(115200);

  // Initialise the I2C interface. On The Wemos ESP32 board, this is on pins 4 and 5
  Wire.begin(5, 4);
  // Initialise and clear the display
  display.begin();
  display.clear();
  // Specify the default font to use for all following text operations
  display.setFixedFont(ssd1306xled_font6x8);

  // Initialise the network  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("SSID: ");
  Serial.println(ssid);
  display.printFixed(0, 8, "SSID:", STYLE_NORMAL);
  display.printFixed(30, 8, ssid, STYLE_NORMAL);
  
  Serial.print("LAN IP: ");
  Serial.println(WiFi.localIP());
  display.printFixed(0, 24, "LAN IP:", STYLE_NORMAL);
  display.printFixed(42, 24, WiFi.localIP().toString().c_str(), STYLE_NORMAL);

  // Retrieve our WAN address from the ident.me service
  HTTPClient http;
  http.begin("http://ident.me/"); 
  int httpCode = http.GET();
  if(httpCode > 0) {
    String response = http.getString();
    Serial.print("WAN IP: ");
    Serial.println(response);
    display.printFixed(0, 32, "WAN IP:", STYLE_NORMAL);
    display.printFixed(42, 32, response.c_str(), STYLE_NORMAL);
    
  }
  http.end(); //Free up any resources

  // Start the server
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  delay(2000);
}

void loop() {
  server.handleClient();
}
