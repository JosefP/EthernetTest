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
unsigned long timeToCheckContent;
unsigned long timeToCheckEthernet;


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
  timeToCheckEthernet = millis();
  timeToCheckEthernetStop = maxMillis;
  timeToCheckContent = maxMillis;
}

int stopCounter = 0;
File contentFile;
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


  if (millis() > timeToCheckEthernet) {
    // listen for incoming clients
    if (!client || client.isStoped()) {
      //Serial.print("CKC:");
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
                     File webFile = SD.open(filename);
                     if (webFile) {
                      registerContent(webFile);                   
                     }
                     break;
                  } else {
                    Serial.println("Doesnt exist");
                      if (strlen(filename) < 2) {
                         File webFile = SD.open("index.htm");
                         if (webFile) {
                          registerContent(webFile);
                         } else {
                          code404(client);
                          registerStop();
                          break;
                        }
                      } else {
                        code404(client);
                        registerStop();
                        break;
                      }
                  }
          }
        }
        // give the web browser time to receive the data
       
        //Serial.print("5:");
        //Serial.println(millis() - milli);
       
      }
    }
  }
   // close the connection:

    if (millis() > timeToCheckContent) {
      //Serial.print("20:");
     if(contentFile.available()) {
      //Serial.print("21:");
      unsigned char buff[128];
      //Serial.print("22:");
        int wr = contentFile.read(buff,128);                        
        //Serial.print("23:");
         client.write(buff,wr);
         //Serial.print("24:");
     } else {
      //Serial.print("25:");
      contentFile.close();
      //Serial.print("26:");
      timeToCheckContent = maxMillis;
      registerStop();
     }
    }
    
    if (millis() > timeToCheckEthernetStop) {
      //Serial.print("7:");
      bool isStoped = client.checkStop();
      if (isStoped) {
        Serial.print("StopCounter:");
        Serial.println(stopCounter);
        // Turn off ethernet stop checking
        timeToCheckEthernetStop = maxMillis;
        timeToCheckEthernet = millis();
        //Serial.println("client disconnected");
        //Serial.print(timeToCheckEthernetStop);
      } else {
        //Serial.print("9:");
        stopCounter++;
        timeToCheckEthernetStop = millis() + 1;
      }
    }
}

void registerStop() {
  stopCounter = 0;
  client.beginStop();
  timeToCheckEthernetStop = millis();
}


void registerContent(File content) {
  contentFile = content;
  timeToCheckEthernet = maxMillis;
  timeToCheckContent = millis();
}

void code200(EthernetClient client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println("Last-Modified: Wed, 28 Oct 2015 23:06:49 GMT");

    client.println();
}
//If-Modified-Since: Wed, 28 Oct 2015 23:06:49 GMT
void code404(EthernetClient client) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<html><head><title>404 - Not Found</title></head><body><h1>404 - Not Found</h1></body></html>");
}

char* processFile( char clientline[255]) {
   char *filename;
   filename = clientline + 5;
  (strstr(clientline, " HTTP"))[0] = 0;
  return filename;
}

