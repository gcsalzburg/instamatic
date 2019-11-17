#include "Arduino.h"

#include <WiFi.h>
#include <esp_http_client.h>
#include <esp_camera.h>

#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>
#include <CAVE_Tasks.h>

// Passwords and private config variables
#include "config.h"

// Enable Debug interface and serial prints over UART1
#define DEGUB_ESP
#ifdef DEGUB_ESP
  #define DBG_noln(x) Serial.print(x)
  #define DBG(x) Serial.println(x)
#else 
  #define DBG(...)
#endif

// Pin definitions
#define PIN_SHUTTER 16
#define PIN_LED 13

#define DEBOUNCE_INTERVAL   25

// Camera buffer, URL and picture name
camera_fb_t *fb = NULL;
String pic_name = "";
String pic_url  = "";

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

// Wifi
const uint16_t wifi_timeout_limit = 10*1000;
bool has_wifi = false;

// LED control
typedef struct{
	uint16_t on;
	uint16_t off;
	uint32_t colour;
}Light_t;
Light_t light_boot = {300,300,strip.Color(255,255,0)};
Light_t light_wifi = {3000,30,strip.Color(0,255,0)};

Light_t *light_state;

// Buttons
Bounce shutter = Bounce(PIN_SHUTTER,DEBOUNCE_INTERVAL);

const char *post_url = "https://penthouse.designedbycave.co.uk/upload.php?code=12345678"; // Location where images are POSTED

// Function prototypes
void deep_sleep(void);
bool take_picture(void);
static esp_err_t take_send_photo(void);
esp_err_t _http_event_handler(esp_http_client_event_t *evt);

// Task function prototypes
void task_check_connection();
void task_flash_led();
void task_check_buttons();

// Table of tasks
CAVE::Task loop_tasks[] = {
   {task_check_connection, 500}, // Delay between connection tests
   {task_flash_led,        16},  // 50fps      
   {task_check_buttons,    30}   // xxx    
};

void setup(){
  #ifdef DEGUB_ESP
    Serial.begin(115200);
    Serial.setDebugOutput(true);
  #endif

  // Register tasks
  CAVE::tasks_register(loop_tasks);

  // Setup pins
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SHUTTER, INPUT_PULLUP);
  light_state = &light_boot;

  // Setup LED
  strip.begin();
  strip.setBrightness(20); // max = 255
  strip.setPixelColor(0,0,0,0);
  strip.show(); // Initialize all pixels to 'off'

  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  //init with high specs to pre-allocate larger buffers
  config.frame_size = FRAMESIZE_XGA; // set picture size, FRAMESIZE_XGA = 1024x768
  config.jpeg_quality = 10;          // quality of JPEG output. 0-63 lower means higher quality
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK){
    Serial.print("Camera init failed with error 0x%x");
    DBG(err);
    return;
  }

  // Change extra settings if required
  //sensor_t * s = esp_camera_sensor_get();
  //s->set_vflip(s, 0);       //flip it back
  //s->set_brightness(s, 1);  //up the blightness just a bit
  //s->set_saturation(s, -2); //lower the saturation
  
  // Enable timer wakeup for ESP32 sleep
  //esp_sleep_enable_timer_wakeup( TIME_TO_SLEEP );

   DBG("");
   DBG("instamatic :-)");
   DBG("");
   DBG_noln("Connecting to: "); DBG(ssid);
   WiFi.begin(ssid, pass);
}

void loop(){
	// Call task updater
	CAVE::tasks_update();
}

void task_check_connection(){
   if((millis() < wifi_timeout_limit) && !has_wifi){
      DBG_noln(".");
      if(WiFi.status() == WL_CONNECTED){
         if( !WiFi.isConnected() ){
            DBG("Failed to connect to WiFi.");
         }else{
            has_wifi = true; 
            DBG_noln("WiFi connected on: "); DBG(WiFi.localIP());
            light_state = &light_wifi;
         }
      }
   }
}

void task_flash_led(){
   if( (millis()%(light_state->off+light_state->on)) <= light_state->on){
      strip.setPixelColor(0,light_state->colour);
   }else{
      strip.setPixelColor(0,0,0,0);
   }
   strip.show();
}

void task_check_buttons(){
   shutter.update();
	if(shutter.fell()){
      DBG("Shutter pressed");
      take_send_photo();
   }
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt){
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        Serial.printf("%.*s", evt->data_len, (char*)evt->data);
      }
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("");
      Serial.println("HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}

static esp_err_t take_send_photo(){
  Serial.println("Taking picture...");
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }

  esp_http_client_handle_t http_client;
  
  esp_http_client_config_t config_client = {0};
  config_client.url = post_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg");

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);

  esp_camera_fb_return(fb);
}