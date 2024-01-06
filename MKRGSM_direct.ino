#include<SDU.h>
#include <MKRGSM.h>
#include <SPI.h>
#include <SD.h>

const char PINNUMBER[] = "";
// APN data
const char GPRS_APN[] = "";
const char GPRS_LOGIN[] = "";
const char GPRS_PASSWORD[] = "";


GSMClient ota_client;
GPRS gprs;
GSM gsmAccess;

File file_ota;


bool header = true;
int nl_count;
int contentLength;

String currentLine;

void setup() {
  // initialize serial communications and wait for port to open:
  Serial.begin(250000);
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

  if (ota_client.connect("probolt.ch", 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    ota_client.print("GET ");
    ota_client.print("/update-1400.bin");
    ota_client.println(" HTTP/1.1");
    ota_client.print("Host: ");
    ota_client.println("server.com");
    ota_client.println("Connection: close");
    ota_client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

  if (SD.exists("UPDATE.bin")) {
    // Serial.println("UPDATE.bin exists.");
    // delete the file:
    //  Serial.println("Removing UPDATE.bin...");
    SD.remove("UPDATE.bin");
  }
  else {
    Serial.println("UPDATE.bin doesn't exist on the SD card. Open a new file..");
  }
  file_ota = SD.open("UPDATE.bin", FILE_WRITE);
}
void loop()
{

  if (ota_client.available()) {
    char c = ota_client.read();

    if (header == true) {
      currentLine += c;
      if (c == '\n') {
        currentLine = "";
        nl_count++;
        //Serial.print("NL detected:");
        //Serial.println(nl_count);
        if (nl_count == 8)//content length auslesen
        {
          String headerLine = ota_client.readStringUntil('\n');
          //Serial.print("headerLine: ");
          //Serial.println(headerLine);
          contentLength = headerLine.substring(16).toInt();
        }
        if (nl_count == 10)
        {
          //   Serial.print("10 NLs detected, end of Header , one byte to digest until the payload: ");
          Serial.print(ota_client.read());

          header = false;
        }
      }
    }
    if (header == false)
    {
      file_ota.print(c);
    }
    // Serial.print(c);
  }

  // if the server's disconnected, stop the client:
  if (!ota_client.available() && !ota_client.connected()) {
    //   Serial.println();
    Serial.println("disconnecting.");
    ota_client.stop();
    file_ota.close();


    // Open the file again to check the sizt
    file_ota = SD.open("UPDATE.bin");

    if (file_ota) {
      // Get the size of the file
      int fileSize = file_ota.size();
      // Close the file
      file_ota.close();
      Serial.print("File size on the SD card: ");
      Serial.print(fileSize);
      Serial.println(" bytes");

      Serial.print("File size on server: ");
      Serial.println(contentLength);

      if (fileSize == contentLength)
      {
        Serial.print("Size Server eq size SD, reboot");
        NVIC_SystemReset();
      }
      else
      {
        Serial.print("Size Server eq size SD NOTeq");
      }

    } else {
      Serial.println("Error opening the file");
    }

   
  }
}
