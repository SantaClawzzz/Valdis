#line 1 "C:\\Users\\Public\\Documents\\Altium\\Valdis\\Code\\Valdis_v0\\Valdis\\Valdis.ino"
#include <Arduino.h>
#include "xbox.h"
#include "Constants.h"
#include "ESP32Servo.h"
#include <ESP32Encoder.h>

// ESC servo control
Servo esc;

// UNKNOWN
namespace {
constexpr uint32_t kPrintIntervalMs = 100;
uint32_t lastPrintMs = 0;
}

#line 16 "C:\\Users\\Public\\Documents\\Altium\\Valdis\\Code\\Valdis_v0\\Valdis\\Valdis.ino"
void setup();
#line 56 "C:\\Users\\Public\\Documents\\Altium\\Valdis\\Code\\Valdis_v0\\Valdis\\Valdis.ino"
void loop();
#line 84 "C:\\Users\\Public\\Documents\\Altium\\Valdis\\Code\\Valdis_v0\\Valdis\\Valdis.ino"
void updateESC(int weaponSpeed);
#line 16 "C:\\Users\\Public\\Documents\\Altium\\Valdis\\Code\\Valdis_v0\\Valdis\\Valdis.ino"
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
  // ESC attaching on pin ESC (18) with min 1000us and max 2000us on-time
  esc.attach(ESC, 1000, 2000);
  /* Start with min throttlle, meaning off 
     This will start the arming sequence */
  esc.writeMicroseconds(1000);  

  // Bluepad32 setup
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);      // Emulator I'm assuming

  if (SERIAL_PRINT_ENABLED) {
    Serial.println("Ready. Move triggers/stick to see left/right motor throttle.");
  }

  // Pinmodes, this currently does not include hall-efect encoders
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  // do NOT use pinMode on ESC - Servo library controls it after attach()
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Initialize encoder library
  initEncoders();
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

  /*
  const uint32_t now = millis();
  if (SERIAL_PRINT_ENABLED && (now - lastPrintMs >= kPrintIntervalMs)) {
    lastPrintMs = now;
    if (!connectedCorrect) {
      Serial.println("[Status] no controller connected");
    } else {
      printDriveDebug();
    }
  }
  */
  //delay(5);
}

// MOVE THIS TO XBOX.CPP

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

