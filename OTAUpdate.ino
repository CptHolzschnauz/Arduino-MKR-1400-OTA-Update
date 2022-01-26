#define SD_CS 4 // SPI Chip select pin for the sd card reader. MUST be 4 because of the SDU lib 
#include <SD.h>
// the mighty SDU lib for the sketch update from the SD card
//#include <SDU.h>

// GSM credentials
const char PINNUMBER[] = "";
// APN data
const char GPRS_APN[] = "internet";
const char GPRS_LOGIN[] = "";
const char GPRS_PASSWORD[] = "";


// HTTP Server infos

String server =  "yourserver.com";
int port =  80;
String filename_server =  "UPDATE.bin";
//int filesize_server =  207076; // Filesize of the UPDATE.bin on the http server

// SD card infos

const char* sd_filename = "UPDATE.bin";

// The file name on the modem memory

constexpr char* filename_modem { "UPDATE.bin" };

unsigned int fileSize = 0;
String http_header = "";

// Read from modem memory: block size
constexpr unsigned int blockSize { 512 };


#include <MKRGSM.h>

// initialize the library instance
GSMClient client;
GPRS gprs;
GSM gsmAccess;


GSMFileUtils fileUtils(false);

void setup()
{

 Serial.begin(115200);
  while (!Serial)
    ;
 
  Serial.println("Start the OTA Update process..");


  fileUtils.begin();

  // FYI but not neccesary : Show what's saved on the modem memory

  auto numberOfFiles = fileUtils.fileCount();
  Serial.print("Number of Files on the modem: ");
  Serial.println(numberOfFiles);
  Serial.println();

  // printFiles(fileUtils);



  Serial.print("Connecting the net");

  // GSM - let's talk

  bool connected = false;


  // attach the shield to the GPRS network with the APN, login and password
  while (!connected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) && (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;  Serial.println("GPRS Connected!");
    } else {
      Serial.println("GSM not connected");
      delay(1000);
    }
  }


  // if you get a connection, report back via serial:
  if (client.connect(server.c_str(), port)) {
    Serial.println("http server connected!");
    // Make a HTTP request:
    client.print("GET /");
    client.print(filename_server);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("Connection failed");
  }

  // Download and store block-by-block on the modems memory
  storeFileBuffered(filename_server);

  // FYI
  /*
    auto updateBinSize = fileUtils.listFile(filename_modem);
    Serial.print(filename_modem);
    Serial.print(" downloaded size: ");
    Serial.println(updateBinSize);

    numberOfFiles = fileUtils.fileCount();
    Serial.print("Number of Files: ");
    Serial.println(numberOfFiles);
    Serial.println();

    printFiles(fileUtils);
  */


  // Copy from modem to Sd card

  Serial.println("Download complete -> Copy the downloaded file to the SD card");

  auto size = fileUtils.listFile(filename_modem);
  auto cycles = (size / blockSize) + 1;
  Serial.print("File size: ");
  Serial.println(size);
  Serial.print("Write cycles: ");
  Serial.println(cycles);

  // Prepare SD Card

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    // don't continue:
    while (true);
  }
  Serial.println("SD card initialization done.");
  // Altes File weg
  Serial.print("Delete (if existing) file "); Serial.print(sd_filename); Serial.println(" from SD card.");
  SD.remove(sd_filename);
  Serial.println("Create new file on SD card");

  File file = SD.open(sd_filename, O_CREAT | O_WRITE);
  if (!file) {
    Serial.println("Could not create the file on the SD card.");
    return;
  }

  uint32_t totalRead { 0 };

  uint64_t b;
  //unsigned long b;

  for (auto i = 0; i < cycles; i++) {
    uint8_t block[blockSize] { 0 };
    auto read = fileUtils.readBlock(filename_modem, i * blockSize, blockSize, block);
    totalRead += read;
    for (auto j = 0; j < read; j++) {
      // if (block[j] < 16)
      //    Serial.print(0);
      //  Serial.print(block[j], BIN);
      // Serial.print("Write SD card block nr "); Serial.print(i); Serial.print(" from "); Serial.println(cycles);
      Serial.print(".");
      b = block[j], BIN;
      file.write(b);
    }
    Serial.println(); Serial.println("Block: "); Serial.print(i);
  }

  if (totalRead != size) {
    Serial.print("ERROR - File size: ");
    Serial.print(size);
    Serial.print(" Bytes read: ");
    Serial.println(totalRead);
  }

  file.close();
}

void loop()
{
  Serial.println("Suceed! Restart/power cycle the board to update from the sd card...");
  /*
    MODEM.send("AT+CFUN=15,1");
    MODEM.waitForResponse(20000);
    MODEM.send("AT+CPWROFF");
    MODEM.waitForResponse(20000);

    NVIC_SystemReset();
  */
  Serial.print("freeMemory: ");
  Serial.println(freeMemory());
  delay(10000);
}


void storeFileBuffered(String filename_server)
{

  bool is_header_complete = false;

  // Skip the HTTP header
  while (!is_header_complete) {
    while (client.available()) {
      const char c = client.read();
      http_header += c;
      if (http_header.endsWith("\r\n\r\n")) {
        Serial.println("Header Complete");
        is_header_complete = true;

        // HTTP Content-Length header.
        fileSize = getContentLength();

        Serial.println();
        Serial.print("HTTP header complete. ");
        Serial.print("OTA file size out of the http header is ");
        Serial.print(fileSize);
        Serial.println(" bytes.");

        break;
      }
    }
  }

  Serial.print("Ready to download \"");
  Serial.print(filename_server);
  Serial.print("\" - len: ");
  Serial.print(fileSize);
  Serial.println(" bytes.");

  constexpr uint32_t len { 512 };

  uint32_t cycles = fileSize / len;
  uint32_t spares = fileSize % len;

  int totalRead { 0 };

  fileUtils.deleteFile(filename_server);

  Serial.print("Saving file in ");
  Serial.print(cycles + 1);
  Serial.print(" blocks. [");
  Serial.print(cycles);
  Serial.print(' ');
  Serial.print(len);
  Serial.print(" -bytes blocks and ");
  Serial.print(spares);
  Serial.println(" bytes].");

  // Define download and save

  auto downloadAndSaveTrunk = [filename_server](uint32_t len) {
    char buf[len] { 0 };
    uint32_t written { 0 };

    if (client.available())
      written = client.readBytes(buf, len);

    fileUtils.appendFile(filename_server, buf, written);
    return written;
  };

  // Define wrapper function
  auto saveTrunk = [&totalRead, downloadAndSaveTrunk](size_t iter, uint32_t len) {
    Serial.print("Download Block ");
    if (iter < 10) Serial.print(' '); if (iter < 100) Serial.print(' ');
    Serial.print(iter);

    totalRead += downloadAndSaveTrunk(len);

    Serial.print(": Size:");
    Serial.print(len);
    Serial.print(" - Total: ");
    Serial.print(totalRead);
    Serial.println(" bytes");
  };

  // Download and save the rest of the block
  for (auto c = 0; c <= cycles; c++)
    saveTrunk(c, len);
  Serial.println();
  Serial.print("freeMemory: ");
  Serial.println(freeMemory());
}

int getContentLength()
{
  const String contentLengthHeader = "Content-Length:";
  const auto contentLengthHeaderLen = contentLengthHeader.length();

  auto indexContentLengthStart = http_header.indexOf(contentLengthHeader);
  if (indexContentLengthStart < 0) {
    Serial.println("Unable to find Content-Length header (Start)");
    return 0;
  }
  auto indexContentLengthStop = http_header.indexOf("\r\n", indexContentLengthStart);
  if (indexContentLengthStart < 0) {
    Serial.println("Unable to find Content-Length header (Stop)");
    return 0;
  }
  auto contentLength = http_header.substring(indexContentLengthStart + contentLengthHeaderLen + 1, indexContentLengthStop);

  contentLength.trim();
  return contentLength.toInt();
}

