
//PWM pins
const int PWM_A = 23;
const int PWM_B = 22;

//ESC PWM
const int ESC_PWM = 6; //ESC PWM pin for the weapon

//Driver input pins
const int AIN_2 = 21;
const int AIN_1 = 20;
const int BIN_2 = 19;
const int BIN_1 = 18;

//Encoder pins
const int MB_EB = 0; //Motor B - Encoder B
const int MB_EA = 1; //Motor B - Encoder A
const int MA_EB = 2; //Motor A - Encoder B
const int MA_EA = 3; //Motor A - Encoder A

//State for GPIO toggle
int state1;
int state2;
int state3;
int state4;

void setup() {
  Serial.begin(115200);

  pinMode(PWM_A,OUTPUT);
  pinMode(PWM_B,OUTPUT);
  pinMode(AIN_2,OUTPUT);
  pinMode(AIN_1,OUTPUT);
  pinMode(BIN_2,OUTPUT);
  pinMode(BIN_1,OUTPUT);
  //pinMode(ESC_PWM,OUTPUT);

  pinMode(MB_EB,INPUT);
  pinMode(MB_EA,INPUT);
  pinMode(MA_EB,INPUT);
  pinMode(MA_EA,INPUT);

  /*PWM
  ledcSetup(0, 5000, 8); //PWM channel 0, 5000Hz, resoliton 8 = 0-255 bits
  ledcSetup(1, 5000, 8); //PWM channel 1, 5000Hz, resoliton 8 = 0-255 bits
  ledcAttachPin(PWM_A, 0);
  ledCAttachPin(PWM_B, 1);
  */

  //Write
  digitalWrite(AIN_2, 1);
  digitalWrite(BIN_2, 1);
  digitalWrite(AIN_1, 0);
  digitalWrite(BIN_1, 0);
  //ledcWrite(0, 128);
  //ledcWrite(1, 128);
}

void loop() {
  for (int i=1;i<26;i++){
    analogWrite(PWM_A, i*10);
    analogWrite(PWM_B, i*10);
    state1 = digitalRead(MB_EB);
    state2 = digitalRead(MB_EA);
    state3 = digitalRead(MA_EB);
    state4 = digitalRead(MA_EA);
    Serial.print("First: ");
    Serial.println(state1);
    Serial.println(state2);
    Serial.print("Second: ");
    Serial.println(state3);
    Serial.println(state4);
    delay(200);
  }
  delay(2000);
}
