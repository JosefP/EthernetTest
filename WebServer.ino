/*
  Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 */

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
const unsigned long maxMillis = 4294967295;
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF
};
IPAddress ip(192, 168, 0, 126);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
unsigned long timeToCheck ;
unsigned long timeToCheckEthernetStop;
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(250000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
 
 digitalWrite(10, HIGH);
  if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
  }
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
 
  timeToCheck = millis();
  timeToCheckEthernetStop = maxMillis;
}

int stopCounter = 0;
File webFile;
EthernetClient client;
void loop() {

  if (millis() > timeToCheck) {
    int missed = millis() - timeToCheck;
    if (missed > 2) {
      Serial.print("Missed:");
      Serial.println(missed);
    }
    timeToCheck = millis() + 10;
  }


  
  // listen for incoming clients
  if (!client || client.isStoped()) {
    client = server.available();
    int milli = millis();
  
    if (client) {
      //Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = true;
      while (client.connected()) {
        if (client.available()) {
          milli = millis();
          int size = client.available();
          unsigned char buff[size];
          int r = client.read(buff,size);
          char* filename = processFile((char*)buff);
          Serial.print("Requested: ");
          Serial.println(filename);
          if (SD.exists(filename)) {
                  Serial.println("Exist");
                   code200(client);
                   webFile = SD.open(filename);
                   if (webFile) {
                       while(webFile.available()) {
                        unsigned char buff[128];
                          int wr = webFile.read(buff,128);                        
                           client.write(buff,wr);
                       }
                       webFile.close();
                   }
                   break;
                } else {
                  Serial.println("Doesnt exist");
                    if (strlen(filename) < 2) {
                      webFile = SD.open("index.htm");
                       if (webFile) {
                           while(webFile.available()) {
                               client.write(webFile.read());
                           }
                           webFile.close();
                       } else {
                        Serial.println("404");
                        client.println("HTTP/1.1 404 Not Found");
                        client.println("Content-Type: text/html");
                        client.println("Connection: close");
                        client.println();
                        client.println("<html><head><title>404 - Not Found</title></head><body><h1>404 - Not Found</h1></body></html>");
                        break;
                      }
                    } else {
                      Serial.println("404");
                      client.println("HTTP/1.1 404 Not Found");
                      client.println("Content-Type: text/html");
                      client.println("Connection: close");
                      client.println();
                      client.println("<html><head><title>404 - Not Found</title></head><body><h1>404 - Not Found</h1></body></html>");
                      break;
                    }
                }
        }
      }
      // give the web browser time to receive the data
     
      //Serial.print("5:");
      //Serial.println(millis() - milli);
      client.beginStop();
      stopCounter = 0;
      //Serial.print("6:");
      //Serial.println(millis() - milli);
      timeToCheckEthernetStop = millis();
    }
  }
   // close the connection:
   

    if (millis() > timeToCheckEthernetStop) {
      //Serial.print("7:");
      bool isStoped = client.checkStop();
      if (isStoped) {
        Serial.print("StopCounter:");
        Serial.println(stopCounter);
        // Turn off ethernet stop checking
        timeToCheckEthernetStop = maxMillis;
        //Serial.println("client disconnected");
        //Serial.print(timeToCheckEthernetStop);
      } else {
        //Serial.print("9:");
        stopCounter++;
        timeToCheckEthernetStop = millis() + 1;
      }
    }
}

void code200(EthernetClient client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
}

char* processFile( char clientline[255]) {
   char *filename;
   filename = clientline + 5;
  (strstr(clientline, " HTTP"))[0] = 0;
  return filename;
}

