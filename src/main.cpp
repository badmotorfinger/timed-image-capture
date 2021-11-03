#define uS_TO_S_FACTOR 1000000ULL         /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60                 /* Time ESP32 will go to sleep (in seconds) */
#define CAMERA_MODEL_AI_THINKER           // Has PSRAM

#include "camera_pins.h"
#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"

// Set board to either ESP32-Dev or ESP32-Wrover Module
// Set baude rate to 460800
// Set partition scheme to Huge APP

void writeFile(fs::FS &fs, const char* rootPath, const char * pathTemplate, const char * extension, uint8_t *buf, size_t len) {

    Serial.printf("Writing file with %d bytes to SD card...\n", len);

    int32_t pictureNumber = 0;

    File counterFile = fs.open("/counter");
    if (!counterFile)
    {
      counterFile = fs.open("/counter", FILE_WRITE);
      counterFile.print("0");
      counterFile.close();
    }
    else
    {
      char counter[4];
      counterFile.readBytes(counter, 4);
      pictureNumber = atoi(counter) + 1;
      counterFile.close();
    }

    File root = fs.open(rootPath);
    if(!root){
      Serial.println("Failed to open directory");
      return;
    }

    String path = String(rootPath) + String(pathTemplate) + "-" + pictureNumber + "." + String(extension);

    Serial.printf("Writing file: %s\n", path.c_str());

    File file = fs.open(path.c_str(), FILE_WRITE);
    if(!file){
        Serial.printf("Failed to open file %s for writing", path.c_str());
        return;
    }

    if(file.write(buf, len)){
        Serial.printf("File %s written \n", path.c_str());
    } else {
        Serial.printf("Writing file %s failed", path.c_str());
    }
    file.close();

    counterFile = fs.open("/counter", FILE_WRITE);
    counterFile.print(pictureNumber);
    counterFile.close();
}

esp_err_t takeSnapshot()
{
  esp_err_t res = ESP_OK;
  camera_fb_t * fb = NULL;

  fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
  }

  writeFile(SD_MMC, "/", "capture", "jpg", fb->buf, fb->len);

  esp_camera_fb_return(fb);

  return res;
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  delay(1000); // Allow serial monitor to start
  Serial.setDebugOutput(true);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  Serial.print("Initialising camera...");

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

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
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
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  Serial.println("Initialised");

  //Serial.println("Starting SD Card");
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }

  takeSnapshot();
}

void loop() {
  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();
}
