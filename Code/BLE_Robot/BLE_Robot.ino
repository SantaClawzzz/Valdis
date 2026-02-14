/*
 * BLE HID Gamepad Client Example
 *
 * This example demonstrates how to connect to a BLE HID Gamepad and read its input.
 * The ESP32 acts as a BLE Central (client) that connects to a BLE gamepad peripheral.
 *
 * Features:
 * - Scans for BLE HID gamepad devices
 * - Connects to the first gamepad found
 * - Secure pairing with bonding
 * - Subscribes to input report notifications
 * - Parses and displays gamepad input (buttons and axes)
 * - Automatic reconnection on disconnect
 *
 * Usage:
 * 1. Upload this sketch to your ESP32
 * 2. Turn on your BLE gamepad (or run the Server_Gamepad example on another ESP32)
 * 3. The ESP32 will scan, connect, and display gamepad input in the serial monitor
 *
 * Note: This example uses "Just Works" pairing for automatic connection without
 * PIN entry or confirmation. The bond is saved for future connections.
 *
 * Compatible with gamepads using the standard HID Report Descriptor format
 *
 * Created by lucasssvaz
 */

#include <BLEDevice.h>
#include <BLESecurity.h>
#include <ESP32Servo.h>

// Defines
#define DBG 5
#define PWM_A 15
#define PWM_B 10
#define ESC 18
#define AIN1 13
#define AIN2 14
#define BIN1 12
#define BIN2 11
#define BUZ 34

Servo esc;

int state = 0;

// HID Service UUID (standard UUID for HID over GATT)
static BLEUUID hidServiceUUID((uint16_t)0x1812);
// HID Report characteristic UUID (used for input/output reports)
static BLEUUID reportCharUUID((uint16_t)0x2A4D);
// HID Report Map characteristic UUID
static BLEUUID reportMapUUID((uint16_t)0x2A4B);

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *pInputReportCharacteristic = nullptr;
static BLEAdvertisedDevice *myDevice = nullptr;
static BLEClient *pClient = nullptr;

// Gamepad report structure (adjust based on your gamepad's report descriptor)
// This matches the Server_Gamepad example format
struct GamepadReport {
  uint8_t reportId;  // Report ID
  int8_t x;          // X axis (-127 to 127)
  int8_t y;          // Y axis (-127 to 127)
  uint8_t buttons;   // 8 buttons (bit 0-7)
} __attribute__((packed));

// Callback function to handle gamepad input notifications
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  Serial.printf("Received %d bytes: ", length);

  // Check if data length matches our expected gamepad report
  if (length == sizeof(GamepadReport)) {
    GamepadReport *report = (GamepadReport *)pData;

    //BUZZER
    bool pressed = report -> buttons & 0x01;
    analogWrite(BUZ, pressed ? 200 : 0);

    //ESC
    /* ESC button control
    bool ESC_BUT = report -> buttons & 0x02;
    if (ESC_BUT == 1){
      servo.write(100);
    }
    */

    /*Analog ESC control
    int minPulse = map(1000, 0, 20000, 0, 4095); // 1ms  
    int maxPulse = map(2000, 0, 20000, 0, 4095); // 2ms  
    int pulse = 0;
    if (state < 50) {
      analogWrite(ESC, minPulse); // low
      state ++;
    } else if (state < 100){
      analogWrite(ESC, 0);
      state ++;
    } else {
      analogWrite(ESC, maxPulse);
    }
    */

    //ESC with servo control
    int pulse = 1;
    if (state < 10){
      esc.writeMicroseconds(1000);
      state++;
    } else if (state < 50) {
      esc.writeMicroseconds(0);
      state++;
    } else if (state <= 100) {
      esc.writeMicroseconds(pulse*40);
      pulse++, state++;
    } else if (state < 120) {
      Serial.print("FULL THROTTLE ESC");
    }

    //MOTORS
    int8_t y = report ->y;
    int8_t base = abs(y);
    int8_t x = report ->x;
    int8_t yA = base;
    int8_t yB = base;
    
    if (x>30){
      yA = max(0, base - abs(x)); //Edit multiplier (0.8) to decrease sensitivity
    } else if (x<-30) {
      yB = max(0, base - abs(x));
    }

    // Determine motor direction
    bool forward = y > 30;
    bool backward = y < -30;

    /*
    // Set motor directions
    digitalWrite(AIN1, forward ? HIGH : backward ? LOW : LOW);
    digitalWrite(AIN2, forward ? LOW : backward ? HIGH : LOW);
    digitalWrite(BIN1, forward ? HIGH : backward ? LOW : LOW);
    digitalWrite(BIN2, forward ? LOW : backward ? HIGH : LOW);
    */

    if (forward) {
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
    }
    else if (backward) {
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    }
    else {
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
    }

    // Set motor speeds
    int16_t pwmA = 0;
    int16_t pwmB = 0;

    if (forward || backward) {
      pwmA = yA * 2;
      pwmB = yB * 2;
    }

    //Serial.printf("Wrote: %d on A and %d on B", pwmA, pwmB);
    analogWrite(PWM_A, pwmA);
    analogWrite(PWM_B, pwmB);

    Serial.printf("ID=%d, X=%4d, Y=%4d, Buttons=0x%02X [", report->reportId, report->x, report->y, report->buttons);

    // Display which buttons are pressed
    for (int i = 0; i < 8; i++) {
      if (report->buttons & (1 << i)) {
        Serial.printf("%d ", i + 1);
      }
    }
    Serial.println("]");
  } else {
    // Unknown format, just display hex dump
    Serial.print("Raw data: ");
    for (size_t i = 0; i < length; i++) {
      Serial.printf("%02X ", pData[i]);
    }
    Serial.println();
  }
}

// Client callbacks to handle connection events
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("Connected to gamepad");
  }

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("Disconnected from gamepad");
  }
};

// Function to connect to the gamepad
bool connectToServer() {
  Serial.print("Connecting to gamepad at ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the gamepad
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  pClient->setMTU(185);  // Set MTU for larger data transfers

  // Obtain a reference to the HID service
  BLERemoteService *pRemoteService = pClient->getService(hidServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find HID service");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found HID service");

  // Get all characteristics to find input reports
  std::map<std::string, BLERemoteCharacteristic *> *pCharMap = pRemoteService->getCharacteristics();

  // Look for input report characteristics (UUID 0x2A4D)
  for (auto const &entry : *pCharMap) {
    BLERemoteCharacteristic *pChar = entry.second;

    if (pChar->getUUID().equals(reportCharUUID)) {
      // Check if this characteristic has notify property (input report)
      if (pChar->canNotify()) {
        Serial.printf(" - Found input report characteristic (handle: 0x%04X)\n", pChar->getHandle());

        // Try to read Report Reference Descriptor to identify report type and ID
        BLERemoteDescriptor *pReportRefDesc = pChar->getDescriptor(BLEUUID((uint16_t)0x2908));
        if (pReportRefDesc != nullptr) {
          String refValue = pReportRefDesc->readValue();
          if (refValue.length() >= 2) {
            uint8_t reportId = refValue[0];
            uint8_t reportType = refValue[1];
            Serial.printf("   Report ID: %d, Type: %d (1=Input, 2=Output, 3=Feature)\n", reportId, reportType);

            // We want input reports (type = 1)
            if (reportType == 1) {
              pInputReportCharacteristic = pChar;
            }
          }
        } else {
          // No report reference descriptor, assume it's an input report
          pInputReportCharacteristic = pChar;
        }
      }
    }
  }

  if (pInputReportCharacteristic == nullptr) {
    Serial.println("Failed to find input report characteristic");
    pClient->disconnect();
    return false;
  }

  // Subscribe to input report notifications
  Serial.println(" - Subscribing to input report notifications");
  pInputReportCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  Serial.println("Successfully connected and subscribed to gamepad!");

  // Note: Security/encryption will be automatically handled by the BLE stack
  // when the HID device requires it (using "Just Works" pairing).

  return true;
}

// Scan callback to detect gamepad devices
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Device found: ");
    Serial.print(advertisedDevice.toString().c_str());

    // Check if device advertises HID service
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(hidServiceUUID)) {
      Serial.print(" - HID Device!");

      // Check if it's a gamepad by appearance (0x03C4 = HID Gamepad)
      if (advertisedDevice.haveAppearance()) {
        uint16_t appearance = advertisedDevice.getAppearance();
        Serial.printf(" (Appearance: 0x%04X)", appearance);
        if (appearance == 0x03C4) {
          Serial.print(" - GAMEPAD!");
        }
      }
      Serial.println();

      // Stop scanning and connect
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    } else {
      Serial.println();
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BLE HID Gamepad Client ===");
  Serial.println("Scanning for BLE HID gamepads...\n");

  // ESC servo control
  esc.attach(ESC, 1000, 2000);

  /* Analog ESC control code
  analogWriteFrequency(ESC, 50);
  analogWriteResolution(12,12);
  */

  BLEDevice::init("ESP32-Gamepad-Client");

  // Configure BLE Security for pairing with HID devices
  BLESecurity *pSecurity = new BLESecurity();

  // Set security capabilities and authentication mode
  // HID devices typically use "Just Works" pairing (no MITM) with bonding

  // Set IO capability to NONE for "Just Works" pairing
  pSecurity->setCapability(ESP_IO_CAP_NONE);

  // Bonding, no MITM, secure connections (for "Just Works" pairing)
  pSecurity->setAuthenticationMode(true, false, true);

  // Set security callbacks (using default implementation)
  BLEDevice::setSecurityCallbacks(new BLESecurityCallbacks());

  Serial.println("Security configured: Bonding + Secure Connections\n");

  // Create scanner and set callbacks
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  //User Code
  pinMode(PWM_A, OUTPUT);
  pinMode(PWM_B, OUTPUT);
  pinMode(ESC, OUTPUT);

  //Motor IN pins
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  //LED
  pinMode(DBG, OUTPUT);

  //Buzzer
  pinMode(BUZ, OUTPUT);
}

void loop() {
  // Connect to gamepad if found
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("\n*** Ready to receive gamepad input ***\n");
    } else {
      Serial.println("Failed to connect to gamepad");
    }
    doConnect = false;
  }

  // Restart scanning if disconnected
  if (!connected && doScan) {
    Serial.println("\nScanning for gamepads...");
    BLEDevice::getScan()->start(5, false);
    delay(1000);
  }

  delay(100);
}
