
#include <DS3231.h>
#include <SD.h>

#include <Arduino.h>
#include <U8g2lib.h>

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

File txtFile;

#define SAMPLES 64
uint16_t historySeconds[SAMPLES];
uint16_t historyMinutes[SAMPLES];

void setup(void) {
  /* U8g2 Project: Pax Instruments Shield: Enable Backlight */
  u8x8.begin();
  u8x8.setFont(u8x8_font_7x14_1x2_r);

  if (!SD.begin(SPI_HALF_SPEED, /* pin */ 10)) {
    u8x8.println(F("No SD Card"));
    while (1);
  }
}

void drawGraph(uint8_t col, uint8_t row, uint16_t *values) {
  uint8_t tmp[8];
  for (uint8_t r = 0; r < 4; r++) {
    for (uint8_t h = 0; h < SAMPLES; h += 8) {
      for (uint8_t i = 0; i < 8; i++) {
        int x = values[SAMPLES - h - 1 - i] / 16;
        x -= 8 * r;
        tmp[i] = 0;
        for (uint8_t b = 0; b < 8 && x > 0; b++, x--) {
          if (x) {
            tmp[i] |= (1 << (7 - b));
          }
        }
      }
      u8x8.drawTile(col + h / 8, row - r, 1, tmp);
    }
  }
}

void toString(char *dst, uint8_t a, uint8_t b, uint8_t c, char sep) {
  dst[0] = a / 10 + '0';
  dst[1] = a % 10 + '0';
  dst[2] = sep;
  dst[3] = b / 10 + '0';
  dst[4] = b % 10 + '0';
  dst[5] = sep;
  dst[6] = c / 10 + '0';
  dst[7] = c % 10 + '0';
  dst[8] = 0;
}

int lastS = -1;

void loop(void) {
  DateTime dt = RTClib::now();

  if (lastS != dt.second()) {
    char tmp[16];
    lastS = dt.second();

    u8x8.clear();
    u8x8.setCursor(0, 0);

    toString(tmp, dt.year() - 2000, dt.month(), dt.day(), '-');
    u8x8.println(tmp);

    if (dt.second() == 0) {
      unsigned long total = 0;
      for (int h = 0; h < SAMPLES; h++)
        total += historySeconds[h];
      memmove(historyMinutes + 1, historyMinutes, (SAMPLES - 1) * sizeof(uint16_t));
      historyMinutes[0] = total / SAMPLES;
      strcat(tmp, ".csv");
      txtFile = SD.open(tmp, FILE_WRITE);
    }

    uint16_t gasVal = analogRead(0);

    memmove(historySeconds + 1, historySeconds, (SAMPLES - 1) * sizeof(uint16_t));
    historySeconds[0] = gasVal;

    toString(tmp, dt.hour(), dt.minute(), dt.second(), ':');
    u8x8.println(tmp);

    if (txtFile) {
      strcat(tmp, ",");
      txtFile.print(tmp);
    }

    itoa(gasVal, tmp, 10);
    u8x8.println(tmp);

    if (txtFile) {
      txtFile.println(tmp);
      txtFile.close();
    }

    drawGraph(8, 3, historySeconds);
    drawGraph(8, 7, historyMinutes);
  }
  delay(250);
}
