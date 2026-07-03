#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

namespace {

constexpr uint8_t PIN_SD_CS = 5;
constexpr uint8_t PIN_SD_SCK = 18;
constexpr uint8_t PIN_SD_MISO = 19;
constexpr uint8_t PIN_SD_MOSI = 23;

constexpr uint32_t SPI_FREQUENCY_HZ = 1000000;

SPIClass sdSpi(VSPI);

const char *cardTypeToString(sdcard_type_t type) {
  switch (type) {
    case CARD_MMC:
      return "MMC";
    case CARD_SD:
      return "SDSC";
    case CARD_SDHC:
      return "SDHC/SDXC";
    case CARD_NONE:
      return "No card";
    default:
      return "Unknown";
  }
}

void printBanner() {
  Serial.println();
  Serial.println("=== SD Card Bring-Up ===");
  Serial.printf("CS   : GPIO %u\n", PIN_SD_CS);
  Serial.printf("SCK  : GPIO %u\n", PIN_SD_SCK);
  Serial.printf("MISO : GPIO %u\n", PIN_SD_MISO);
  Serial.printf("MOSI : GPIO %u\n", PIN_SD_MOSI);
  Serial.printf("SPI frequency: %lu Hz\n", SPI_FREQUENCY_HZ);
}

void listDirectory(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("  Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("  Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  [DIR ] %s\n", file.name());
      if (levels > 0) {
        listDirectory(fs, file.path(), levels - 1);
      }
    } else {
      Serial.printf("  [FILE] %s (%u bytes)\n", file.name(),
                    static_cast<unsigned>(file.size()));
    }
    file = root.openNextFile();
  }
}

void readHelloFile(fs::FS &fs) {
  const char *path = "/hello.txt";
  Serial.printf("Reading %s\n", path);

  File file = fs.open(path, FILE_READ);
  if (!file) {
    Serial.println("  Could not open hello.txt");
    Serial.println("  Put a small text file called hello.txt in the SD-card root.");
    return;
  }

  Serial.println("  File contents:");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println();
}

void printCardInfo() {
  sdcard_type_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card detected.");
    return;
  }

  uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);
  uint64_t totalMb = SD.totalBytes() / (1024ULL * 1024ULL);
  uint64_t usedMb = SD.usedBytes() / (1024ULL * 1024ULL);

  Serial.printf("Card type     : %s\n", cardTypeToString(cardType));
  Serial.printf("Card size     : %llu MB\n", cardSizeMb);
  Serial.printf("Filesystem total: %llu MB\n", totalMb);
  Serial.printf("Filesystem used : %llu MB\n", usedMb);
}

bool beginSdCard() {
  sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  if (!SD.begin(PIN_SD_CS, sdSpi, SPI_FREQUENCY_HZ)) {
    Serial.println("SD.begin() failed.");
    return false;
  }

  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  printBanner();

  if (!beginSdCard()) {
    Serial.println("Check power, wiring, card formatting, and SPI pin assignments.");
    return;
  }

  Serial.println("SD initialization succeeded.");
  printCardInfo();
  listDirectory(SD, "/", 1);
  readHelloFile(SD);
}

void loop() {
  delay(1000);
}

