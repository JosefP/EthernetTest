

/*
  Web Server

  A simple web server that shows the value of the analog input pins.
  using an Arduino Wiznet Ethernet shield.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13
   Analog inputs attached to pins A0 through A5 (optional)

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
unsigned long timeToCheckGetDatePaths;
unsigned long timeToCheckGetDateSumSizes;


String getDatePaths[200];
int getDatePathsCount;
int getDatePathsPosition;
long getDateSize;
String previousYear;
File getDateSumSizesFile;
String getDateSumSizesEndsWith;
long getDateSumSizesSize;

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
  timeToCheckGetDatePaths = maxMillis;
  previousYear = "";
  timeToCheckGetDateSumSizes = maxMillis;
}

int stopCounter = 0;
File contentFile;
EthernetClient client;

const uint8_t daysArray [] PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const uint8_t dowArray[] PROGMEM = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };



void loop() {

  if (millis() > timeToCheck) {
    int missed = millis() - timeToCheck;
    if (missed > 20) {
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
            int r = client.read(buff, size);
            char* filename = processFile((char*)buff);
            Serial.print("Requested: ");
            Serial.println(filename);

            //GetYears.cshtml?0008|0009|0010|0011|0012|0101|0102|0103|0104|0105|0106|

            if (strstr(filename, "GetYears.cshtml?") != 0) {
              code200(client, filename);
              String years = String(filename).substring(String(filename).indexOf("?") + 1);
              String files[50];
              int outlen = listDir("/Data/", true, files);
              getDatePathsCount = 0;

              String dates[50];
              int outlenDates = splitByDelim("|", years, dates);
              for (int i = 0; i < outlen; i++) {
                String yearDirectory = files[i];
                Serial.println(yearDirectory);
                long size = 0;
                uint8_t yearDirectoryInt = yearDirectory.toInt();
                byte bytes[] = {yearDirectoryInt};

                for (int j = 0; j < outlenDates; j++)
                {

                  String date = dates[j];
                  if (date != "") {
                    uint8_t year = date.substring(0, 2).toInt() + yearDirectoryInt;
                    uint8_t month = date.substring(2, 4).toInt() ;

                    String filePath = "/Data/" + String(year) + "/" + String(month) + "/";
                    getDatePaths[getDatePathsCount] = String(yearDirectoryInt)+"/"+String(yearDirectoryInt+1) + "," + filePath;
                    getDatePathsCount++;

                  }
                }

              }



              registerGetDate();
              //client.println();
              timeToCheckEthernet = maxMillis;
              break;
            }

            if (strstr(filename, "GetMonths.cshtml?") != 0) {
              code200(client, filename);
              break;
            }

            if (strstr(filename, "GetDays.cshtml?") != 0) {
              code200(client, filename);
              break;
            }




            File webFile = SD.open(filename);
            if (webFile) {
              Serial.println("Exist");
              code200(client, filename, webFile);
              registerContent(webFile);
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
        //Serial.println(millsis() - milli);

      }
    }
  }
  // close the connection:

  if (millis() > timeToCheckContent) {
    //Serial.print("20:");
    if (contentFile.available()) {
      //Serial.print("21:");
      unsigned char buff[512];
      //Serial.print("22:");
      int wr = contentFile.read(buff, 512);
      //Serial.print("23:");
      client.write(buff, wr);
      //Serial.print("24:");
    } else {
      //Serial.print("25:");
      contentFile.close();
      //Serial.print("26:");
      timeToCheckContent = maxMillis;
      registerStop();
    }
  }


  if (millis() > timeToCheckGetDatePaths) {
    Serial.print("B1:");
    Serial.println(getDatePathsPosition);
    Serial.print("B2:");
    Serial.println(getDatePathsCount);

    if (getDatePathsPosition < getDatePathsCount) {
      String getDatePath = getDatePaths[getDatePathsPosition];
      Serial.print("B3:");
      Serial.println(getDatePath);
      getDateSize += getDateSumSizesSize;
      getDateSumSizesSize = 0;
      String year = String(getDatePath).substring(0, String(getDatePath).indexOf(","));
      Serial.print("B4:");
      Serial.println(year);
      String path = String(getDatePath).substring(String(getDatePath).indexOf(",") + 1);
      Serial.print("B5:");
      Serial.println(path);
      if (previousYear != "" && year != previousYear) {
        int firstYear = String(previousYear).substring(0, String(previousYear).indexOf("/")).toInt();
        client.write(firstYear);
        long halfSize = getDateSize / 2;
        byte bytes2[4] ;
        bytes2[0] =  halfSize;
        bytes2[1] =  halfSize >> 8;
        bytes2[2] =  halfSize >> 16;
        bytes2[3] =  halfSize >> 24;
        client.write(bytes2, 4);
        getDateSize = 0;
      }
      previousYear = year;
      //getDateSize = getDateSize + sumSizes(path,".TXT");
      timeToCheckGetDatePaths = maxMillis;
      registerGetDateSumSizes(path, ".TXT");


    } else {
      getDateSize += getDateSumSizesSize;
      getDateSumSizesSize = 0;
      int firstYear = String(previousYear).substring(0, String(previousYear).indexOf("/")).toInt();
      client.write(firstYear);
      long halfSize = getDateSize / 2;
      byte bytes2[4] ;
      bytes2[0] =  halfSize;
      bytes2[1] =  halfSize >> 8;
      bytes2[2] =  halfSize >> 16;
      bytes2[3] =  halfSize >> 24;
      client.write(bytes2, 4);
      getDateSize = 0;
      previousYear = "";
      getDatePathsPosition = 0;
      timeToCheckGetDatePaths = maxMillis;
      registerStop();
    }
  }


  if (millis() > timeToCheckGetDateSumSizes) {

    Serial.println("C1:");
    File entry =  getDateSumSizesFile.openNextFile();
    Serial.println("C2:");
    if (!entry) {
      Serial.println("C3:");
      entry.close();
      getDateSumSizesFile.close();
      getDatePathsPosition++;
      timeToCheckGetDatePaths = millis() + 2;
      timeToCheckGetDateSumSizes = maxMillis;
    }
    else if (entry.isDirectory()) {
      Serial.println("C4:");
    } else {
      Serial.println("C5:");
      if ( String(entry.name()).endsWith(getDateSumSizesEndsWith)) {
        getDateSumSizesSize += entry.size();
        Serial.println("C6:");
      }
      timeToCheckGetDateSumSizes = millis();
    }


  }

  //--------------KEEP LAST------------------------
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



void registerGetDateSumSizes(String path, String endsWith) {
  long sizes = 0;
  File root = SD.open(path);
  getDateSumSizesEndsWith = endsWith;
  if (root) {
    getDateSumSizesFile = root;
    timeToCheckGetDateSumSizes = millis();
  } else {
    getDatePathsPosition++;
    timeToCheckGetDatePaths = millis() + 2;
  }
}


void registerGetDate() {
  timeToCheckGetDatePaths = millis();


}

int listDir(String path, bool onlyDir, String files[50]) {
  File root = SD.open(path);
  int outLen = 0;
  if (root) {
    if (!root.isDirectory()) {
      return outLen;
    }
    int i = 0;
    while (true) {
      File f = root.openNextFile();
      if (! f) {
        root.rewindDirectory();
        break;
      }
      if ((onlyDir && f.isDirectory()) || !onlyDir) {
        if (i < 50) files[i] = f.name();
        i++;
      }
      f.close();
    }
    outLen = i;
    root.close();
  }
  return outLen;
}


int splitByDelim(String delimiter, String toSplit, String outputArray[50]) {
  int outlen = 0;
  size_t pos = 0;
  String token;
  int index = 0;
  while ((pos = toSplit.indexOf(delimiter)) != -1) {
    token = toSplit.substring(0, pos);
    outputArray[index] = token;
    toSplit.remove(0, pos + delimiter.length());
    index++;
  }
  outlen = index + 1;
  outputArray[index] = toSplit;
  return outlen;
}


long getSize(String dir, String endsWith) {
  File file = SD.open(dir);
  if (file) {
    long fileSize = file.size();
    file.close();
    return fileSize;
  }
  return 0;
}

long sumSizes(String dir, String endsWith) {
  long sizes = 0;
  File root = SD.open(dir);
  if (root) {
    while (true) {
      File entry =  root.openNextFile();
      if (! entry) {
        // no more files
        // return to the first file in the directory
        root.rewindDirectory();
        break;
      }
      //Serial.print(entry.name());
      if (entry.isDirectory()) {
      } else {
        if ( String(entry.name()).endsWith(endsWith)) {
          sizes += entry.size();
        }
      }
    }
    root.close();
  }
  return sizes;
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

void code304(EthernetClient client) {
  client.println("HTTP/1.1 304 Not Modified");
  client.println("Connection: close");
  client.println();
}

void code200(EthernetClient client, char* filename) {
  String builder = "";


  builder += "HTTP/1.1 200 OK";
  builder += "\n";
  if (strstr(filename, ".htm") != 0) {
    builder += "Content-Type: text/html";
    builder += "\n";
  }
  else if (strstr(filename, ".css") != 0) {
    builder += "Content-Type: text/css";
    builder += "\n";
  }
  else if (strstr(filename, ".png") != 0) {
    builder += "Content-Type: image/png";
    builder += "\n";
  }
  else if (strstr(filename, ".jpg") != 0) {
    builder += "Content-Type: image/jpeg";
    builder += "\n";
  }
  else if (strstr(filename, ".gif") != 0) {
    builder += "Content-Type: image/gif";
    builder += "\n";
  }
  else if (strstr(filename, ".3gp") != 0) {
    builder += "Content-Type: video/mpeg";
    builder += "\n";
  }
  else if (strstr(filename, ".pdf") != 0) {
    builder += "Content-Type: application/pdf";
    builder += "\n";
  }
  else if (strstr(filename, ".js") != 0) {
    builder += "Content-Type: application/x-javascript";
    builder += "\n";
  }
  else if (strstr(filename, ".xml") != 0) {
    builder += "Content-Type: application/xml";
    builder += "\n";
  }
  else {
    builder += "Content-Type: text/html";
    builder += "\n";
  }
  builder += "Connection: close";
  builder += "\n";
  builder += "\n";
  client.print(builder);
}


void code200(EthernetClient client, char* filename, File file) {
  String builder = "";
  dir_t p;
  file.dirEntry(&p);
  uint16_t year = FAT_YEAR(p.lastWriteDate);
  uint16_t month = FAT_MONTH(p.lastWriteDate);
  uint16_t day = FAT_DAY(p.lastWriteDate);
  uint16_t hour = FAT_HOUR(p.lastWriteTime);
  uint16_t minute = FAT_MINUTE(p.lastWriteTime);
  uint16_t second = FAT_SECOND(p.lastWriteTime);

  //Serial.print("Before:");
  //Serial.println(millis());
  String timeString = buildRFC822String(second, minute, hour, day, month, year);
  //Serial.print("After:");
  //Serial.println(millis());

  //Serial.println(timeString);


  builder += "HTTP/1.1 200 OK";
  builder += "\n";
  if (strstr(filename, ".htm") != 0) {
    builder += "Content-Type: text/html";
    builder += "\n";
  }
  else if (strstr(filename, ".css") != 0) {
    builder += "Content-Type: text/css";
    builder += "\n";
  }
  else if (strstr(filename, ".png") != 0) {
    builder += "Content-Type: image/png";
    builder += "\n";
  }
  else if (strstr(filename, ".jpg") != 0) {
    builder += "Content-Type: image/jpeg";
    builder += "\n";
  }
  else if (strstr(filename, ".gif") != 0) {
    builder += "Content-Type: image/gif";
    builder += "\n";
  }
  else if (strstr(filename, ".3gp") != 0) {
    builder += "Content-Type: video/mpeg";
    builder += "\n";
  }
  else if (strstr(filename, ".pdf") != 0) {
    builder += "Content-Type: application/pdf";
    builder += "\n";
  }
  else if (strstr(filename, ".js") != 0) {
    builder += "Content-Type: application/x-javascript";
    builder += "\n";
  }
  else if (strstr(filename, ".xml") != 0) {
    builder += "Content-Type: application/xml";
    builder += "\n";
  }
  else {
    builder += "Content-Type: text/html";
    builder += "\n";
  }
  builder += "Connection: close";
  builder += "\n";
  builder += "Last-Modified: " + timeString;
  builder += "\n";
  builder += "\n";
  client.print(builder);
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

uint8_t dow(uint16_t y, uint8_t m, uint8_t d)
{
  uint8_t dow;

  y -= m < 3;
  dow = ((y + y / 4 - y / 100 + y / 400 + pgm_read_byte(dowArray + (m - 1)) + d) % 7);

  if (dow == 0)
  {
    return 7;
  }

  return dow;
}

//Sun, 06 Nov 1994 08:49:37 GMT
String buildRFC822String(uint16_t second, uint16_t minute, uint16_t hour, uint16_t dayOfMonth, uint16_t month, uint16_t year)
{
  String dateString  = "";
  int dayOfWeek = dow(year, month, dayOfMonth);
  switch (dayOfWeek) {
    case 1:
      dateString += "Mon";
      break;
    case 2:
      dateString += "Tue";
      break;
    case 3:
      dateString += "Wed";
      break;
    case 4:
      dateString += "Thu";
      break;
    case 5:
      dateString += "Fri";
      break;
    case 6:
      dateString += "Sat";
      break;
    case 7:
      dateString += "Sun";
      break;

  }
  dateString += ", ";
  if (dayOfMonth < 10)
  {
    dateString += "0";
  }
  dateString += dayOfMonth;
  dateString += " ";
  switch (month) {
    case 1:
      dateString += "Jan";
      break;
    case 2:
      dateString += "Feb";
      break;
    case 3:
      dateString += "Mar";
      break;
    case 4:
      dateString += "Apr";
      break;
    case 5:
      dateString += "May";
      break;
    case 6:
      dateString += "Jun";
      break;
    case 7:
      dateString += "Jul";
      break;
    case 8:
      dateString += "Aug";
      break;
    case 9:
      dateString += "Sep";
      break;
    case 10:
      dateString += "Oct";
      break;
    case 11:
      dateString += "Nov";
      break;
    case 12:
      dateString += "Dec";
      break;

  }
  dateString += " ";
  dateString += year;
  dateString += " ";

  if (hour < 10)
  {
    dateString += "0";
  }
  dateString += hour;
  dateString += ":";
  if (minute < 10)
  {
    dateString += "0";
  }
  dateString += minute;
  dateString += ":";
  if (second < 10)
  {
    dateString += "0";
  }
  dateString += second;
  dateString += " GMT";

  return dateString;
}


