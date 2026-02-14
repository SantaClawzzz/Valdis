//#include "TANK_PRO_MAX.ino"
#include <Arduino.h>
#include "xbox.h"
#include "Constants.h"

#if SERIAL_PRINT_ENABLED
  #define LOG_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define LOG_PRINTLN(x) Serial.println(x)
#else
  #define LOG_PRINTF(...)
  #define LOG_PRINTLN(x)
#endif

int leftSpeed, rightSpeed;
bool leftForward, rightForward;
bool weaponHalfSpeedMode = false;
bool manualMode = false;
bool weaponFullSpeedMode = false;
bool battlebot = false;
int weaponSpeed = 0;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

bool connectedCorrect = false;

void onConnectedController(ControllerPtr ctl) {
    // Retrieve the controller properties
    LOG_PRINTLN("[Callback] controller connection event");

    ControllerProperties properties = ctl->getProperties();
    
    // Convert the Bluetooth address bytes to a string.
    // (Assumes 'bdaddr' is a uint8_t[6] within ControllerProperties.)

    char addressStr[18];  // Enough space for "xx:xx:xx:xx:xx:xx" and the terminating null
    sprintf(addressStr, "%02x:%02x:%02x:%02x:%02x:%02x",
            properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);

    bool macConfigured = (CONTROLLER_MAC[0] != '\0');
    bool macMatches = !macConfigured || (strcasecmp(addressStr, CONTROLLER_MAC) == 0);
    const char* expectedMac = macConfigured ? CONTROLLER_MAC : "ANY";

    LOG_PRINTF("[Controller] Detected MAC: %s | Expected MAC: %s\n", addressStr, expectedMac);
    LOG_PRINTF("[Controller] MAC check: %s\n", macMatches ? "OK (right device)" : "FAIL (wrong device)");

    if(!connectedCorrect) {
        // Check if the MAC address matches the allowed address (if configured)
        if (!macMatches) {
            LOG_PRINTF("CALLBACK: Rejected controller. Connected MAC %s does not match expected %s\n", addressStr, expectedMac);
            return;
        } else {
            bool foundEmptySlot = false;
            for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
                if (myControllers[i] == nullptr) {
                    connectedCorrect = true;
                    LOG_PRINTF("CALLBACK: Controller is connected, index=%d\n", i);
                    LOG_PRINTF("Controller model: %s, VID=0x%04x, PID=0x%04x\n",
                                ctl->getModelName().c_str(),
                                properties.vendor_id,
                                properties.product_id);
                    LOG_PRINTF("Bluetooth MAC address: %s (approved)\n", addressStr);
                    myControllers[i] = ctl;
                    ctl->setRumble(0x40, 250);
                    foundEmptySlot = true;
                    break;
                }
            }
            if (!foundEmptySlot) {
                LOG_PRINTLN("CALLBACK: Controller connected, but could not find empty slot");
            }
        }
    } else {
        if (!macMatches) {
            LOG_PRINTF("CALLBACK: Rejected controller. Connected MAC %s does not match expected %s\n", addressStr, expectedMac);
            return;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            LOG_PRINTF("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            connectedCorrect = false;
            return;
        }
    }
    
    LOG_PRINTLN("CALLBACK: Controller disconnected, but not found in myControllers");
}

void dumpGamepad(ControllerPtr ctl) {
    LOG_PRINTF(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        ctl->index(), ctl->dpad(), ctl->buttons(), ctl->axisX(), ctl->axisY(),
        ctl->axisRX(), ctl->axisRY(), ctl->brake(), ctl->throttle(),
        ctl->miscButtons(), ctl->gyroX(), ctl->gyroY(), ctl->gyroZ(),
        ctl->accelX(), ctl->accelY(), ctl->accelZ()
    );
}

void processControllerInput(ControllerPtr ctl) {
    static bool prevA = false, prevB = false, prevX = false, prevY = false;

    bool A = ctl->a();
    bool B = ctl->b();
    bool X = ctl->x();
    bool Y = ctl->y();

    // A - full brake / kill
    if (A && !prevA) {
        weaponHalfSpeedMode = false;
        manualMode = false;
        weaponFullSpeedMode = false;
        battlebot = false;
        weaponSpeed = 0;
        stopDriveMotors();
        LOG_PRINTLN("[A] KILL: full brake, manual OFF, weapon OFF");
    }

    // B - Manual mode toggle. Second press = STOP (same as A).
    if (B && !prevB) {
        if (!manualMode) {
            manualMode = true;
            weaponHalfSpeedMode = false;
            weaponFullSpeedMode = false;
            battlebot = false;
            LOG_PRINTLN("[B] Manual mode ON");
        } else {
            weaponHalfSpeedMode = false;
            manualMode = false;
            weaponFullSpeedMode = false;
            battlebot = false;
            weaponSpeed = 0;
            stopDriveMotors();
            LOG_PRINTLN("[B] STOP: full brake, manual OFF, weapon OFF");
        }
    }

    // X - weapon full speed toggle (uses weaponFullSpeedMode flag).
    if (X && !prevX) {
        if (weaponSpeed == WEAPON_SPEED_X) {
            weaponSpeed = 0;
            weaponFullSpeedMode = false;
            weaponHalfSpeedMode = false;
            LOG_PRINTLN("[X] Weapon OFF");
        } else {
            weaponSpeed = WEAPON_SPEED_X;
            weaponFullSpeedMode = true;
            weaponHalfSpeedMode = false;
            LOG_PRINTF("[X] Weapon speed set to %d\n", weaponSpeed);
        }
    }

    // Y - weapon half speed toggle (uses weaponHalfSpeedMode flag).
    if (Y && !prevY) {
        if (weaponSpeed == WEAPON_SPEED_Y) {
            weaponSpeed = 0;
            weaponHalfSpeedMode = false;
            weaponFullSpeedMode = false;
            LOG_PRINTLN("[Y] Weapon OFF");
        } else {
            weaponSpeed = WEAPON_SPEED_Y;
            weaponHalfSpeedMode = true;
            weaponFullSpeedMode = false;
            LOG_PRINTF("[Y] Weapon speed set to %d\n", weaponSpeed);
        }
    }

    // Update previous states
    prevA = A;
    prevB = B;
    prevX = X;
    prevY = Y;
}
void processConnectedControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processControllerInput(myController);
                drive(myController);
            } else {
                LOG_PRINTLN("Unsupported controller");
            }
        }
    }
}

// New signature: use left stick Y for forward/back and X for turning.
void computeDriveMotorSpeeds(int axisY, int axisX,
                             int& leftSpeed, bool& leftForward,
                             int& rightSpeed, bool& rightForward) {
    // forward/back from Y axis (invert if stick reports - when pushed forward)
    int forward = -axisY;

    // map -512..+512 -> -255..+255
    int speed = map(forward, -512, 512, -255, 255);
    speed = constrain(speed, -255, 255);

    // turning: cubic scaling on X axis as before
    float normTurn = axisX / 512.0f; // -1..1
    float smoothTurn = normTurn * normTurn * normTurn;
    int turn = static_cast<int>(smoothTurn * 512); // back to -512..512
    int turnEffect = map(turn, -512, 512, -255 + 32, 255 - 32);

    int rawLeft  = speed + turnEffect;
    int rawRight = speed - turnEffect;

    // deadzone / constrain
    if (abs(rawLeft) < JOYSTICK_DEADZONE) rawLeft = 0;
    else rawLeft = constrain(rawLeft, -255, 255);
    if (abs(rawRight) < JOYSTICK_DEADZONE) rawRight = 0;
    else rawRight = constrain(rawRight, -255, 255);

    leftForward  = rawLeft >= 0;
    rightForward = rawRight >= 0;
    leftSpeed  = abs(rawLeft);
    rightSpeed = abs(rawRight);
}

void drive(ControllerPtr ctl){
  if (!manualMode) {
    stopDriveMotors();
    return;
  }

  // Use left stick Y for forward/back and X for steering
  computeDriveMotorSpeeds(ctl->axisY(), ctl->axisX(),
                   leftSpeed, leftForward, rightSpeed, rightForward);

  // set motor directions based on forward flags
  digitalWrite(AIN1, leftForward ? HIGH : LOW);
  digitalWrite(AIN2, leftForward ? LOW  : HIGH);
  digitalWrite(BIN1, rightForward ? HIGH  : LOW); // right forward -> BIN1 LOW?
  digitalWrite(BIN2, rightForward ? LOW : HIGH);

  // and output PWM speeds
  analogWrite(PWMA, leftSpeed);
  analogWrite(PWMB, rightSpeed);
}

void printDriveDebug(){
  LOG_PRINTF("Throttle -> Left: %s %d | Right: %s %d\n",
              leftForward ? "FWD" : "REV", leftSpeed,
              rightForward ? "FWD" : "REV", rightSpeed);
}

void stopDriveMotors() {
  leftSpeed = 0;
  rightSpeed = 0;
  leftForward = true;
  rightForward = true;
  // drive pins low/zero PWM
  
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  LOG_PRINTLN("Motors stopped");
  
}