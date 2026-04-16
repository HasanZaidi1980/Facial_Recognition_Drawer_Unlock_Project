#include <WiFi.h>
#include <esp_now.h>
 
// --- Hardware Setup ---
// Change this to whatever pin you connect your Relay to
#define RELAY_PIN 4
 
// Callback function: This runs automatically whenever a message is caught in the air
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Convert the raw bytes back into text
  char msg[len + 1];
  memcpy(msg, incomingData, len);
  msg[len] = '\0';
 
  Serial.printf("Signal Caught: %s\n", msg);
 
  // Verify the secret password
  if (strcmp(msg, "UNLOCK") == 0) {
    Serial.println("ACCESS GRANTED! Opening Drawer...");
   
    digitalWrite(RELAY_PIN, HIGH); // Trigger the Relay to pop the lock
    delay(3000);                   // Keep it open for 3 seconds
    digitalWrite(RELAY_PIN, LOW);  // Turn off relay to lock it again
   
    Serial.println("Drawer Secured.");
  } else {
    Serial.println("Unknown command ignored.");
  }
}
 
void setup() {
  Serial.begin(115200);
 
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start in the "Locked" position
 
  // ESP-NOW requires the WiFi radio to be turned on
  WiFi.mode(WIFI_STA);
 
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
 
  // Tell the ESP32 what function to run when it hears a message
  esp_now_register_recv_cb(OnDataRecv);
 
  Serial.println("LOCK BOARD ONLINE. Listening for the Camera...");
}
 
void loop() {
  // The ESP-NOW receiver works in the background using interrupts.
  // We don't need anything in the main loop!
  delay(1000);
}
