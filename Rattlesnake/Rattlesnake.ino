#include <Adafruit_MotorShield.h>

/*Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *motor = AFMS.getMotor(3);   // M3 terminals

void setup() {
  Serial.begin(9600);
  if (!AFMS.begin()) {
    Serial.println("Shield not found - check A4/A5/SDA/SCL pins");
    while (1);
  }
  Serial.println("Shield found");
  motor->setSpeed(255);   // full speed
  motor->run(RELEASE);
}*/

//void loop() {}

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *leftMotor  = AFMS.getMotor(3);  // M1 terminals
Adafruit_DCMotor *rightMotor = AFMS.getMotor(2);  // M2 terminals

const byte PHOTO_TL = A0;  // left photodiode
const byte PHOTO_TR = A1;  // right photodiode (don't use A4/A5)
const int threshold = 500; // below this = dark enough, stop
const int BASE_SPEED = 150; // cruising speed, 0-255

// PID state: global so findDarkness() can use it
float integral = 0;
float prevError = 0;
unsigned long prevTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PHOTO_TR, INPUT);
  pinMode(PHOTO_TL, INPUT);
  if (!AFMS.begin()) {
    Serial.println("Shield not found");
    while (1);
  }
  leftMotor->run(RELEASE);
  rightMotor->run(RELEASE);
  prevTime = millis();
}

void findDarkness(const byte leftPin, const byte rightPin, const int lThreshold, const int baseSpeed) {
  // --- Timing ---
  unsigned long now = millis();
  float dt = (now - prevTime) / 1000.0;  // seconds since last call
  if (dt <= 0) return;
  prevTime = now;

  // --- Sensors ---
  float left = analogRead(leftPin);
  float right = analogRead(rightPin);
  Serial.println(left);
  float error = left - right;            // setpoint is 0: both sides equally bright

  // --- Found darkness: stop ---
  //if (left < lThreshold && right < lThreshold) {
  if (left < lThreshold) {
    leftMotor->run(RELEASE);
    rightMotor->run(RELEASE);
    integral = 0;
    prevError = error;
    return;
  }

  // --- PID ---
  float kp = 0.1;
  float ki = 0.05;
  float kd = 0.001;

  integral += error * dt;
  integral = constrain(integral, -500, 500);   // anti-windup
  float derivative = (error - prevError) / dt;
  prevError = error;

  float output = kp * error + ki * integral + kd * derivative;
  //int speed = int(output);
  int speed = left / 5;

  // --- Drive motors ---
  leftMotor->setSpeed(constrain(baseSpeed + speed, 0, 255));
  rightMotor->setSpeed(constrain(baseSpeed - speed, 0, 255));
  leftMotor->run(FORWARD);
  rightMotor->run(FORWARD);
}

void loop() {
  findDarkness(PHOTO_TL, PHOTO_TR, threshold, BASE_SPEED);
  delay(10);
}