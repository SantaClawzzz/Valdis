#line 1 "C:\\Users\\Public\\Documents\\Altium\\Valdis\\Code\\Valdis_v0\\Valdis\\xbox.cpp"
//#include "TANK_PRO_MAX.ino"
#include <Arduino.h>
#include "xbox.h"
#include "Constants.h"
#include <ESP32Encoder.h>

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
// ESP32Encoder objects

ESP32Encoder encoder;
ESP32Encoder encoder2;

unsigned long lastEncoderTime = 0;
const unsigned long ENCODER_SAMPLE_INTERVAL = 100; // ms
const unsigned long CORRECTION_INTERVAL = 100; // ms
const int CPR = 485; // Counts per revolution for the encoders
const int PID_P_GAIN = 2; // step to reduce PWM by when correcting
int appliedLeftPWM = 0;
int appliedRightPWM = 0;
unsigned long lastCorrectionTime = 0;

bool connectedCorrect = false;

// Initialize and attach ESP32Encoder instances
void initEncoders() {

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    
    // attachHalfQuad(pinA, pinB)
    encoder.attachHalfQuad(MBEB, MBEA);
    // attach right encoder with MAEA, MAEB (A then B). If your wiring swaps A/B, invert order.
    encoder2.attachHalfQuad(MAEA, MAEB);

    // start counts at zero
    encoder.clearCount();
    encoder2.clearCount();

    lastEncoderTime = millis();
    LOG_PRINTLN("ESP32Encoder: attached half-quad encoders");
}

// Sample encoder counts and log pulses/RPM
void updateEncoderSpeeds() {
    unsigned long now = millis();
    if (now - lastEncoderTime < ENCODER_SAMPLE_INTERVAL) return;

    long leftCount = encoder.getCount();
    long rightCount = encoder2.getCount();
    LOG_PRINTF("encL: %ld, enderR: %ld\n", leftCount, rightCount);

    static long prevLeft = 0;
    static long prevRight = 0;

    prevLeft = leftCount;
    prevRight = rightCount;
    lastEncoderTime = now;
}

// Simple decrease-only motor matching using encoder counts
void correctMotorSpeeds(int& leftPWM, int& rightPWM) {
    unsigned long now = millis();
    unsigned long deltaTime = now - lastCorrectionTime;
    if (deltaTime < CORRECTION_INTERVAL) return;
    lastCorrectionTime = now;

    // PID constants
    float Kp = 1.0; // Proportional gain

    // Defines prev values as static, so they persist across loops
    static long prevLeft = 0;
    static long prevRight = 0;

    // Get current encoder counts
    long leftCount = encoder.getCount();
    long rightCount = encoder2.getCount();

    // Calculate change, meaning the error
    long dLeft = leftCount - prevLeft;
    long dRight = rightCount - prevRight;

    // Finished calculating with prev values, so now the prev values are updated
    prevLeft = leftCount;
    prevRight = rightCount;

    uint8_t actualLeftPWM = map(dLeft, 0, 350, 0, 255);
    uint8_t actualRightPWM = map(dRight, 0, 350, 0, 255);

    // Calculate error and correction
    if (actualLeftPWM < leftPWM && leftForward && rightForward) {
        leftPWM = leftPWM + (leftPWM - actualLeftPWM) * Kp; // Scale correction by how far off we are
    } else if (actualRightPWM < rightPWM && leftForward && rightForward) {
        rightPWM = rightPWM + (rightPWM - actualRightPWM) * Kp; // Scale correction by how far off we are
    } else if (actualLeftPWM > leftPWM && !leftForward && !rightForward) {
        leftPWM = leftPWM - (actualLeftPWM - leftPWM) * Kp; // Scale correction by how far off we are
    } else if (actualRightPWM > rightPWM && !leftForward && !rightForward) {
        rightPWM = rightPWM - (actualRightPWM - rightPWM) * Kp; // Scale correction by how far off we are
    }

    // Prevent negative PWM
    appliedLeftPWM  = max(0, leftPWM);
    appliedRightPWM = max(0, rightPWM);
    LOG_PRINTF("Correction applied: leftPWM=%d, rightPWM=%d\n", appliedLeftPWM, appliedRightPWM);
}


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

// Use throttle for forward, brake for backward, and left stick X for turning.
void computeDriveMotorSpeeds(int throttle, int brake, int axisX,
                             int& leftSpeed, bool& leftForward,
                             int& rightSpeed, bool& rightForward) {
    // throttle (0..1023) for forward, brake (0..1023) for backward
    int speed = map(throttle, 0, 1023, 0, 255) - map(brake, 0, 1023, 0, 255);

    // constrain to -255..+255
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
    // Use throttle/brake for forward/back and left stick X for steering
    computeDriveMotorSpeeds(ctl->throttle(), ctl->brake(), ctl->axisX(),
                                                     leftSpeed, leftForward, rightSpeed, rightForward);


    // Run correction which may decrease the faster motor (transient)
    correctMotorSpeeds(leftSpeed, rightSpeed);

    // set motor directions based on forward flags
    digitalWrite(AIN1, rightForward ? LOW : HIGH);
    digitalWrite(AIN2, rightForward ? HIGH : LOW);
    digitalWrite(BIN1, leftForward ? LOW : HIGH);
    digitalWrite(BIN2, leftForward ? HIGH : LOW);

    // and output PWM speeds (applied values)
    analogWrite(PWMA, appliedRightPWM);
    analogWrite(PWMB, appliedLeftPWM);

}

void printDriveDebug(){
  LOG_PRINTF("Throttle -> Left: %s %d | Right: %s %d\n",
              leftForward ? "FWD" : "REV", appliedLeftPWM,
              rightForward ? "FWD" : "REV", appliedRightPWM);
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