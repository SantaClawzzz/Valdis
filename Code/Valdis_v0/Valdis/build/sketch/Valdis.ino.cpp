#line 1 "C:\\Users\\Vadim\\Downloads\\Valdis\\Valdis.ino"
#include <Arduino.h>
#include "xbox.h"
#include "Constants.h"
#include "ESP32Servo.h"

//ESC servo control
Servo esc;

static int state = 0;

namespace {
constexpr uint32_t kPrintIntervalMs = 100;
uint32_t lastPrintMs = 0;
}

#line 16 "C:\\Users\\Vadim\\Downloads\\Valdis\\Valdis.ino"
void setup();
#line 50 "C:\\Users\\Vadim\\Downloads\\Valdis\\Valdis.ino"
void loop();
#line 76 "C:\\Users\\Vadim\\Downloads\\Valdis\\Valdis.ino"
void updateESC(int weaponSpeed);
#line 16 "C:\\Users\\Vadim\\Downloads\\Valdis\\Valdis.ino"
void setup() {
  if (SERIAL_PRINT_ENABLED) {
    Serial.begin(115200);
    // wait for the USB-serial connection to be established (helpful on some boards)
    uint32_t startWait = millis();
    while (!Serial && millis() - startWait < 2000) {
      // spin for up to 2 seconds
      delay(10);
    }
    delay(100); // give monitor a moment to open
    Serial.println("Starting Xbox throttle monitor...");
  }
  esc.attach(ESC, 1000, 2000);
  esc.writeMicroseconds(1000);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  if (SERIAL_PRINT_ENABLED) {
    Serial.println("Ready. Move triggers/stick to see left/right motor throttle.");
    Serial.flush(); // make sure the message goes out immediately
  }
  pinMode(DBG, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  // do NOT use pinMode on ESC - Servo library controls it after attach()
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BUZ, OUTPUT);

}

void loop() {
  const bool dataUpdated = BP32.update();
  if (!dataUpdated) {
    delay(10);
    return;
  }

  processConnectedControllers();

  // Update ESC based on weapon speed
  updateESC(weaponSpeed);

  const uint32_t now = millis();
  if (SERIAL_PRINT_ENABLED && (now - lastPrintMs >= kPrintIntervalMs)) {
    lastPrintMs = now;
    if (!connectedCorrect) {
      Serial.println("[Status] no controller connected");
    } else {
      printDriveDebug();
    }
  }


  //delay(5);
}

void updateESC(int weaponSpeed) {
  // Map weapon speed (0-255) to ESC microseconds (1000-2000)
  // 0 = 1000 µs (idle/off)
  // 255 = 2000 µs (full speed)
  int escMicros = map(weaponSpeed, 0, 255, 1000, 2000);
  esc.writeMicroseconds(escMicros);
  
  if (SERIAL_PRINT_ENABLED && weaponSpeed > 0) {
    Serial.printf("[ESC] weaponSpeed=%d -> %d us\n", weaponSpeed, escMicros);
  }
}

