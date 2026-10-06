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
const byte IR = A2 // IR sensor
const int lightThreshold = 500; // below this = dark enough, stop
const int BASE_SPEED = 150; // cruising speed, 0-255
const int rattleThreshold = 500; // furthest distance where it will rattle, in mm
const int lungeThreshold = 150; // furthest distance where it will lunge, in mm

const uint8_t HIDING = 0; // for when the snake is trying to go somewhere darker
const uint8_t WAITING = 1; // for when the snake is not in motion
const uint8_t LUNGING = 2; // for when the snake is lunging at a predator

uint8_t state = HIDING;
uint16_t rattlingSpeed = 0;
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
  leftMotor->setSpeed(constrain(baseSpeed + speed, 0, 255));
  rightMotor->setSpeed(constrain(baseSpeed - speed, 0, 255));
  leftMotor->run(FORWARD);
  rightMotor->run(FORWARD);
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

  // Rattle if too close
  if distance < rattleThreshold {
    rattlingSpeed = rattleThreshold - distance
  }

  // Lunge if too close
  if distance < lungeThreshold {
    state = LUNGING
    rattlingSpeed = 0
    lungeStart = millis()
  }
}

void loop() {
  if state == HIDING {
    findDarkness(PHOTO_TL, PHOTO_TR, lightThreshold, BASE_SPEED);
  }
  else if state == WAITING {
    watchForPredators();
  }
  else if state == LUNGING {
    if millis() - lungeStart < lungeTime {
      leftMotor->setSpeed(225);
      rightMotor->setSpeed(225);
      leftMotor->run(BACKWARD);
      rightMotor->run(BACKWARD);
    }
    else {
      state = HIDING;
      leftMotor->setSpeed(0);
      rightMotor->setSpeed(0);
    }
  }
  else {
    Serial.println("No Vlaid State!")
  }
  delay(10);
}