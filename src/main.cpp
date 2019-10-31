#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_camera.h>
#include <ESP32_FTPClient.h>
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

// Camera buffer, URL and picture name
camera_fb_t *fb = NULL;
String pic_name = "";
String pic_url  = "";

ESP32_FTPClient ftp (ftp_server, ftp_user, ftp_pass);

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

// Function prototypes
void deep_sleep(void);
void FTP_upload( void );
bool take_picture(void);

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

  // Take picture
 /* if( take_picture() ){
    FTP_upload();
    deep_sleep();
  }else{
    DBG("Capture failed, sleeping");
    deep_sleep();
  }

  if( millis() > CON_TIMEOUT){
    DBG("Timeout");
    deep_sleep();
  }*/
}

void task_check_connection(){
   if((millis() < wifi_timeout_limit) && !has_wifi){
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
 //  DBG(digitalRead(PIN_SHUTTER));
}

bool take_picture(){
  DBG("Taking picture now");
  fb = esp_camera_fb_get();  
  if(!fb){
    DBG("Camera capture failed");
    return false;
  }
  
  // Rename the picture with the time string
  // pic_name += String( now() ) + ".jpg";
  pic_name += "blah.jpg";
  DBG("Camera capture success, saved as:");
  DBG( pic_name );
  return true;
}

void FTP_upload(){
  DBG("Uploading via FTP");
 
  ftp.OpenConnection();
  
  //Create a file and write the image data to it;
  ftp.InitFile("Type I");
  ftp.ChangeWorkDir("/penthouse.designedbycave.co.uk/camera"); // change it to reflect your directory
  const char *f_name = pic_name.c_str();
  ftp.NewFile( f_name );
  ftp.WriteData(fb->buf, fb->len);
  ftp.CloseFile();

  // Breath, withouth delay URL failed to update.
  delay(100);
}
