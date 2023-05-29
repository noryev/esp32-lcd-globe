#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <SPI.h>
#include <SPIFFS.h>

#define BG_COLOR ST77XX_BLACK

const char* ssid = "evil_morty";
const char* password = "P^yuEJKy7h<{Kmza&<vkJ4";

// Display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RESET);

void writeMessage(const String& message) {
  Serial.println(message);
  tft.fillScreen(BG_COLOR);
  tft.setCursor(0, 0);
  tft.println(message);
}

void bmpDraw(char *filename, int x, int y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  File     bmpFile;
  int      bmpWidth, bmpHeight;
  uint8_t  bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  uint8_t  sdbuffer[3*BUFSIZE];
  uint8_t  buffidx = sizeof(sdbuffer);
  boolean  goodBmp = false;
  boolean  flip    = true;

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  bmpFile = SPIFFS.open(filename, "r");

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { 
    // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not common but has been observed in some
        // .bmp files.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        // Crop area to be loaded
        x = (tft.width()  - w) / 2;
        y = (tft.height() - h) / 2;
        
        for (row=0; row<h; row++) { // For each scanline...
          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos, SeekSet);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  tft.init(240, 240);                // Initialize ST7789 screen
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH); // Backlight on

  tft.fillScreen(BG_COLOR);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextWrap(true);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    writeMessage("Connecting to WiFi...");
  }
  writeMessage("Connected to WiFi");

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  bmpDraw("/image.bmp", 0, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
}
