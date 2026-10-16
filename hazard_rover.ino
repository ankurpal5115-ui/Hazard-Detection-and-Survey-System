#include <SoftwareSerial.h>

// --- Bluetooth Pins ---
// HC-05 TXD -> Pin D3, HC-05 RXD -> Pin D4
SoftwareSerial BTSerial(3, 4);

// --- 2-Motor Driver Pins ---
const int IN1 = 5;  // Left Motor Forward
const int IN2 = 6;  // Left Motor Reverse
const int IN3 = 9;  // Right Motor Forward
const int IN4 = 10; // Right Motor Reverse

// --- Ultrasonic Sensor Pins ---
const int TRIG_PIN = 7;
const int ECHO_PIN = 8;

// --- Flame Sensor & Indicator ---
const int FLAME_PIN = 2;  // Digital output from flame module
const int LED_FIRE = 11;  // Flame warning LED

// --- Safety Thresholds ---
const int OBSTACLE_DISTANCE_CM = 20;

// Runtime variables
char currentCommand = 'S';
unsigned long lastPingTime = 0;
float currentDistance = 100.0;

// --- 2-Wheel Differential Drive Primitives ---
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// Ultrasonic measurement helper
float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0) return 999.0;

  return (duration * 0.0343) / 2.0;
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(LED_FIRE, OUTPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopMotors();
  digitalWrite(LED_FIRE, LOW);

  Serial.begin(9600);
  BTSerial.begin(9600);

  Serial.println("2-Motor Hazard Rover Firmware Online.");
}

void loop() {
  // 1. Read Flame Status (Active LOW)
  bool fireDetected = (digitalRead(FLAME_PIN) == LOW);

  // 2. Measure Ultrasonic Distance every 60ms
  if (millis() - lastPingTime >= 60) {
    lastPingTime = millis();
    currentDistance = getDistanceCm();
  }

  // 3. Receive Bluetooth Commands
  if (BTSerial.available() > 0) {
    currentCommand = BTSerial.read();
    Serial.print("Command: ");
    Serial.println(currentCommand);
  }

  // Priority 1: Fire Hazard Cutoff
  if (fireDetected) {
    stopMotors();
    digitalWrite(LED_FIRE, HIGH);
    currentCommand = 'S';
    BTSerial.println("ALERT: FIRE DETECTED! Motors Halted.");
    delay(40);
    return;
  } else {
    digitalWrite(LED_FIRE, LOW);
  }

  // Priority 2: Collision Detection
  bool obstacleAhead =
      (currentDistance > 0 &&
       currentDistance <= OBSTACLE_DISTANCE_CM);

  // Priority 3: Steering Execution
  switch (currentCommand) {
    case 'F':
    case 'f':
      if (obstacleAhead) {
        stopMotors();
        BTSerial.println("BLOCKED: Obstacle Ahead!");
      } else {
        moveForward();
      }
      break;

    case 'B':
    case 'b':
      moveBackward();
      break;

    case 'L':
    case 'l':
      turnLeft();
      break;

    case 'R':
    case 'r':
      turnRight();
      break;

    case 'S':
    case 's':
    default:
      stopMotors();
      break;
  }

  delay(20);
}
