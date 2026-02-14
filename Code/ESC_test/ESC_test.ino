#include <Servo.h>

//ESC control with servo object
Servo esc;

void setup() {
  Serial.begin(115200);         // Serial for debug
  esc.attach(18, 1000, 2000);   // Connect ESC signal to Pin 18 with a MIN of 1000us and a MAX of 2000us
  esc.writeMicroseconds(1000);  // Write the off signal of 1000us on-time, this will trigger the arming sequence

  delay(10000);                 // The ESC firmware is slow so the arming takes a long time
                                // In reality you don't have a delay and have the esc.write on a button which
                                // you only touch after the arming process
}

void loop() {
  esc.write(180);               // esc.write(180) == esc.writeMicroseconds(2000), meaning full speed
  delay(3000);                  // Random test delay of 3s of full speed and 5s of off
  // Stop
  esc.writeMicroseconds(1000);  // This will assign 1000us on-time meaning stop
  delay(5000);                  // 5s wait before loop start
}