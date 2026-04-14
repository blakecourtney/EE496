#include "esp_camera.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "drone_model_quant.h" 

uint8_t *tensor_arena = NULL;
const int arena_size = 1024 * 1024;

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
// HardwareSerial DroneSerial(1);
#define MESH_TX_PIN 43
#define MESH_RX_PIN 44

// TFLite Globals
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
tflite::ErrorReporter* error_reporter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

float detect_thresh = .8;
uint8_t detect_count = 0;
char detect_flag = 0;
char detect_ack_flag = 0;
uint8_t frame_capture = NULL;
float probability_capture;

void setup() {
  // Serial 0: For your PC/Python Script (High Speed)
  Serial.begin(115200);

  // Serial 1: For sending data to the Mesh ESP32 (Baud rate: 115200)
  DroneSerial.begin(115200, SERIAL_8N1, MESH_RX_PIN, MESH_TX_PIN);

  // Camera Init
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
  config.xclk_freq_hz = 10000000; 
  config.pixel_format = PIXFORMAT_RGB565; 
  config.frame_size = FRAMESIZE_240X240;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (!psramFound()) {
      Serial.println("FATAL: PSRAM not found");
      return;
  } else {
      Serial.printf("PSRAM found. Free: %d bytes\n", ESP.getFreePsram());
  }
  
  config.jpeg_quality = 10; // 0-63, lower is higher quality
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
      Serial.println("OV3660 Init Failed!");
      return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
      s->set_vflip(s, 1);      // Flip vertically if the image is upside down
      s->set_hmirror(s, 1);    // Mirror horizontally for more natural drone view
      s->set_brightness(s, 1); // Boost brightness slightly for AI detection
  }


  // TFLite Init
  // allocate memory for tensor, put in PSRAM because it doesn't fit in SRAM
  // tensor_arena = (uint8_t *)heap_caps_malloc(arena_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  tensor_arena = (uint8_t *)heap_caps_aligned_alloc(16, arena_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (tensor_arena == NULL) {
      Serial.println("Arena allocation failed!");
      return;
  }
  // Check alignment - TFLite on S3 needs 16-byte alignment
  if ((uintptr_t)tensor_arena % 16 != 0) {
      Serial.println("Warning: Arena not 16-byte aligned. Acceleration might fail.");
  }

  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  model = tflite::GetModel(g_model_data);
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, arena_size, error_reporter);
  
  interpreter = &static_interpreter;
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors Failed!");
    return;
  }
  input = interpreter->input(0);
  output = interpreter->output(0);
}

void loop() {
  if (!detect_flag){
      // camera / model stuff
  }    
  
  if (detect_flag == 1){
    // Send to Mesh ESP32 (via Hardware Serial wires) ---
    DroneSerial.println("human detected!\n");
  }
  
  
  while (Serial.available () > 0)
    processSerialByte(Serial.read());

  if (detect_ack_flag){
    // start transmitting frame_capture
    Serial.println("START_IMAGE");
    size_t chunk_size = 2048; // Send in 2KB blocks
    uint8_t *buffer_ptr = ->buf;

    for (size_t i = 0; i < total_bytes; i += chunk_size) {
      size_t bytes_to_send = (total_bytes - i < chunk_size) ? (total_bytes - i) : chunk_size;
      Serial.write(buffer_ptr + i, bytes_to_send);
      Serial.flush();
    }
    Serial.println("END_IMAGE");
    Serial.printf("PROBABILITY: %.2f\n", score);
  }

}


void processSerialByte(const byte ){
  static char input_line [5];
  static unsigned int input_pos = 0;

  switch (inByte){
    case '\n':   // end of text
      input_line [input_pos] = 0;  // terminating null byte
      
      if (input_line.compare("ACK") == 0){
        detect_ack_flag = 1;
      }

      if (input_line.compare("CLEAR") == 0){
        detect_flag = 0;
      }
    
      input_pos = 0;  
      break;

    case '\r':   // discard carriage return
      break;

    default:
      if (input_pos < (MAX_INPUT - 1))
        input_line [input_pos++] = inByte;
      break;
    }   
} 

