#include "esp_camera.h"
#include "TensorFlowLite.h"
#include "drone_model_quant.h" 

// Configure the camera pins
#define PWDN_GPIO_NUM    -1 // Power down pin not used
#define RESET_GPIO_NUM   -1 // Reset pin not used
#define XCLK_GPIO_NUM    0  // External clock pin
#define SIOD_GPIO_NUM    26 // I2C data pin
#define SIOC_GPIO_NUM    27 // I2C clock pin
#define Y9_GPIO_NUM      35 // Camera data pins
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25 // Vertical sync pin
#define HREF_GPIO_NUM    23 // Horizontal reference pin
#define PCLK_GPIO_NUM    22 // Pixel clock pin

// model stuff
const int kInputSize = (128,128,3);
const int kOutputSize = 1;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize the camera
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

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  
  // Load the TensorFlow Lite model
  static tflite::MicroErrorReporter micro_error_reporter;
  const tflite::Model* model = tflite::GetModel(gesture_model_tflite);
 

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);

  interpreter = &static_interpreter;
 
  // Allocate memory
  interpreter->AllocateTensors();
}

void loop() {
  // put your main code here, to run repeatedly:
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }

  // Clear any text in the buffer
  Serial.println("START_IMAGE"); 
  Serial.write(fb->buf, fb->len);
  Serial.println("END_IMAGE");
  esp_camera_fb_return(fb);

  delay(2000);
}
