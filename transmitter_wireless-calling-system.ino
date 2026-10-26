
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

// ==================================
// CNC STORE BANDUNG
// WIRELESS CALLING SYSTEM
// ESP32 V1 + OLED + KEYPAD 4x5
// ==================================

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT,
  &Wire, -1
);

// KEYPAD 5 BARIS x 4 KOLOM
const byte ROWS = 5;
const byte COLS = 4;

// Susunan mengikuti foto keypad
char keys[ROWS][COLS] = {
  {'F', 'G', '#', '*'},
  {'1', '2', '3', 'U'},
  {'4', '5', '6', 'D'},
  {'7', '8', '9', 'C'},
  {'L', '0', 'R', 'E'}
};

// F = F1, G = F2
// U = Panah Atas
// D = Panah Bawah
// C = ESC
// L = Panah Kiri
// R = Panah Kanan
// E = ENT

byte rowPins[ROWS] = {
  13, 14, 16, 17, 32
};

byte colPins[COLS] = {
  18, 19, 23, 27
};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins, colPins,
  ROWS, COLS
);

// ESP-NOW
#define WIFI_CHANNEL 1
#define MAGIC_CODE 0xC5A12026UL
#define CMD_CALL 1
#define CMD_RESET 2

uint8_t broadcastAddress[] = {
  0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF
};

typedef struct {
  uint32_t magic;
  uint32_t sequence;
  uint8_t target;
  uint8_t command;
} CallPacket;

uint32_t sequenceNumber = 0;

String nomor = "";
String pesan = "MASUKKAN NOMOR";

bool status01 = false;
bool status02 = false;

unsigned long pesanSampai = 0;

// ==================================
// TAMPILAN OLED
// ==================================

void tampilOLED() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(4, 0);
  display.print("CNC WIRELESS CALL");

  display.drawLine(
    0, 11, 127, 11,
    SSD1306_WHITE
  );

  display.setCursor(8, 15);
  display.print("NOMOR ANTREAN:");

  String tampilNomor = nomor;

  if (tampilNomor == "") {
    tampilNomor = "--";
  } else if (tampilNomor.length() == 1) {
    tampilNomor = "0" + tampilNomor;
  }

  display.setTextSize(3);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(
    tampilNomor, 0, 0,
    &x1, &y1, &w, &h
  );

  display.setCursor(
    (128 - w) / 2,
    27
  );

  display.print(tampilNomor);

  display.setTextSize(1);
  display.setCursor(0, 55);

  if (pesanSampai != 0 &&
      (int32_t)(pesanSampai - millis()) > 0) {

    display.print(pesan);

  } else {

    display.print("01:");
    display.print(status01 ? "ON " : "OFF");

    display.print(" 02:");
    display.print(status02 ? "ON" : "OFF");
  }

  display.display();
}

// ==================================
// PESAN STATUS
// ==================================

void setPesan(String teks) {
  pesan = teks;
  pesanSampai = millis() + 1800;
}

// ==================================
// KIRIM PERINTAH
// ==================================

bool kirimPerintah(
  uint8_t target,
  uint8_t command
) {

  CallPacket packet = {};

  packet.magic = MAGIC_CODE;
  packet.sequence = ++sequenceNumber;
  packet.target = target;
  packet.command = command;

  bool berhasil = true;

  for (int i = 0; i < 3; i++) {

    esp_err_t hasil = esp_now_send(
      broadcastAddress,
      (uint8_t *)&packet,
      sizeof(packet)
    );

    if (hasil != ESP_OK) {
      berhasil = false;
      Serial.println("Gagal mengantre pengiriman");
    }

    delay(30);
  }

  return berhasil;
}

// ==================================
// PANGGIL ANTREAN
// ==================================

void panggilAntrean() {

  if (nomor == "") {
    setPesan("ISI NOMOR DULU");
    return;
  }

  int id = nomor.toInt();

  if (id != 1 && id != 2) {
    setPesan("NOMOR TIDAK ADA");
    return;
  }

  bool terkirim = kirimPerintah(
    id,
    CMD_CALL
  );

  if (terkirim) {

    if (id == 1) status01 = true;
    if (id == 2) status02 = true;

    setPesan("CALL DIKIRIM");

    Serial.print("PANGGIL NOMOR ");
    Serial.println(id);

  } else {
    setPesan("GAGAL KIRIM");
  }

  nomor = "";
}

// ==================================
// RESET SEMUA
// ==================================

void resetSemua() {

  bool terkirim = kirimPerintah(
    0,
    CMD_RESET
  );

  if (terkirim) {

    status01 = false;
    status02 = false;

    setPesan("RESET DIKIRIM");

  } else {
    setPesan("RESET GAGAL");
  }

  nomor = "";

  Serial.println("RESET SEMUA");
}

// ==================================
// SETUP
// ==================================

void setup() {

  Serial.begin(115200);

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  if (!display.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  )) {

    Serial.println("OLED ERROR");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 25);
  display.print("SYSTEM STARTING");
  display.display();

  keypad.setDebounceTime(40);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_channel(
    WIFI_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  if (esp_now_init() != ESP_OK) {

    Serial.println("ESP-NOW ERROR");

    while (true) {
      delay(1000);
    }
  }

  esp_now_peer_info_t peer = {};

  memcpy(
    peer.peer_addr,
    broadcastAddress,
    6
  );

  peer.channel = WIFI_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {

    Serial.println("PEER ERROR");

    while (true) {
      delay(1000);
    }
  }

  setPesan("SYSTEM READY");

  Serial.println("WIRELESS CALL READY");
}

// ==================================
// LOOP
// ==================================

void loop() {

  char tombol = keypad.getKey();

  if (tombol != NO_KEY) {

    Serial.print("TOMBOL: ");
    Serial.println(tombol);

    // INPUT NOMOR
    if (tombol >= '0' && tombol <= '9') {

      if (nomor.length() < 3) {
        nomor += tombol;
      }
    }

    // ENT ATAU # = CALL
    else if (
      tombol == 'E' ||
      tombol == '#'
    ) {
      panggilAntrean();
    }

    // ESC = HAPUS
    else if (tombol == 'C') {

      nomor = "";
      setPesan("INPUT DIHAPUS");
    }

    // * = RESET SEMUA
    else if (tombol == '*') {

      resetSemua();
    }
  }

  static unsigned long lastOLED = 0;

  if (millis() - lastOLED >= 80) {

    lastOLED = millis();
    tampilOLED();
  }

  delay(5);
}
