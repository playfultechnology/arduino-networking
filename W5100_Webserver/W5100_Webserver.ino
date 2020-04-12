/*
 * W5100 WebServer
 *
 * Demonstrates how to create functionality that can be controlled over the internet
 * via an exposed web interface
*/

// INCLUDES
// "Ethernet" library included with Arduino IDE
#include <Ethernet.h>
#include "Ringbuffer.h"

// CONSTANTS
const byte mac[6] = {0xab, 0xcd, 0xef, 0x00, 0x00, 0x00};
const IPAddress ip(192, 168, 1, 41);
const uint16_t port = 80;

// GLOBALS
// Use a ring buffer to store rolling 8 chars of HTTP request
RingBuffer buf(16);
// The server object, and the port on which to start the server listening
EthernetServer server(port);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  // Connect to the network
  // Note that you can leave IP blank to have it assigned by DHCP, but that makes Arduino sketch occupy more memory
  Ethernet.begin(mac, ip);
  Serial.print(F("LAN IP: "));
  Serial.println(Ethernet.localIP());
  // Start the server
  server.begin();
  // Wait a little for server to initialise before starting main program loop
  delay(2000);
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println(F("New client connected"));
    buf.init();
    while (client.connected()) {
      // There's some data to process
      if (client.available()) {
        // Read the next character
        char c = client.read();
        // Write it to the serial monitor for debugging
        Serial.write(c);
        // Add it on to the ring buffer
        buf.push(c);
        if (buf.endsWith("GET /H")) {
          Serial.println(F(" - Turning LED on"));
          digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        }
        else if (buf.endsWith("GET /L")) {
          Serial.println((" - Turning LED off"));
          digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        }
        else if (buf.endsWith("GET /?p=1234")) {
          Serial.println((" - CORRECT PASSWORD ENTERED!"));
        }
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        else if (buf.endsWith("\r\n\r\n")) {
          Serial.println(F("Sending response"));
          // Send a HTTP response header
          // Use \r\n instead of println statements increases speed
          client.print(F("HTTP/1.1 200 OK\r\n"));
          client.print(F("Content-type:text/html\r\n"));
          client.print(F("Connection: close\r\n\r\n"));
          // Now send the content of the webpage
          client.print(F("<!DOCTYPE HTML>\r\n"));
          client.print(F("<html>\r\n"));
          client.print(F("<h1>Escape Room Controller</h1>\r\n"));
          /*
           * If desired, you could display readings on the webpage , like this 
           * client.print(analogRead(A0));
           */         
          client.print(F("<form action='/' method='GET'>"));
          client.print(F("Password: <input type='text' name='p' value='' size='4' maxlength='4'>"));
          client.print(F("<input type='submit' value='submit'>"));
          client.print(F("</form>"));
          client.print(F("Click <a href=\"/H\">here</a> turn the LED on<br>"));
          client.print(F("Click <a href=\"/L\">here</a> turn the LED off<br>"));
          client.print(F("</html>"));
          break;
        }
      }
    }
    // Give the web browser time to receive the data
    delay(10);
    // Close the connection:
    client.stop();
    Serial.println(F("Client disconnected"));
  }
}
