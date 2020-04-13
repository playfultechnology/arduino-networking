/*
 * ESP-01 WebServer
 * Demonstrates how to create functionality that can be controlled over the internet
 * via an exposed web interface
*/

// INCLUDES
// "WiFiEsp" from Arduino IDE Library Manager
#include "WiFiEsp.h"
// Arduino HTTP Client" from Arduino IDE Library Manager
#include <ArduinoHttpClient.h>
// If running on a device that only has one hardware serial connection (i.e. UNO/Nano), 
// emulate a secondary one using software emulation.
#ifndef HAVE_HWSERIAL1
  #include "SoftwareSerial.h"
  SoftwareSerial Serial1(2, 3); // RX,TX 
#endif

// CONSTANTS
// The SSID of the Wi-Fi network to join
const char ssid[] = "VodafoneConnect53686628";
// Password required to join network
const char password[] = "8p2ty6329x2mk6v";
// Port on which server will listen to requests
const uint16_t port = 2002;
// This pin will be driven HIGH by the user's browser
const byte relayPin = 7;

// GLOBALS
// Use a ring buffer to store the HTTP request
RingBuffer buf(16);
// This buffer will store the last x characters of the HTTP request in a rolling ring buffer.
// Why not store the whole client request? Well...
// If you were to visit the following simple URL in your browser:
// http://192.168.1.18/
// The request as received by the Arduino server would actually look something like:
// GET / HTTP/1.1
// Host: 192.168.1.18
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
// Accept-Language: en-GB,en;q=0.5
// Accept-Encoding: gzip, deflate
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// (330 chars)
// The only part of that request we really want in this case is the resource being requested,
// (the very top line), and, if a form is being submitted, the message body (the very bottom).
// We can ignore the rest, so we'll just stream the response through a ring buffer only picking
// out the parts we are interested in.
// The server object, and the port on which to start the server listening
WiFiEspServer server(80);
// We don't strictly *need* a client object to simply respond to incoming requests, but we'll
// use it to create an outoing request to the ident.me page to determine our WAN address
WiFiEspClient client;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  // Initialize serial connection to ESP module
  Serial1.begin(9600);
  // Initialize ESP module
  WiFi.init(&Serial1);

  // Attempt to connect to WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print config to serial monitor
  Serial.print(F("SSID: "));
  Serial.println(ssid);
  Serial.print(F("LAN IP: "));
  Serial.println(WiFi.localIP());
  
  // Retrieve our WAN address from the ident.me service
  HttpClient httpClient = HttpClient(client, "176.58.123.25", 80);
  httpClient.beginRequest();
  httpClient.get("/");
  httpClient.endRequest();
  // Read the status code and body of the response
  int httpCode = httpClient.responseStatusCode();
  if(httpCode > 0) {
    String response = httpClient.responseBody();
    Serial.print(F("WAN IP: "));
    Serial.println(response);
  }
  // Start the web server
  server.begin();
  // Initialise the relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  // Wait a little for server to initialise before starting main program loop
  delay(2000);
}

void loop() {
  // Listen for incoming clients
  WiFiEspClient client = server.available();
  if (client) {
    Serial.println(F("New client connected"));
    // Initialise the buffer
    buf.init();
    // If the client is connected
    while (client.connected()) {
      // There's some data to process
      if (client.available()) {
        // Read the next character
        char c = client.read();
        // Write it to the serial monitor for debugging
        Serial.write(c);
        // Add it on to the ring buffer
        buf.push(c);
        // Check what the user's browser was requesting
        if (buf.endsWith("GET /H")) {
          Serial.println(F(" - Activating relay"));
          digitalWrite(relayPin, HIGH);
        }
        else if (buf.endsWith("GET /L")) {
          Serial.println((" - Activating relay"));
          digitalWrite(relayPin, LOW);
        }
        else if (buf.endsWith("GET /?p=1234")) {
          digitalWrite(relayPin, HIGH);
          delay(500);
          digitalWrite(relayPin, LOW);
          Serial.println((" - CORRECT PASSWORD ENTERED!"));
        }
        // Two newline characters in a row indicates the end of the HTTP request
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
