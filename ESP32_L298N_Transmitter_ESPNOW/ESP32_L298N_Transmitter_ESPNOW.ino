#include <ESP32Servo.h>
#include <esp_now.h>
#include <WiFi.h>

// Motor Driver Pins
#define in1 25
#define in2 26
#define in3 27
#define in4 14

// Relay Pin
#define rely1 33

// Servo Pins
#define sev1 19
#define sev2 18

Servo s1;
Servo s2;

// --- SERVO 1 CONFIGURATION ---
const int servo1_TargetAngle = 90; // Change this to the angle you want Servo 1 to stop at

// --- SERVO 2 SWEEP VARIABLES ---
unsigned long sweepPreviousMillis = 0;
const long sweepInterval = 15; // Speed of the sweep (15ms per 1 degree). Lower is faster.
int servo2Pos = 0;             // Current position of Servo 2
int servo2Step = 1;            // Direction of sweep (1 for up, -1 for down)
// -----------------------------

typedef struct struct_message {
  int x;
  int y;
  int swi;
  int front;
  int back;
  int left;
  int right;
  int speed;
} struct_message;
struct_message data;

// CALLBACK SIGNATURE for ESP32 Core 3.x
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  // 1. MOTOR DIRECTION LOGIC
  if(data.back == 0){
    digitalWrite(in1,HIGH);
    digitalWrite(in2,LOW);
    digitalWrite(in3,HIGH);
    digitalWrite(in4,LOW);
  }
  else if(data.front == 0){
    digitalWrite(in1,LOW);
    digitalWrite(in2,HIGH);
    digitalWrite(in3,LOW);
    digitalWrite(in4,HIGH);
  }
  else if(data.right == 0){
    digitalWrite(in1,LOW);
    digitalWrite(in2,HIGH);
    digitalWrite(in3,HIGH);
    digitalWrite(in4,LOW);
  }
  else if(data.left == 0){
    digitalWrite(in1,HIGH);
    digitalWrite(in2,LOW);
    digitalWrite(in3,LOW);
    digitalWrite(in4,HIGH);
  }
  else{
    digitalWrite(in1,LOW);
    digitalWrite(in2,LOW);
    digitalWrite(in3,LOW);
    digitalWrite(in4,LOW);
  }
  
  // 2. RELAY LOGIC (Assuming standard Active-Low relay module)
  if (data.swi == 1){
    digitalWrite(rely1, LOW);  // Relay ON
  }
  else{
    digitalWrite(rely1, HIGH); // Relay OFF
  }

  // Debug Print to Serial Monitor
  Serial.print("Servo 1 (x): "); Serial.print(data.x);
  Serial.print(" | Servo 2 (y): "); Serial.print(data.y);
  Serial.print(" | Relay (swi): "); Serial.println(data.swi);
}
 
void setup() {
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  delay(200);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // PRINT THIS BOARD'S MAC ADDRESS
  Serial.println();
  Serial.println("-----------------------------------");
  Serial.print("THIS BOARD'S MAC ADDRESS: ");
  Serial.println(WiFi.macAddress());
  Serial.println("-----------------------------------");
  Serial.println();

  esp_now_register_recv_cb(OnDataRecv);

  // Setup Outputs
  pinMode(in1,OUTPUT);
  pinMode(in2,OUTPUT);
  pinMode(in3,OUTPUT);
  pinMode(in4,OUTPUT);
  pinMode(rely1,OUTPUT);

  // Default relay to OFF state on boot
  digitalWrite(rely1, HIGH); 

  s1.attach(sev1);
  s2.attach(sev2);

  // Start both servos at 0 degrees
  s1.write(0);
  s2.write(0);
}
 
void loop() {
  
  // --- SERVO 1 LOGIC (Fixed Angle) ---
  if (data.x == 1) {
    s1.write(servo1_TargetAngle); // Go to 90 degrees (or whatever you set above)
  } else {
    s1.write(0); // Return to 0 degrees
  }


  // --- SERVO 2 LOGIC (Continuous Sweep 0 to 180) ---
  if (data.y == 1) {
    
    unsigned long currentMillis = millis();
    
    // Check if it's time to move Servo 2 by 1 degree
    if (currentMillis - sweepPreviousMillis >= sweepInterval) {
      sweepPreviousMillis = currentMillis;
      
      servo2Pos = servo2Pos + servo2Step; // Move by 1 step
      
      // If we reach the limits (180 or 0), reverse the direction
      if (servo2Pos >= 180) {
        servo2Pos = 180;
        servo2Step = -1; // Change direction to go down
      } else if (servo2Pos <= 0) {
        servo2Pos = 0;
        servo2Step = 1; // Change direction to go up
      }
      
      // Update Servo 2 position
      s2.write(servo2Pos);
    }
    
  } else {
    // If data.y is 0, gently return Servo 2 to 0 and hold it there
    if (servo2Pos != 0) {
      servo2Pos = 0;
      servo2Step = 1; // Reset step direction for next time
      s2.write(0);
    }
  }

}