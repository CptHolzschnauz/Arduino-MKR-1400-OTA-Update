#include<SDU.h>
#include <MKRGSM.h>
#include <SPI.h>
#include <SD.h>

const char PINNUMBER[] = "";
// APN data
const char GPRS_APN[] = "";
const char GPRS_LOGIN[] = "";
const char GPRS_PASSWORD[] = "";


GSMClient client;
GPRS gprs;
GSM gsmAccess;

File myFile;

char server[] = "server.com";
char path[] = "/update-1400.bin";
int port = 80;
bool header = true;
int nl_count;
String currentLine;

void setup() {
  // initialize serial communications and wait for port to open:
  Serial.begin(2000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("Initialization failed!");
    while (1);
  }
  Serial.println("Initialization done.");

  Serial.println("Starting Arduino web client.");
  bool connected = false;
  while (!connected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }
  Serial.println("connecting...");

  if (client.connect(server, port)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.print("GET ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

  if (SD.exists("UPDATE.bin")) {
    Serial.println("UPDATE.bin exists.");
    // delete the file:
    Serial.println("Removing UPDATE.bin...");
    SD.remove("UPDATE.bin");
  }
  else {
    Serial.println("UPDATE.bin doesn't exist on the SD card. Open a new file..");
  }
  myFile = SD.open("UPDATE.bin", FILE_WRITE);
}
void loop()
{

  if (client.available()) {
    char c = client.read();

    if (header == true) {
      currentLine += c;
      if (c == '\n') {
        currentLine = "";
        nl_count++;
        Serial.print("NL detected:");
        Serial.print(nl_count);
        if (nl_count == 10)
        {
          Serial.print("10 NLs detected, end of Header , three bytes to digest until the payload");
          Serial.print(client.read());
          Serial.print(client.read());
          Serial.print(client.read());
          header = false;
        }
      }
    }
    if (header == false)
    {
      myFile.print(c);
    }
    //Serial.print(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.available() && !client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    myFile.close();
    Serial.println("done.");
    // do nothing forevermore:
    for (;;)
      ;
  }
}
