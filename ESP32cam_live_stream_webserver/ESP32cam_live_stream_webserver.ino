/*
 * Make sure adjust Tools settings to,
 * Tools >> Board "AI Thinker ESP32-Cam"
 * Partiotion scheme "Huge aPP (3MB No OTA / 1MB SPIFFS)"
*/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// Replace with your network credentials(WiFi)
const char* ssid = "---";                // name
const char* password = "---";    // pswd

// Pin definition for CAMERA_MODEL_AI_THINKER -- Has PSRAM
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

#define LED_FLASH_PIN      4   // Pin to control LED flash

// Function to start the camera server
void startCameraServer();

// Function to configure camera settings
void configureCameraSettings();

// Global variable to hold the last captured frame
camera_fb_t *fb = NULL;  

// Function to stream video frames
esp_err_t stream_handler(httpd_req_t *req);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Initialize LED flash pin
  pinMode(LED_FLASH_PIN, OUTPUT);
  digitalWrite(LED_FLASH_PIN, LOW);  // Ensure the LED is off initially

  // Configure LED PWM
  ledcAttachPin(LED_FLASH_PIN, LEDC_CHANNEL_0);
  ledcSetup(LEDC_CHANNEL_0, 5000, 8); // 5 kHz PWM, 8-bit resolution, LED brightness
  ledcWrite(LEDC_CHANNEL_0, 0); // Set 0 to turn off the LED, 255 for full brightness

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;       // Use valid timer (0-3)
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 7;                    // Adjust streaming quality
  config.fb_count = 1;

  if (psramFound()) {                         // Has PSRAM
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 7;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 7;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Configure Camera Settings
  configureCameraSettings();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the camera server
  startCameraServer();

  Serial.print("Camera Ready! Go to: http://");
  Serial.print(WiFi.localIP());   // Camera IP that can access via web browser
  Serial.println("/");
}

void loop() {
  delay(1);
}

// Configure camera settings
void configureCameraSettings() {
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_HVGA);     // HVGA 480x320
    s->set_quality(s, 5);                    // Adjust Quality 5
    s->set_brightness(s, 0);                 // Brightness (-2 - +2)
    s->set_contrast(s, -1);                   // Contrast 0
    s->set_saturation(s, 0);                 // Saturation 0
    s->set_special_effect(s, 0);             // No effects
    s->set_whitebal(s, 1);                   // Enable AWB
    s->set_awb_gain(s, 1);                   // Enable AWB gain
    s->set_wb_mode(s, 0);                    // Auto WB mode
    s->set_exposure_ctrl(s, 1);              // Enable AEC
    s->set_aec2(s, 1);                       // Enable AEC sensor
    s->set_ae_level(s, 0);                   // AE Level 0
    s->set_gain_ctrl(s, 1);                  // Enable AGC
    s->set_agc_gain(s, 4);                   // AGC gain ceiling 4x (DO NOT CHANGE, App crashed) 
    s->set_bpc(s, 0);                        // Disable BPC
    s->set_wpc(s, 1);                        // Enable WPC
    s->set_raw_gma(s, 1);                    // Enable Raw GMA
    s->set_lenc(s, 1);                       // Enable lens correction
    s->set_hmirror(s, 1);                    // Enable horizontal mirror
    s->set_vflip(s, 0);                      // Disable vertical flip
    s->set_dcw(s, 1);                        // Enable downsize
    s->set_colorbar(s, 0);                   // Disable color bar
    // You can set other parameters here as needed
  }
}

// HTTP handler for streaming video frames
esp_err_t stream_handler(httpd_req_t *req) {
  char part_buf[64];
  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
  static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
    if (httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)) != ESP_OK ||
        httpd_resp_send_chunk(req, (const char *)part_buf, hlen) != ESP_OK ||
        httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len) != ESP_OK) {
      esp_camera_fb_return(fb);
      break;
    }
    esp_camera_fb_return(fb);
  }
  return ESP_OK;
}

// Function to start the camera server
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 8192;  // Increase stack size if needed

  httpd_uri_t stream_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &stream_uri);
  }
}
