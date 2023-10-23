#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "driver/gpio.h"

#define SD_CS 5 // GPIO5 (change this to match your SD card module's CS pin)
#define BUF_SIZE 6600
#define UART_NUM UART_NUM_0
#define ECHO_TEST_TXD (GPIO_NUM_1)
#define ECHO_TEST_RXD (GPIO_NUM_3)

File file;
String buffer;
boolean logging = true;
const char filename[] = "data.log";
unsigned long logStartTime = 0;
unsigned long logDuration = 5400000; // 1.5 hours in milliseconds
int logCount = 1;

void setup() {
  // Set up the UART for 614400 baud rate, 8 data bits, no parity, and 1 stop bit.
  uart_config_t uart_config = {
      .baud_rate = 921600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  uart_param_config(UART_NUM, &uart_config);

  // Set up the UART pins.
  uart_set_pin(UART_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  // Set up a DMA buffer to store incoming UART data.
  uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

  // Initialize the SD card.
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card failed to initialize. Check your wiring.");
    return;
  }

  char newFilename[20];
  sprintf(newFilename, "/data%d.txt", logCount);
  logCount++;
  SD.remove(newFilename);
  file = SD.open(newFilename, FILE_WRITE);
  if (!file) {
    Serial.print("Error opening ");
    Serial.println(newFilename);
    while (1);
  }
}

void loop() {
  uint8_t data[BUF_SIZE];
  int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
  if (len > 0) {
    for (int i = 0; i < len; i++) {
      buffer += (char)data[i];
    }

    if (buffer.length() >= BUF_SIZE) {
      file.print(buffer);
      file.flush();
      buffer = "";
    }

    if ((millis() - logStartTime) >= logDuration) {
      logging = false;
      file.close();
    }
  }

  if (file && file.availableForWrite() >= BUF_SIZE && buffer.length() >= BUF_SIZE) {
    file.print(buffer.substring(0, BUF_SIZE));
    file.flush();
    buffer.remove(0, BUF_SIZE);
  }
}