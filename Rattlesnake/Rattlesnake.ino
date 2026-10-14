#include <Adafruit_MotorShield.h>
#include <Servo.h>

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
Adafruit_DCMotor *leftMotor  = AFMS.getMotor(1);  // M1 terminals
Adafruit_DCMotor *rightMotor = AFMS.getMotor(4);  // M4 terminals

const byte PHOTO_TL = A0;  // left photodiode
const byte PHOTO_TR = A1;  // right photodiode (don't use A4/A5)
const byte IR = A2; // IR sensor
const uint8_t RATTLE = 3; // rattle servo port
const int lightThreshold = 600; // below this = dark enough, stop
const int BASE_SPEED = 50; // cruising speed, 0-255
const int rattleThreshold = 600; // furthest distance where it will rattle, in mm
const int lungeThreshold = 250; // furthest distance where it will lunge, in mm
const int RATTLE_CENTER = 50; // center of oscilation of rattle
const int RATTLE_AMPLITUDE = 75; // how far the rattle will go from RATTLE_CENTER, ing deg
const float RATTLE_SPEED = 0.004; // scale the rattle's speed from ms to rad
const int LUNGE_TIME = 1000; // the time the snake will take to lunge, in ms
Servo rattleServo; // the servo that controls the rattle

const uint8_t HIDING = 0; // for when the snake is trying to go somewhere darker
const uint8_t WAITING = 1; // for when the snake is not in motion
const uint8_t LUNGING = 2; // for when the snake is lunging at a predator

uint8_t state = HIDING;
uint16_t rattlingAmount = 0;
unsigned long lungeStart;

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
  rattleServo.attach(RATTLE);
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
  Serial.println("light: " + left);
  float error = left - right;            // setpoint is 0: both sides equally bright

  // --- Found darkness: stop ---
  //if (left < lThreshold && right < lThreshold) {
  if (left < lThreshold) {
    leftMotor->run(RELEASE);
    rightMotor->run(RELEASE);
    integral = 0;
    prevError = error;
    state = WAITING;
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
  int motorSpeed = baseSpeed + speed;
  Serial.println("unclamped: " + motorSpeed);
  motorSpeed = constrain(motorSpeed, 0, 100);
  Serial.println("clamped: " + motorSpeed);
  leftMotor->setSpeed(motorSpeed);
  rightMotor->setSpeed(motorSpeed);
  leftMotor->run(BACKWARD);
  rightMotor->run(BACKWARD);
}

void watchForPredators() {
  // Scan with the IR sensor.
  uint16_t scan1 = analogRead(IR);
  uint16_t scan2 = analogRead(IR);
  uint16_t scan3 = analogRead(IR);

  // Calculate the distance in milimeters.
  float voltage = min(min(scan1, scan2), scan3);
  voltage = voltage * 5 / 1024;
  int distance = 94.6 * pow(voltage, 4) +
    -722.6 * pow(voltage, 3) +
    2121.1 * pow(voltage, 2) +
    -3072.9 * voltage +
    2228.1;
  distance = constrain(distance, 200, 1000);
  if(voltage == 0) {
    distance = 1000;
  }
  Serial.println(distance);

  // Rattle if too close
  if(distance < rattleThreshold) {
    rattlingAmount = RATTLE_AMPLITUDE;
  }
  else {
    rattlingAmount = 0;
  }

  // Lunge if too close
  if(distance < lungeThreshold) {
    state = LUNGING;
    rattlingAmount = 0;
    lungeStart = millis();
  }
}

void loop() {
  Serial.println(state);
  // Execute state specific actions:
  if(state == HIDING) {
    // If hiding, go backwards into the cave
    findDarkness(PHOTO_TL, PHOTO_TR, lightThreshold, BASE_SPEED);
  }
  else if(state == WAITING) {
    // If waiting, check for predators
    watchForPredators();

    // Move the rattle
    uint16_t rattlePos = RATTLE_CENTER + rattlingAmount * sin(float(millis()) * RATTLE_SPEED);
    rattleServo.write(rattlePos);
  }
  else if(state == LUNGING) {
    // If lunging, move forward at max speed for LUNGE_TIME then start hiding
    if(millis() - lungeStart < LUNGE_TIME) {
      leftMotor->setSpeed(225);
      rightMotor->setSpeed(225);
      leftMotor->run(FORWARD);
      rightMotor->run(FORWARD);
    }
    else {
      state = HIDING;
      leftMotor->setSpeed(0);
      rightMotor->setSpeed(0);
    }
  }
  else {
    Serial.println("No Vlaid State!");
  }

  // Delay before the next frame
  delay(10);
}