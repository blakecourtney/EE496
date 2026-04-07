#include "esp_camera.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "drone_model_quant.h" 

// Freenove ESP32-S3 Pin Map
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y9_GPIO_NUM    16
#define Y8_GPIO_NUM    17
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    10
#define Y4_GPIO_NUM    8
#define Y3_GPIO_NUM    9
#define Y2_GPIO_NUM    11
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

// Hardware Serial for Board-to-Board Communication
// Using pins 43 and 44 as they are usually free on the Freenove S3
HardwareSerial DroneSerial(1);
#define MESH_TX_PIN 43
#define MESH_RX_PIN 44

// TFLite Globals
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
tflite::ErrorReporter* error_reporter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

const size_t tensor_arena_size = 100 * 1024;
uint8_t tensor_arena[tensor_arena_size];

void setup() {
  // Serial 0: For your PC/Python Script (High Speed)
  Serial.begin(2000000);

  // Serial 1: For sending data to the Mesh ESP32 (Baud rate: 115200)
  DroneSerial.begin(115200, SERIAL_8N1, MESH_RX_PIN, MESH_TX_PIN);

  // 1. Camera Init
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
  
  // TFLite requires 96x96 Grayscale
  config.frame_size = FRAMESIZE_96X96;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  // 2. TFLite Init
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  model = tflite::GetModel(g_model_data);
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, tensor_arena_size, error_reporter);
  
  interpreter = &static_interpreter;
  if (interpreter->AllocateTensors() != kTfLiteOk) return;
  input = interpreter->input(0);
  output = interpreter->output(0);
}

void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  // Load image into TFLite input
  for (int i = 0; i < fb->len; i++) {
    input->data.f[i] = fb->buf[i] / 255.0f; 
  }

  // Run Inference
  if (interpreter->Invoke() == kTfLiteOk) {
    float score = output->data.f[0];
    
    // --- 1. Send to PC (via USB) ---
    Serial.println("START_JPEG");
    Serial.write(fb->buf, fb->len);
    Serial.println("END_JPEG");
    Serial.printf("PROBABILITY: %.2f\n", score);

    // --- 2. Send to Mesh ESP32 (via Hardware Serial wires) ---
    // The other ESP32 will use Serial1.readStringUntil() to grab this
    DroneSerial.println("START_JPEG");
    DroneSerial.write(fb->buf, fb->len);
    DroneSerial.println("END_JPEG");
    DroneSerial.printf("PROBABILITY: %.2f\n", score);
  }

  esp_camera_fb_return(fb);
}