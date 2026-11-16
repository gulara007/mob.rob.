// ===================== Lab 6 — Board A (node, advertiser) =====================
// Receives a reading from STM32 over UART (STM32 USART1 <-> GPIO17/18),
// packs it with pack_reading() and advertises it as NON-CONNECTABLE
// manufacturer data every ADV_INTERVAL_UNITS * 0.625 ms.
//
// Wiring:  STM32 D8 (PA9, TX)  -> GPIO18 (RX)
//          STM32 D2 (PA10, RX) <- GPIO17 (TX)
//          GND                 -- GND
//
// UART frame (replace with your Lecture 5 frame if it differs):
//   0xAA | LEN | PAYLOAD[LEN] | CHK      CHK = LEN ^ PAYLOAD[0] ^ ... ^ PAYLOAD[LEN-1]
// Payload STM32 -> A:  seq (uint16 LE) | temp*100 (int16 LE)   = 4 bytes
// ==============================================================================

#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include "esp_bt.h"

#define UART_RX_PIN 18
#define UART_TX_PIN 17
#define UART_BAUD   115200

#define COMPANY_ID  0xFFFF          // from the encoding table

// Advertising interval in units of 0.625 ms:
//   160  = 100 ms     668 = 417.5 ms     1636 = 1022.5 ms     1600 = 1 s
#define ADV_INTERVAL_UNITS 160

// TX power: ESP_PWR_LVL_N0 = 0 dBm, ESP_PWR_LVL_P9 = +9 dBm
#define TX_POWER ESP_PWR_LVL_N0

#if ESP_ARDUINO_VERSION_MAJOR >= 3
typedef String BleStr;          // Arduino-ESP32 core 3.x
#else
typedef std::string BleStr;     // Arduino-ESP32 core 2.x
#endif

BLEAdvertising *adv;

// ---------- Encoding table -> bytes ----------
// bytes 0-1 Company ID (LE) | 2-3 seq (LE) | 4-5 temp*100 (int16, LE)
size_t pack_reading(uint8_t *out, uint16_t seq, int16_t temp_c100) {
  out[0] = COMPANY_ID & 0xFF;
  out[1] = COMPANY_ID >> 8;
  out[2] = seq & 0xFF;
  out[3] = seq >> 8;
  out[4] = (uint16_t)temp_c100 & 0xFF;
  out[5] = (uint16_t)temp_c100 >> 8;
  return 6;
}

void advertiseReading(uint16_t seq, int16_t temp_c100) {
  uint8_t md[6];
  size_t n = pack_reading(md, seq, temp_c100);

  BLEAdvertisementData ad;
  ad.setFlags(0x06);                              // 02 01 06
  ad.setManufacturerData(BleStr((char *)md, n));  // 07 FF + 6 bytes

  // Only replace the data. Advertising keeps running on its own schedule,
  // so the new reading goes out at the NEXT regular advertising event.
  // (stop()+start() would send it immediately and hide the interval in 6C.)
  adv->setAdvertisementData(ad);
}

// ---------- UART frame receiver (state machine) ----------
bool readFrame(HardwareSerial &s, uint8_t *payload, uint8_t &len) {
  static uint8_t state = 0, n = 0, idx = 0, chk = 0, buf[32];
  while (s.available()) {
    uint8_t b = s.read();
    switch (state) {
      case 0: if (b == 0xAA) state = 1; break;
      case 1: n = b; chk = b; idx = 0;
              state = (n > 0 && n <= sizeof(buf)) ? 2 : 0; break;
      case 2: buf[idx++] = b; chk ^= b; if (idx == n) state = 3; break;
      case 3: state = 0;
              if (b == chk) { memcpy(payload, buf, n); len = n; return true; }
              break;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);                                        // USB debug
  Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN); // to STM32

  BLEDevice::init("");
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, TX_POWER);

  adv = BLEDevice::getAdvertising();
  adv->setAdvertisementType(ADV_TYPE_NONCONN_IND);   // non-connectable
  adv->setMinInterval(ADV_INTERVAL_UNITS);
  adv->setMaxInterval(ADV_INTERVAL_UNITS);
  adv->setScanResponse(false);

  advertiseReading(0, 0);   // something on air from the start
  adv->start();             // start advertising ONCE; later only the data changes

  Serial.print("Board A MAC: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());  // write it down for board B
}

void loop() {
  uint8_t p[32], len;
  if (readFrame(Serial1, p, len) && len >= 4) {
    uint16_t seq  = p[0] | (p[1] << 8);
    int16_t  temp = (int16_t)(p[2] | (p[3] << 8));
    advertiseReading(seq, temp);
    Serial.printf("adv seq=%u temp=%d\n", seq, temp);
  }
}board_A_node.ino
