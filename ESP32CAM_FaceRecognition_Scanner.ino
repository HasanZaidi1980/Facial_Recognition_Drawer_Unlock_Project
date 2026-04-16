#include "esp_camera.h"
#include <esp_now.h>
#include <WiFi.h>
#include "fd_forward.h"
#include "fr_forward.h"
 
// --- Hardware & Network ---
#define ENROLL_BUTTON  12
#define TRIGGER_BUTTON 13
#define INDICATOR_LED  4  // Your new breadboard LED!
 
uint8_t lockAddress[] = {0xF4, 0x2D, 0xC9, 0x6C, 0x46, 0x88};
esp_now_peer_info_t peerInfo;
 
// AI Thinker Pinout
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
 
static face_id_list id_list = {0};
static mtmn_config_t mtmn_config = {0};
dl_matrix3du_t *global_image_matrix = NULL;
dl_matrix3du_t *global_aligned_face = NULL;
 
bool is_scanning = false;
unsigned long scan_start_time = 0;
const unsigned long SCAN_TIMEOUT = 15000;
 
void setup() {
  Serial.begin(115200);
  pinMode(ENROLL_BUTTON, INPUT_PULLUP);
  pinMode(TRIGGER_BUTTON, INPUT_PULLUP);
 
  pinMode(INDICATOR_LED, OUTPUT);
  digitalWrite(INDICATOR_LED, LOW); // Ensure it starts completely OFF
 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
 
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed");
    return;
  }
 
  global_image_matrix = dl_matrix3du_alloc(1, 320, 240, 3);
  global_aligned_face = dl_matrix3du_alloc(1, 64, 64, 3);
 
  mtmn_config.type = FAST;
  mtmn_config.min_face = 100;
  mtmn_config.pyramid = 0.7;
  mtmn_config.pyramid_times = 3;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 10;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 5;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
 
  face_id_init(&id_list, 7, 5);
 
  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) {
    memcpy(peerInfo.peer_addr, lockAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
 
  Serial.println("SYSTEM READY. ASLEEP. Press Button 13 to Wake.");
}
 
void loop() {
  // --- 1. WAKE BUTTON ---
  if (digitalRead(TRIGGER_BUTTON) == LOW && !is_scanning) {
    is_scanning = true;
    scan_start_time = millis();
    Serial.println("SYSTEM AWAKE: Scanning for face...");
    while(digitalRead(TRIGGER_BUTTON) == LOW) delay(10);
  }
 
  // --- 2. ENROLL MODE ---
  if (digitalRead(ENROLL_BUTTON) == LOW) {
    is_scanning = false;
    Serial.println("ENROLLMENT INITIATED: Stand still...");
   
    // Turn the light ON to show enrollment is active
    digitalWrite(INDICATOR_LED, HIGH);
   
    int samples_left = 5;
   
    while (samples_left > 0) {
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb) {
        fmt2rgb888(fb->buf, fb->len, fb->format, global_image_matrix->item);
        box_array_t *net_boxes = face_detect(global_image_matrix, &mtmn_config);
       
        if (net_boxes) {
          if (align_face(net_boxes, global_image_matrix, global_aligned_face) == ESP_OK) {
            samples_left = enroll_face(&id_list, global_aligned_face);
            Serial.printf("Sample Taken! %d left.\n", samples_left);
           
            // --- SHUTTER BLINK ---
            digitalWrite(INDICATOR_LED, LOW); // Blink OFF
            delay(150);                    
            if (samples_left > 0) {
              digitalWrite(INDICATOR_LED, HIGH); // Turn back ON
            }
 
            if (samples_left == 0) Serial.println("ENROLLMENT COMPLETE!");
          }
        }
        esp_camera_fb_return(fb);
      }
      delay(300);
    }
   
    digitalWrite(INDICATOR_LED, LOW); // Off completely
    while(digitalRead(ENROLL_BUTTON) == LOW) delay(10);
    Serial.println("Returning to Sleep. Press Button 13 to Wake.");
  }
 
  // --- 3. SCANNING MODE ---
  if (is_scanning) {
    if (millis() - scan_start_time > SCAN_TIMEOUT) {
      Serial.println("TIMEOUT: No match found. Going back to sleep.");
      is_scanning = false;
      return;
    }
 
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
      fmt2rgb888(fb->buf, fb->len, fb->format, global_image_matrix->item);
      box_array_t *net_boxes = face_detect(global_image_matrix, &mtmn_config);
     
      if (net_boxes) {
        if (align_face(net_boxes, global_image_matrix, global_aligned_face) == ESP_OK) {
          int matched_id = recognize_face(&id_list, global_aligned_face);
         
          if (matched_id >= 0) {
            Serial.printf("MATCH! Face ID: %d\n", matched_id);
            Serial.println("Sending ESP-NOW UNLOCK signal...");
            const char *msg = "UNLOCK";
            esp_now_send(lockAddress, (uint8_t *)msg, strlen(msg));
           
            // --- DOUBLE BLINK ON MATCH ---
            digitalWrite(INDICATOR_LED, HIGH); delay(100);
            digitalWrite(INDICATOR_LED, LOW); delay(100);
            digitalWrite(INDICATOR_LED, HIGH); delay(100);
            digitalWrite(INDICATOR_LED, LOW);
           
            is_scanning = false;
            Serial.println("Drawer Unlocked. System returning to sleep.");
          } else {
            Serial.println("UNKNOWN FACE - Keep looking...");
          }
        }
      }
      esp_camera_fb_return(fb);
    }
    delay(200);
  }
}
