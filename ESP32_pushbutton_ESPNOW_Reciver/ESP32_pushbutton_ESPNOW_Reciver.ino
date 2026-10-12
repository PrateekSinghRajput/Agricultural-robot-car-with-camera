#include <esp_now.h>
#include <WiFi.h>

// 4 Push Buttons for Robot Control
#define fwd 14
#define rev 27
#define lef 26
#define rit 25 

// 3 Push Buttons for INDEPENDENT Control (Toggle Action)
#define btn_servo1 18 // Controls Servo 1
#define btn_servo2 19 // Controls Servo 2
#define btn_relay  4

// Status LED Pin 
#define led_pin 15

// NEW MAC ADDRESS (From your Receiver board)
uint8_t broadcastAddress[] = {0xA4, 0xF0, 0x0F, 0x5B, 0x00, 0xD8}; 

typedef struct struct_message {
  int x;
  int y;
  int swi;     // Now used for the Relay Module
  int front;
  int back;
  int left;
  int right;
  int speed;
} struct_message;
struct_message data;

esp_now_peer_info_t peerInfo;

// ---- TOGGLE STATE VARIABLES ----
bool servo1_state = false; // Keeps track of Servo 1 (ON/OFF)
bool servo2_state = false; // Keeps track of Servo 2 (ON/OFF)
bool relay_state  = false; // Keeps track of Relay Module (ON/OFF)

bool last_btn1_state = HIGH;   // Used to detect button press
bool last_btn2_state = HIGH;   // Used to detect button press
bool last_btn_relay_state = HIGH; // Used to detect button press
// --------------------------------

// NEW CALLBACK SIGNATURE for ESP32 Core 3.x
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    digitalWrite(led_pin, HIGH);
  } else {
    digitalWrite(led_pin, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Setup the Status LED
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  // Setup Movement Button Pins
  pinMode(fwd, INPUT_PULLUP);
  pinMode(rev, INPUT_PULLUP);
  pinMode(lef, INPUT_PULLUP);
  pinMode(rit, INPUT_PULLUP); 
  
  // Setup Toggle Button Pins
  pinMode(btn_servo1, INPUT_PULLUP);
  pinMode(btn_servo2, INPUT_PULLUP);
  pinMode(btn_relay, INPUT_PULLUP);
}

void loop() {

  // 1. Read the 4 movement buttons
  data.front = digitalRead(fwd);
  data.back  = digitalRead(rev);
  data.left  = digitalRead(lef);
  data.right = digitalRead(rit);

  // 2. Read the Buttons for TOGGLE action
  bool current_btn1 = digitalRead(btn_servo1);
  bool current_btn2 = digitalRead(btn_servo2);
  bool current_btn_relay = digitalRead(btn_relay);

  // If Button 1 is pressed (changed from HIGH to LOW) -> Toggle Servo 1
  if (current_btn1 == LOW && last_btn1_state == HIGH) {
    servo1_state = !servo1_state; 
  }
  
  // If Button 2 is pressed (changed from HIGH to LOW) -> Toggle Servo 2
  if (current_btn2 == LOW && last_btn2_state == HIGH) {
    servo2_state = !servo2_state; 
  }

  // If Relay Button is pressed (changed from HIGH to LOW) -> Toggle Relay
  if (current_btn_relay == LOW && last_btn_relay_state == HIGH) {
    relay_state = !relay_state; 
  }

  // Save the button states for the next loop
  last_btn1_state = current_btn1;
  last_btn2_state = current_btn2;
  last_btn_relay_state = current_btn_relay;

  // 3. Assign to data structure (Sends 1 if state is true, 0 if false)
  data.x   = servo1_state ? 1 : 0; // x controls Servo 1
  data.y   = servo2_state ? 1 : 0; // y controls Servo 2
  data.swi = relay_state  ? 1 : 0; // swi controls Relay Module

  // 4. Set unused variables
  data.speed = 255; // Maximum speed

  // Print values to Serial Monitor for testing
  Serial.print("Fwd:"); Serial.print(data.front);
  Serial.print(" Rev:"); Serial.print(data.back);
  Serial.print(" Lef:"); Serial.print(data.left);
  Serial.print(" Rit:"); Serial.print(data.right);
  Serial.print(" | Servo1 (x):"); Serial.print(data.x);
  Serial.print(" | Servo2 (y):"); Serial.print(data.y);
  Serial.print(" | Relay (swi):"); Serial.println(data.swi);

  // Send the data package over Wi-Fi
  esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));
  
  // A 50ms delay acts as "debounce" so the button doesn't accidentally trigger twice
  delay(50);
}