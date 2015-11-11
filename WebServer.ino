

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>


//------------ Check variables ------------------
unsigned long timeToCheck ;
unsigned long timeToCheckEthernetStop;
unsigned long timeToCheckContent;
unsigned long timeToCheckEthernet;
unsigned long timeToCheckGetDatePaths;
unsigned long timeToCheckGetDateSumSizes;
unsigned long timeToCheckGetDatePathsForMonths;
unsigned long timeToCheckGetDateSumSizesForMonths;
unsigned long timeToCheckGetDatePathsForDays;

//------------- Working variables -----------------
const unsigned long maxMillis = 4294967295;
const uint8_t daysArray [] PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const uint8_t dowArray[] PROGMEM = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
int lastFreeRam = 10000;
char* getDatePaths[100];
int getDatePathsCount;
int getDatePathsPosition;
long getDateSize;
char* previousYear;
File getDateSumSizesFile;
char* getDateSumSizesEndsWith;
long getDateSumSizesSize;
int stopCounter = 0;
File contentFile;
EthernetClient client;
EthernetServer server(80);

//------------- Settings --------------
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF
};
IPAddress ip(192, 168, 0, 126);

char* dataFilesExtension = (char *)".BIN";
char* dataRootDirectory = (char *)"/Data/";
char* queryStringDelimiter = (char *)"|";
char* pathDelimiter = (char *)"/";
char* dateDelimiter = (char *)"-";
char* yearPathDelimiter = (char *)",";





void setup() {
  Serial.begin(250000);

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

  planCheckImmediately(&timeToCheck);
  planCheckImmediately(&timeToCheckEthernet);
  disableCheck(&timeToCheckEthernetStop);
  disableCheck(&timeToCheckContent);
  disableCheck(&timeToCheckGetDatePaths);
  previousYear =(char *)"";
  disableCheck(&timeToCheckGetDateSumSizes);
  disableCheck(&timeToCheckGetDatePathsForMonths);
  disableCheck(&timeToCheckGetDateSumSizesForMonths);
  disableCheck(&timeToCheckGetDatePathsForDays);

}



void loop() {
 
  if (hasToCheck(&timeToCheck)) {
    check();
  }

  if (hasToCheck(&timeToCheckEthernet)) {
    checkEthernet();
  }

  if (hasToCheck(&timeToCheckContent)) {
    checkContent();
  }

  if (hasToCheck(&timeToCheckGetDatePathsForDays)) {
    checkGetDatePathsForDays();
  }

  if (hasToCheck(&timeToCheckGetDatePaths)) {
    checkGetDatePaths();
  }

  if (hasToCheck(&timeToCheckGetDateSumSizes)) {
    checkGetDateSumSizes();
  }

  if (hasToCheck(&timeToCheckGetDatePathsForMonths)) {
    checkGetDatePathsForMonths();
  }

  if (hasToCheck(&timeToCheckGetDateSumSizesForMonths)) {
    checkGetDateSumSizesForMonths();
  }

  //--------------KEEP LAST------------------------
  if (hasToCheck(&timeToCheckEthernetStop)) {
    checkEthernetStop();
  }
}

//---------------------- Process methods ----------------------

void processSDFile(EthernetClient client, char* filename) {
  File webFile = SD.open(filename);
  if (webFile) {
    Serial.println("Exist");
    code200(client, filename, webFile);
    registerContent(webFile);
  } else {
    Serial.println("Doesnt exist");
    if (strlen(filename) < 2) {
      File webFile = SD.open("index.htm");
      if (webFile) {
        registerContent(webFile);
      } else {
        code404(client);
        registerStop();
      }
    } else {
      code404(client);
      registerStop();
    }
  }
}

void processGetDays(EthernetClient client, char* filename) {
  code200(client, filename);

  char* parts[2];
  splitByDelim("?", filename, parts,2);
  char* months = parts[1];
  char* dates[50];
  int outlenDates = splitByDelim(queryStringDelimiter, months, dates,50);

  

  getDatePathsCount = 0;
  for (int j = 0; j < outlenDates; j++)
  {
    char* date = dates[j];

    if (date != "") {
      char subBuffer [3];
      uint8_t year = atoi(subString(date, 0, 2, subBuffer)) ;
      uint8_t month = atoi(subString(date, 2, 2, subBuffer)) ;
      uint8_t day = atoi(subString(date, 4, 2, subBuffer)) ;

      char path[24];
      char buffer [3];
      strcpy( path, dataRootDirectory );
      strcat( path, itoa(year, buffer, 10) );
      strcat( path, pathDelimiter );
      strcat( path, itoa(month, buffer, 10) );
      strcat( path,  pathDelimiter );
      strcat( path, itoa(day, buffer, 10) );
      strcat( path, dateDelimiter );
      strcat( path, itoa(month, buffer, 10) );
      strcat( path, dateDelimiter );
      strcat( path, itoa(year, buffer, 10) );
      strcat( path, dataFilesExtension );

      getDatePaths[getDatePathsCount] =  strdup(path);
      getDatePathsCount++;
      
    }

  }
  
  registerGetDateForDays();


  disableCheck(&timeToCheckEthernet);
}

void processGetMonths(EthernetClient client, char* filename) {
  code200(client, filename);
  char* parts[2];
  splitByDelim("?", filename, parts,2);
  char* months = parts[1];
  char* dates[50];
  int outlenDates = splitByDelim(queryStringDelimiter, months, dates,50);
  getDatePathsCount = 0;
  for (int j = 0; j < outlenDates; j++)
  {
    char* date = dates[j];
    if (date != "") {
      char subBuffer [3];
      uint8_t year = atoi(subString(date, 0, 2, subBuffer)) ;
      uint8_t month = atoi(subString(date, 2, 2, subBuffer)) ;

      char path[12];
      char buffer [3];
      strcpy( path, dataRootDirectory );
      strcat( path, itoa(year, buffer, 10) );
      strcat( path, pathDelimiter );
      strcat( path, itoa(month, buffer, 10) );
      strcat( path,  pathDelimiter );

      getDatePaths[getDatePathsCount] =  strdup(path);
      getDatePathsCount++;
    }
  }
  registerGetDateForMonths();

  disableCheck(&timeToCheckEthernet);
}

void processGetYears(EthernetClient client, char* filename) {
  code200(client, filename);
  char* parts[2];
  splitByDelim("?", filename, parts,2);
  char* years = parts[1];
  char* dates[50];
  int outlenDates = splitByDelim(queryStringDelimiter, years, dates,50);
  char* filesArray[50];
  int outlen = listDir(dataRootDirectory, true, filesArray);
  getDatePathsCount = 0;
  for (int i = 0; i < outlen; i++) {
    char* yearDirectory = filesArray[i];
    long size = 0;
    const uint8_t yearDirectoryInt = atoi(yearDirectory);
    
    for (int j = 0; j < outlenDates; j++)
    {

      char* date = dates[j];
      if (date != "") {
        char subBuffer [3];
        uint8_t yearVar = atoi(subString(date, 0, 2, subBuffer)) + yearDirectoryInt;
        uint8_t monthVar = atoi(subString(date, 2, 2, subBuffer)) ;
        char filePath[18];
        char buffer [3];
        
        strcpy( filePath, itoa(yearDirectoryInt, buffer, 10) );
        strcat( filePath, pathDelimiter );
        strcat( filePath, itoa(yearDirectoryInt + 1, buffer, 10) );
        strcat( filePath, yearPathDelimiter );
        strcat( filePath, dataRootDirectory );
        strcat( filePath, itoa(yearVar, buffer, 10) );
        strcat( filePath, pathDelimiter );
        strcat( filePath, itoa(monthVar, buffer, 10) );
        strcat( filePath,  pathDelimiter );

        getDatePaths[getDatePathsCount] = strdup(filePath);
        getDatePathsCount++;

      }
    }
  }

  registerGetDate();
  disableCheck(&timeToCheckEthernet);
}


//---------------- Check methods -------------
void check() {
  int missed = millis() - timeToCheck;
  if (missed > 50) {
    Serial.print("Missed:");
    Serial.println(missed);
  }

  if (freeRam() < lastFreeRam) {
    Serial.print("freeMemory(MI)=");
    Serial.println(freeRam());
    lastFreeRam = freeRam();
  }
  planCheck(&timeToCheck, 10);
}

void checkEthernet() {
  // listen for incoming clients
  if (!client || client.isStoped()) {
    client = server.available();
    int milli = millis();

    if (client) {
      boolean currentLineIsBlank = true;
      while (client.connected()) {
        if (client.available()) {
          milli = millis();
          int size = client.available();
          unsigned char buff[size];
          int r = client.read(buff, size);
          char* filename = processFile((char*)buff);
          Serial.print("Requested:");
          Serial.println(filename);
          //GetYears.cshtml?0008|0009|0010|0011|0012|0101|0102|0103|0104|0105|0106|

          if (strstr(filename, "GetYears.cshtml?") != 0) {
            processGetYears(client, filename);
            break;
          }

          if (strstr(filename, "GetMonths.cshtml?") != 0) {
            processGetMonths(client, filename);
            break;
          }

          if (strstr(filename, "GetDays.cshtml?") != 0) {
            processGetDays(client, filename);
            break;
          }
          processSDFile(client, filename);
          break;
        }
      }
    }
  }
}

void checkContent() {
  if (contentFile.available()) {
    char buff[1024];
    int wr = contentFile.read(buff, 1024);
    client.write(buff, wr);
  } else {
    contentFile.close();
    disableCheck(&timeToCheckContent);
    registerStop();
  }
}

void checkGetDatePathsForDays() {
  if (getDatePathsPosition < getDatePathsCount) {
    char* getDatePath = getDatePaths[getDatePathsPosition];
    long getDatePathSize = getSize(getDatePath, dataFilesExtension);
    long halfSize = getDatePathSize / 2;
    byte bytes2[2] ;
    bytes2[0] =  halfSize;
    bytes2[1] =  halfSize >> 8;
    client.write(bytes2, 2);
    planCheckImmediately(&timeToCheckGetDatePathsForDays);
    getDatePathsPosition++;
  } else {
    getDatePathsPosition = 0;
    disableCheck(&timeToCheckGetDatePathsForDays);
    registerStop();
  }
}

void checkGetDatePaths() {
  if (getDatePathsPosition < getDatePathsCount) {
    char* getDatePath = getDatePaths[getDatePathsPosition];
    getDateSize += getDateSumSizesSize;
    getDateSumSizesSize = 0;
    char* parts[2];
    splitByDelim(yearPathDelimiter, getDatePath, parts,2);
    char* months = parts[1];

    char* year = parts[0];
    char* path = parts[1];
  
    if (previousYear != "" && strcmp(year, previousYear) != 0 ) {
      splitByDelim(pathDelimiter, previousYear, parts,2);
      int firstYear = atoi(parts[0]);
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

    disableCheck(&timeToCheckGetDatePaths);
    registerGetDateSumSizes(path, dataFilesExtension);
  } else {
    getDateSize += getDateSumSizesSize;
    getDateSumSizesSize = 0;
    char* parts[2];
    splitByDelim(pathDelimiter, previousYear, parts,2);
    int firstYear = atoi(parts[0]);
    client.write(firstYear);
    long halfSize = getDateSize / 2;
    byte bytes2[4] ;
    bytes2[0] =  halfSize;
    bytes2[1] =  halfSize >> 8;
    bytes2[2] =  halfSize >> 16;
    bytes2[3] =  halfSize >> 24;
    client.write(bytes2, 4);
    getDateSize = 0;
    previousYear = (char *)"";
    getDatePathsPosition = 0;
    disableCheck(&timeToCheckGetDatePaths);
    registerStop();
  }
}

void checkGetDateSumSizes() {
  File entry =  getDateSumSizesFile.openNextFile();
  if (!entry) {
    getDateSumSizesFile.close();
    getDatePathsPosition++;
    planCheck(&timeToCheckGetDatePaths, 2);
    disableCheck(&timeToCheckGetDateSumSizes);
  }
  else if (entry.isDirectory()) {
    entry.close();
  } else {

    if ( endsWith(entry.name(), getDateSumSizesEndsWith)) {
      getDateSumSizesSize += entry.size();

    }
    planCheckImmediately(&timeToCheckGetDateSumSizes);
    entry.close();
  }
}

void checkGetDatePathsForMonths() {
  if (getDatePathsPosition < getDatePathsCount) {
    char* getDatePath = getDatePaths[getDatePathsPosition];
    if (getDatePathsPosition > 0) {
      getDateSize = getDateSumSizesSize;
      long halfSize = getDateSize / 2;
      byte bytes2[4] ;
      bytes2[0] =  halfSize;
      bytes2[1] =  halfSize >> 8;
      bytes2[2] =  halfSize >> 16;
      bytes2[3] =  halfSize >> 24;
      client.write(bytes2, 4);
    }
    getDateSumSizesSize = 0;
    char* path = getDatePath;
    disableCheck(&timeToCheckGetDatePathsForMonths);
    registerGetDateSumSizesForMonths(path, dataFilesExtension);
  } else {
    getDateSize = getDateSumSizesSize;
    getDateSumSizesSize = 0;
    long halfSize = getDateSize / 2;
    byte bytes2[4] ;
    bytes2[0] =  halfSize;
    bytes2[1] =  halfSize >> 8;
    bytes2[2] =  halfSize >> 16;
    bytes2[3] =  halfSize >> 24;
    client.write(bytes2, 4);
    getDateSize = 0;
    previousYear= (char *)"";
    getDatePathsPosition = 0;
    disableCheck(&timeToCheckGetDatePathsForMonths);
    registerStop();
  }
}

void checkGetDateSumSizesForMonths() {
  File entry =  getDateSumSizesFile.openNextFile();
  if (!entry) {
    getDateSumSizesFile.close();
    getDatePathsPosition++;
    planCheck(&timeToCheckGetDatePathsForMonths, 2);
    disableCheck(&timeToCheckGetDateSumSizesForMonths);
  }
  else if (entry.isDirectory()) {
    entry.close();
  } else {
    if ( endsWith(entry.name(), getDateSumSizesEndsWith)) {
      getDateSumSizesSize += entry.size();
    }
    planCheckImmediately(&timeToCheckGetDateSumSizesForMonths);
    entry.close();
  }
}

void checkEthernetStop() {
  bool isStoped = client.checkStop();
  if (isStoped) {
    Serial.print("StopCounter:");
    Serial.println(stopCounter);
    // Turn off ethernet stop checking
    disableCheck(&timeToCheckEthernetStop);
    planCheckImmediately(&timeToCheckEthernet);
  } else {
    stopCounter++;
    planCheck(&timeToCheckEthernetStop, 1);
  }
}


//---------------------- Register methods ----------------------
void registerGetDateSumSizes(char* path, char* endsWith) {
  long sizes = 0;
  File root = SD.open(path);
  getDateSumSizesEndsWith = endsWith;
  if (root) {
    getDateSumSizesFile = root;
    planCheckImmediately(&timeToCheckGetDateSumSizes);
  } else {
    getDatePathsPosition++;
    planCheck(&timeToCheckGetDatePaths, 2);
  }
}

void registerGetDateSumSizesForMonths(char* path, char* endsWith) {
  long sizes = 0;
  File root = SD.open(path);
  getDateSumSizesEndsWith = endsWith;
  if (root) {
    getDateSumSizesFile = root;
    planCheckImmediately(&timeToCheckGetDateSumSizesForMonths);
  } else {
    getDatePathsPosition++;
    planCheck(&timeToCheckGetDatePathsForMonths, 2);
  }
}

void registerGetDate() {
  planCheckImmediately(&timeToCheckGetDatePaths);
}


void registerGetDateForDays() {
  planCheck(&timeToCheckGetDatePathsForDays, 1);
}

void registerGetDateForMonths() {
  planCheck(&timeToCheckGetDatePathsForMonths, 1);
}

void registerStop() {
  stopCounter = 0;
  int pos = 0;
  for (int i = 0; i < getDatePathsCount; i++) {    
    free(getDatePaths[i]);
  }
  
    getDatePathsCount = 0;
  client.beginStop();
  planCheckImmediately(&timeToCheckEthernetStop);
}


void registerContent(File content) {

  contentFile = content;
  disableCheck(&timeToCheckEthernet);
  planCheckImmediately(&timeToCheckContent);
}

//------------------ Help methods ----------------------


bool hasToCheck(unsigned long* checkVariable) {
  return millis() > *checkVariable;
}

void planCheckImmediately(unsigned long* checkVariable) {
  *checkVariable = millis();
}

void planCheck(unsigned long* checkVariable, int checkAfterMillis) {
  *checkVariable = millis() + checkAfterMillis;
}

void disableCheck(unsigned long* checkVariable) {
  *checkVariable = maxMillis;
}


int listDir(char* path, bool onlyDir, char* filesArray[]) {
  File root = SD.open(path);
  int outLen = 0;
  if (root) {
    if (!root.isDirectory()) {
      return outLen;
    }
    
    while (true) {
      File f = root.openNextFile();
      if (! f) {
        root.rewindDirectory();
        root.close();
        break;
      }
      if ((onlyDir && f.isDirectory()) || !onlyDir) {
        if (outLen < 50){
          char* name = strdup(f.name());
          filesArray[outLen] = name;
        }
        outLen++;
      }
      f.close();
    }
    root.close();
  }
  return outLen;
}



long getSize(char* dir, char* endsWith) {
  File file = SD.open(dir);
  if (file) {
    long fileSize = file.size();
    file.close();
    return fileSize;
  }
  return 0;
}

long sumSizes(char* dir, char* endsWithValue) {
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
      if (entry.isDirectory()) {
      } else {
        if ( endsWith(entry.name(), endsWithValue)) {
          sizes += entry.size();
        }
      }
    }
    root.close();
  }
  return sizes;
}



void code304(EthernetClient client) {
  client.println("HTTP/1.1 304 Not Modified");
  client.println("Connection: close");
  client.println();
}

void code200(EthernetClient client, char* filename) {
  char  builder[250];
  strcpy( builder, "HTTP/1.1 200 OK");
  strcat( builder, "\n");
   Serial.println(filename);
  if (strstr(filename, ".htm") != 0) {
    strcat( builder, "Content-Type: text/html");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".css") != 0) {
    strcat( builder, "Content-Type: text/css");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".png") != 0) {
    strcat( builder, "Content-Type: image/png");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".jpg") != 0) {
    strcat( builder, "Content-Type: image/jpeg");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".gif") != 0) {
    strcat( builder, "Content-Type: image/gif");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".3gp") != 0) {
    strcat( builder, "Content-Type: video/mpeg");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".pdf") != 0) {
    strcat( builder, "Content-Type: application/pdf");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".js") != 0) {
    strcat( builder, "Content-Type: application/x-javascript");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".xml") != 0) {
    strcat( builder, "Content-Type: application/xml");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".bin") != 0) {
    strcat( builder, "text/plain");
    strcat( builder, "\n");
  }
  else {
    strcat( builder, "Content-Type: text/html");
    strcat( builder, "\n");
  }
  strcat( builder, "Connection: close");
  strcat( builder, "\n");
  strcat( builder, "\n");
  client.print(builder);
}


void code200(EthernetClient client, char* filename, File file) {
  char  builder[250];
  dir_t p;
  file.dirEntry(&p);
  uint16_t year = FAT_YEAR(p.lastWriteDate);
  uint16_t month = FAT_MONTH(p.lastWriteDate);
  uint16_t day = FAT_DAY(p.lastWriteDate);
  uint16_t hour = FAT_HOUR(p.lastWriteTime);
  uint16_t minute = FAT_MINUTE(p.lastWriteTime);
  uint16_t second = FAT_SECOND(p.lastWriteTime);
char timeString[30];
buildRFC822String(second, minute, hour, day, month, year,timeString);
Serial.println(filename);
  strcpy( builder, "HTTP/1.1 200 OK");
  strcat( builder, "\n");
  if (strstr(filename, ".htm") != 0) {
    strcat( builder, "Content-Type: text/html");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".css") != 0) {
    strcat( builder, "Content-Type: text/css");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".png") != 0) {
    strcat( builder, "Content-Type: image/png");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".jpg") != 0) {
    strcat( builder, "Content-Type: image/jpeg");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".gif") != 0) {
    strcat( builder, "Content-Type: image/gif");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".3gp") != 0) {
    strcat( builder, "Content-Type: video/mpeg");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".pdf") != 0) {
    strcat( builder, "Content-Type: application/pdf");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".js") != 0) {
    strcat( builder, "Content-Type: application/x-javascript");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".xml") != 0) {
    strcat( builder, "Content-Type: application/xml");
    strcat( builder, "\n");
  }
  else if (strstr(filename, ".bin") != 0) {
    strcat( builder, "Content-Type: text/plain");
    strcat( builder, "\n");
  }
  else {
    strcat( builder, "Content-Type: text/html");
    strcat( builder, "\n");
  }
  strcat( builder, "Connection: close");
  strcat( builder, "\n");

  strcat( builder, "Last-Modified: ");

  strcat( builder, timeString);

  strcat( builder, "\n");
  strcat( builder, "\n");
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

char* processFile( char clientline[600]) {
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
void buildRFC822String(uint16_t second, uint16_t minute, uint16_t hour, uint16_t dayOfMonth, uint16_t month, uint16_t year,char dateString[])
{
  int dayOfWeek = dow(year, month, dayOfMonth);
  switch (dayOfWeek) {
    case 1:
      strcpy(dateString, "Mon");
      break;
    case 2:
      strcpy(dateString, "Tue");
      break;
    case 3:
      strcpy(dateString, "Wed");
      break;
    case 4:
      strcpy(dateString, "Thu");
      break;
    case 5:
      strcpy(dateString, "Fri");
      break;
    case 6:
      strcpy(dateString, "Sat");
      break;
    case 7:
      strcpy(dateString, "Sun");
      break;

  }
  strcat(dateString, ", ");
  if (dayOfMonth < 10)
  {
    strcat(dateString, "0");
  }
  char buff[5];
  strcat(dateString, itoa(dayOfMonth, buff, 10));
  strcat(dateString, " ");
  switch (month) {
    case 1:
      strcat(dateString, "Jan");
      break;
    case 2:
      strcat(dateString, "Feb");
      break;
    case 3:
      strcat(dateString, "Mar");
      break;
    case 4:
      strcat(dateString, "Apr");
      break;
    case 5:
      strcat(dateString, "May");
      break;
    case 6:
      strcat(dateString, "Jun");
      break;
    case 7:
      strcat(dateString, "Jul");
      break;
    case 8:
      strcat(dateString, "Aug");
      break;
    case 9:
      strcat(dateString, "Sep");
      break;
    case 10:
      strcat(dateString, "Oct");
      break;
    case 11:
      strcat(dateString, "Nov");
      break;
    case 12:
      strcat(dateString, "Dec");
      break;

  }
  strcat(dateString, " ");
  strcat(dateString, itoa(year, buff, 10));
  strcat(dateString, " ");

  if (hour < 10)
  {
    strcat(dateString, "0");
  }
  strcat(dateString, itoa(hour, buff, 10));
  strcat(dateString, ":");
  if (minute < 10)
  {
    strcat(dateString, "0");
  }
  strcat(dateString, itoa(minute, buff, 10));
  strcat(dateString, ":");
  if (second < 10)
  {
    strcat(dateString, "0");
  }
  strcat(dateString, itoa(second, buff, 10));
  strcat(dateString, " GMT");

  
}

void freeargpointer(char** array, int count)
{
    int i;

    for ( i = 0; array[i]; i++ )
        free( array[i] );

    free( array );
}

int indexOf (char* base, char* str) {
  return strcspn(base, str);
}


int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

char* subString (const char* input, int offset, int len, char subbuff[])
{
  memcpy( subbuff, &input[offset], len );
  subbuff[len] = '\0';

  return subbuff;
}

void freeArray(char* items[] ,int count) {
   for (int i = 0; i < getDatePathsCount; i++) {    
    free(items[i]);
  }
}

int splitByDelim(const char* delimiter, char* toSplit, char* outputArray[],int arrayLength) {
  int outlen = 0;
  char *p = strtok(toSplit, delimiter);
  while (p) {
    if (outlen >= arrayLength) {
      break;
    }
    outputArray[outlen] = p;
    p = strtok(NULL, delimiter);
    outlen++;
  }

  return outlen;
}

bool endsWith (char* base, char* str) {
  int blen = strlen(base);
  int slen = strlen(str);
  return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}

