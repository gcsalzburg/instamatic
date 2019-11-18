#include "Arduino.h"

#include <WiFi.h>
#include <esp_http_client.h>
#include <esp_camera.h>

#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>
#include <CAVE_Tasks.h>

// Passwords and private config variables
#include "config.h"

// Define debug interface over UART1
#define DBG_noln(x) Serial.print(x)
#define DBG(x) Serial.println(x)

// Pin definitions
#define PIN_SHUTTER 2
#define PIN_FLASH 4
#define PIN_LED 13

// Switch definitions
#define DEBOUNCE_INTERVAL   25
Bounce shutter = Bounce(PIN_SHUTTER,DEBOUNCE_INTERVAL);

// Camera buffer, URL and picture name
camera_fb_t *fb = NULL;
String pic_name = "";
String pic_url  = "";

// Wifi
const uint16_t wifi_timeout_limit = 10*1000;
bool has_wifi = false;

// LED control
typedef struct{
	uint16_t on;
	uint16_t off;
	uint32_t colour;
}Light_t;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

Light_t light_boot = {300,300,strip.Color(255,255,0)};
Light_t light_ready = {3000,30,strip.Color(0,255,0)};
Light_t light_capture = {1000,0,strip.Color(255,255,255)};
Light_t light_upload = {150,0,strip.Color(200,0,200)};
Light_t *light_state;   // Which light is it currently showing

// Function prototypes
void set_led_state(Light_t *new_state);
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
   {task_check_buttons,    100}   // xxx    
};

void setup(){
   Serial.begin(115200);
   Serial.setDebugOutput(true);

   // Register tasks
   CAVE::tasks_register(loop_tasks);

   // Setup pins
   pinMode(PIN_FLASH, OUTPUT);
   pinMode(PIN_LED, OUTPUT);
   pinMode(PIN_SHUTTER, INPUT_PULLUP);
   digitalWrite(PIN_FLASH, LOW);
   set_led_state(&light_boot);

   // Setup LED
   strip.begin();
   strip.setBrightness(20); // max = 255
   strip.setPixelColor(0,0,0,0);
   strip.show(); // Initialize all pixels to 'off'

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
   //init with high specs to pre-allocate larger buffers
   if (psramFound()) {
      config.frame_size = FRAMESIZE_UXGA;
      config.jpeg_quality = 10;
      config.fb_count = 2;
   } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.jpeg_quality = 12;
      config.fb_count = 1;
   }

   // camera init
   esp_err_t err = esp_camera_init(&config);
   if (err != ESP_OK) {
      DBG("Camera init failed with error 0x%x");
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
   DBG("[[ instamatic ]]");
   DBG("");
   DBG_noln("Connecting to: "); DBG(ssid);
   WiFi.begin(ssid, pass);
}

// Main loop timer
void loop(){
	// Call task updater
	CAVE::tasks_update();
}

// Sets the LED to the new state flasher
void set_led_state(Light_t *new_state){
   light_state = new_state;
   task_flash_led();
}

// Checks if wifi connection has been set yet
void task_check_connection(){
   if((millis() < wifi_timeout_limit) && !has_wifi){
      DBG_noln(".");
      if(WiFi.status() == WL_CONNECTED){
         DBG("");
         if( !WiFi.isConnected() ){
            DBG("Failed to connect to WiFi.");
         }else{
            has_wifi = true; 
            DBG_noln("WiFi connected on: "); DBG(WiFi.localIP());
            set_led_state(&light_ready);
         }
      }
   }
}

// Flashes LED in given timing
void task_flash_led(){
   if( (millis()%(light_state->off+light_state->on)) <= light_state->on){
      strip.setPixelColor(0,light_state->colour);
   }else{
      strip.setPixelColor(0,0,0,0);
   }
   strip.show();
}

// Check for button presses on shutter etc... 
void task_check_buttons(){
   shutter.update();
	if(shutter.fell()){
      DBG("Shutter pressed");
      if(has_wifi){
         take_send_photo();
      }
  }
}


// Handler for HTTP events
esp_err_t _http_event_handler(esp_http_client_event_t *evt){
   switch (evt->event_id) {
      case HTTP_EVENT_ERROR:
         DBG(" > HTTP_EVENT_ERROR");
         break;
      case HTTP_EVENT_ON_CONNECTED:
         DBG(" > HTTP_EVENT_ON_CONNECTED");
         break;
      case HTTP_EVENT_HEADER_SENT:
         DBG(" > HTTP_EVENT_HEADER_SENT");
         break;
      case HTTP_EVENT_ON_HEADER:
         //DBG(" > HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
         break;
      case HTTP_EVENT_ON_DATA:
         DBG(" > HTTP_EVENT_ON_DATA");
         if (!esp_http_client_is_chunked_response(evt->client)) {
            // Write out data
            DBG_noln(" > > ");
            DBG((char*)evt->data);
         }
         break;
      case HTTP_EVENT_ON_FINISH:
         DBG(" > HTTP_EVENT_ON_FINISH");
         break;
      case HTTP_EVENT_DISCONNECTED:
         DBG(" > HTTP_EVENT_DISCONNECTED");
         break;
  }
  return ESP_OK;
}

// Captures a picture
static esp_err_t take_send_photo(){
   set_led_state(&light_capture);
   DBG("Taking picture...");
   camera_fb_t * fb = NULL;
   esp_err_t res = ESP_OK;

   fb = esp_camera_fb_get();
   if (!fb) {
      DBG("Camera capture failed");
      return ESP_FAIL;
   }
   set_led_state(&light_upload);

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
      DBG_noln(" > HTTP_CLIENT_STATUS_CODE");
      DBG(esp_http_client_get_status_code(http_client));
   }

   esp_http_client_cleanup(http_client);

   esp_camera_fb_return(fb);
   set_led_state(&light_ready);

   return res;
}