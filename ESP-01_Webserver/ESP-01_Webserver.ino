/*
 * ESP-01 WebServer
 * Demonstrates how to create functionality that can be controlled over the internet
 * via an exposed web interface
*/

// INCLUDES
#include "WiFiEsp.h"
// Install the "Arduino HTTP Client" from Manage Libraries
#include <ArduinoHttpClient.h>
// If running on a device that only has one hardware serial connection )i.e. UNO/Nano), 
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
const uint16_t port = 8081;

// GLOBALS
// The type of HTTP request received (GET/POST)
enum Method { Undefined, GET, POST };
Method method = Method::Undefined;
// Keep track of the part of the request being processed
enum MessagePart { Header, Body, PostContent };
MessagePart messagePart = MessagePart::Header;
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
WiFiEspClient client;
int reqCount = 0;                // number of requests received

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
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("LAN IP: ");
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
    Serial.print("WAN IP: ");
    Serial.println(response);
  }
  
  // Start the web server on port 80
  server.begin();

  delay(2000);
}


void loop() {
  // listen for incoming clients
  WiFiEspClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    buf.init();

    // Initialise the variables we know about this connection
    method = Method::Undefined;
    messagePart = MessagePart::Header;
    int messageLength;

    // https://stackoverflow.com/questions/14944773/receiving-a-http-post-request-on-arduino
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      // There's some data to process
      if (client.available()) {
        // Read the next character
        char c = client.read();
        // Write it to the serial monitor for debugging
        Serial.write(c);
        // Add it on to the ring buffer
        buf.push(c);

        // Scan the contents of the ring buffer to identify the request method
        if(method == Method::Undefined) {
          if (buf.endsWith("GET /")) {
            method = Method::GET;
          }
          else if(buf.endsWith("POST /")) {
            method = Method::POST;
          }
        }

        // If we're dealing with a GET request
        if(method == Method::GET) {
          // Check to see if the client request was "GET /H" or "GET /L":
          if (buf.endsWith("GET /H")) {
            Serial.println("Turn led ON");
            digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
          }
          else if (buf.endsWith("GET /L")) {
            Serial.println("Turn led OFF");
            digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
          }
          // you got two newline characters in a row
          // that's the end of the HTTP request, so send a response
          else if (buf.endsWith("\r\n\r\n")) {
            Serial.println("Sending response to GET request");
            // send a standard http response header
            // use \r\n instead of many println statements to speedup data send
            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:text/html\r\n"));
            client.print(F("Connection: close\r\n"));
            client.println();
            client.print("<!DOCTYPE HTML>\r\n");
            client.print("<html>\r\n");
            client.print("<h1>Hello World!</h1>\r\n");
            client.print("Requests received: ");
            client.print(++reqCount);
            client.print("<br>\r\n");
            client.print("Analog input A0: ");
            client.print(analogRead(0));
            client.print("<br>\r\n");
            //form added to send data from browser and view received data in serial monitor         
            client.println("<FORM ACTION=\"/\" METHOD=\"POST\">");
            client.println("Name: <INPUT TYPE=\"TEXT\" NAME=\"Name\" VALUE=\"\" SIZE=\"25\" MAXLENGTH=\"50\"><BR>");
            client.println("Email: <INPUT TYPE=\"TEXT\" NAME=\"Email\" VALUE=\"\" SIZE=\"25\" MAXLENGTH=\"50\"><BR>");
            client.println("<INPUT TYPE=\"SUBMIT\" NAME=\"submit\" VALUE=\"Sign Me Up!\">");
            client.println("</FORM>");
            client.println("Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.println("Click <a href=\"/L\">here</a> turn the LED off<br>");
            client.print("</html>\r\n");
            break;
          }
        }

        // If we're dealing with a POST request
        else if(method==Method::POST) {

          // Scan the ring buffer to find out how long the message body is
          if (messagePart == MessagePart::Header && buf.endsWith("Content-Length: ")) {
            // Retrieve the next characters from the request to get the value of the content length
            char contentLength[8];
            for(int c=0; c<7; c++) {
              char n = client.read();
              buf.push(n);
              if(n=='\r') { break; }
              contentLength[c] = n;
              contentLength[c+1] = 0;
            }
            // Convert to an int
            messageLength = atoi(contentLength);
            Serial.print("messageLength:");
            Serial.println(messageLength);
          }
          // If we've got to the end of the header
          if (messagePart == MessagePart::Header && buf.endsWith("\r\n\r\n")) {
            messagePart = MessagePart::Body;
          }
          // If we're in the message body itself
          else if(messagePart == MessagePart::Body) {
            // Read the known number of characters in the message body
            for(int c=0; c<messageLength; c++) {
              char n = client.read();
              Serial.print(n);
              buf.push(n);
            }
            Serial.println("Sending response to POST request");
            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:text/html\r\n"));
            client.print(F("Connection: close\r\n"));
            client.println();
            client.print("<!DOCTYPE HTML>\r\n");
            client.print("<html>\r\n");
            client.print("<h1>Hello World!</h1>\r\n");
            client.print("Requests received: ");
            client.print(++reqCount);
            client.print("<br>\r\n");
            client.print("Analog input A0: ");
            client.print(analogRead(0));
            client.print("<br>\r\n");
            //form added to send data from browser and view received data in serial monitor         
            client.println("<FORM ACTION=\"/\" METHOD=\"POST\">");
            client.println("Name: <INPUT TYPE=\"TEXT\" NAME=\"Name\" VALUE=\"\" SIZE=\"25\" MAXLENGTH=\"50\"><BR>");
            client.println("Email: <INPUT TYPE=\"TEXT\" NAME=\"Email\" VALUE=\"\" SIZE=\"25\" MAXLENGTH=\"50\"><BR>");
            client.println("<INPUT TYPE=\"SUBMIT\" NAME=\"submit\" VALUE=\"Sign Me Up!\">");
            client.println("</FORM>");
            client.println("Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.println("Click <a href=\"/L\">here</a> turn the LED off<br>");
            client.print("</html>\r\n");
            break;
          }
        }
      }
    }
    
    // give the web browser time to receive the data
    delay(10);

    // close the connection:
    client.stop();
    Serial.println("Client disconnected");
  }
}
