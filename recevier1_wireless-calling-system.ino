
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_arduino_version.h>

// ======================================
// WIRELESS CALLING - ANTREAN 01
// ESP32 DEVKIT V1
// ======================================

#define DEVICE_ID 1

// PIN OUTPUT
#define BUZZER_PIN 25
#define LED_PIN 26

// ESP-NOW
#define WIFI_CHANNEL 1
#define MAGIC_CODE 0xC5A12026UL
#define CMD_CALL 1
#define CMD_RESET 2

// DURASI BUNYI BUZZER
#define BEEP_ON_TIME 200
#define BEEP_OFF_TIME 300

// FORMAT DATA (SAMA DENGAN CONTROLLER)
typedef struct {
  uint32_t magic;
  uint32_t sequence;
  uint8_t target;
  uint8_t command;
} CallPacket;

volatile uint8_t pendingCommand = 0;

bool callingActive = false;
bool buzzerState = false;

unsigned long lastBeepTime = 0;

// ======================================
// CALLBACK PENERIMA ESP-NOW
// KOMPATIBEL CORE ESP32 2.x DAN 3.x
// ======================================

void processReceivedData(
  const uint8_t *data,
  int len
) {

  if (len != sizeof(CallPacket)) {
    return;
  }

  CallPacket packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != MAGIC_CODE) {
    return;
  }

  if (packet.command == CMD_RESET &&
      packet.target == 0) {

    pendingCommand = CMD_RESET;
  }

  if (packet.command == CMD_CALL &&
      packet.target == DEVICE_ID) {

    pendingCommand = CMD_CALL;
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3

void onDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len
) {
  processReceivedData(data, len);
}

#else

void onDataRecv(
  const uint8_t *mac,
  const uint8_t *data,
  int len
) {
  processReceivedData(data, len);
}

#endif

// ======================================
// AKTIFKAN PANGGILAN
// ======================================

void startCalling() {

  callingActive = true;

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);

  buzzerState = true;
  lastBeepTime = millis();

  Serial.println("ANTREAN 01 DIPANGGIL");
}

// ======================================
// RESET PANGGILAN
// ======================================

void stopCalling() {

  callingActive = false;
  buzzerState = false;

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Serial.println("ANTREAN 01 RESET");
}

// ======================================
// BUZZER BEEP BEEP BERULANG
// ======================================

void updateBuzzer() {

  if (!callingActive) {
    return;
  }

  unsigned long now = millis();

  if (buzzerState) {

    if (now - lastBeepTime >= BEEP_ON_TIME) {

      buzzerState = false;
      digitalWrite(BUZZER_PIN, LOW);
      lastBeepTime = now;
    }

  } else {

    if (now - lastBeepTime >= BEEP_OFF_TIME) {

      buzzerState = true;
      digitalWrite(BUZZER_PIN, HIGH);
      lastBeepTime = now;
    }
  }
}

// ======================================
// SETUP
// ======================================

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_channel(
    WIFI_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  Serial.print("MAC Antrean 01: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {

    Serial.println("ESP-NOW ERROR");

    while (true) {
      delay(1000);
    }
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("ANTREAN 01 READY");
}

// ======================================
// LOOP
// ======================================

void loop() {

  uint8_t command = pendingCommand;

  if (command != 0) {

    pendingCommand = 0;

    if (command == CMD_CALL) {
      startCalling();
    }

    if (command == CMD_RESET) {
      stopCalling();
    }
  }

  updateBuzzer();

  delay(2);
}
