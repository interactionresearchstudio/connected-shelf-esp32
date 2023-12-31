#include <Arduino.h>
#include <main.h>
#include <pins.h>
#include <WiFi.h>
//#include <DNSServer.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "util.h"
#include "json.h"


String AP_SSID = "CS";
#define PASSWORD "12345678"
#define DNS_PORT 53

#define VERBOSE

String namePrefix = "";
String serverName = "irs-iot.ddns.net";   //
String serverPath = "/upload";     // The default serverPath should be upload.php
const int serverPort = 80; //server port for HTTPS
//WiFiClientSecure client;
WiFiClient client;

const IPAddress apIP(192, 168, 2, 1);
const IPAddress gateway(255, 255, 255, 0);

//DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket websocket("/ws");
#define MAX_NUM_CLIENTS 16
AsyncWebSocketClient *clients[MAX_NUM_CLIENTS];

StaticJsonDocument<512> json;

//UI Variables
bool wifiScanSend = false;
bool connectedStatusSend = false;

unsigned long noneBlockingWifiConnectStartTime;
unsigned long prevWifiMillis;
bool isNBConnecting = false;
unsigned long wifiCheckMillis;


void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void isWiFiScanReady();
void checkWebsocketRequests();
void isConnectedStatusReady();
void noneblockingWiFiConnect();
void noneblockingWiFiHandler();
void setupAPSSID();
void wifiStatusCheck();

int setFramesize(sensor_t * s, int val);
int getFramesize(sensor_t * s);

typedef struct {
        camera_fb_t * fb;
        size_t index;
} camera_frame_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";

static const char * JPG_CONTENT_TYPE = "image/jpeg";
static const char * BMP_CONTENT_TYPE = "image/x-windows-bmp";

class AsyncBufferResponse: public AsyncAbstractResponse {
    private:
        uint8_t * _buf;
        size_t _len;
        size_t _index;
    public:
        AsyncBufferResponse(uint8_t * buf, size_t len, const char * contentType){
            _buf = buf;
            _len = len;
            _callback = nullptr;
            _code = 200;
            _contentLength = _len;
            _contentType = contentType;
            _index = 0;
        }
        ~AsyncBufferResponse(){
            if(_buf != nullptr){
                free(_buf);
            }
        }
        bool _sourceValid() const { return _buf != nullptr; }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override{
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            memcpy(buffer, _buf+index, maxLen);
            if((index+maxLen) == _len){
                free(_buf);
                _buf = nullptr;
            }
            return maxLen;
        }
};

class AsyncFrameResponse: public AsyncAbstractResponse {
    private:
        camera_fb_t * fb;
        size_t _index;
    public:
        AsyncFrameResponse(camera_fb_t * frame, const char * contentType){
            _callback = nullptr;
            _code = 200;
            _contentLength = frame->len;
            _contentType = contentType;
            _index = 0;
            fb = frame;
        }
        ~AsyncFrameResponse(){
            if(fb != nullptr){
                esp_camera_fb_return(fb);
            }
        }
        bool _sourceValid() const { return fb != nullptr; }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override{
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            memcpy(buffer, fb->buf+index, maxLen);
            if((index+maxLen) == fb->len){
                esp_camera_fb_return(fb);
                fb = nullptr;
            }
            return maxLen;
        }
};

class AsyncJpegStreamResponse: public AsyncAbstractResponse {
    private:
        camera_frame_t _frame;
        size_t _index;
        size_t _jpg_buf_len;
        uint8_t * _jpg_buf;
        uint64_t lastAsyncRequest;
    public:
        AsyncJpegStreamResponse(){
            _callback = nullptr;
            _code = 200;
            _contentLength = 0;
            _contentType = STREAM_CONTENT_TYPE;
            _sendContentLength = false;
            _chunked = true;
            _index = 0;
            _jpg_buf_len = 0;
            _jpg_buf = NULL;
            lastAsyncRequest = 0;
            memset(&_frame, 0, sizeof(camera_frame_t));
        }
        ~AsyncJpegStreamResponse(){
            if(_frame.fb){
                if(_frame.fb->format != PIXFORMAT_JPEG){
                    free(_jpg_buf);
                }
                esp_camera_fb_return(_frame.fb);
            }
        }
        bool _sourceValid() const {
            return true;
        }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            if(!_frame.fb || _frame.index == _jpg_buf_len){
                if(index && _frame.fb){
                    uint64_t end = (uint64_t)micros();
                    int fp = (end - lastAsyncRequest) / 1000;
                   // log_printf("Size: %uKB, Time: %ums (%.1ffps)\n", _jpg_buf_len/1024, fp);
                    lastAsyncRequest = end;
                    if(_frame.fb->format != PIXFORMAT_JPEG){
                        free(_jpg_buf);
                    }
                    esp_camera_fb_return(_frame.fb);
                    _frame.fb = NULL;
                    _jpg_buf_len = 0;
                    _jpg_buf = NULL;
                }
                if(maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + strlen(JPG_CONTENT_TYPE) + 8)){
                    //log_w("Not enough space for headers");
                    return RESPONSE_TRY_AGAIN;
                }
                //get frame
                _frame.index = 0;

                _frame.fb = esp_camera_fb_get();
                if (_frame.fb == NULL) {
                    log_e("Camera frame failed");
                    return 0;
                }

                if(_frame.fb->format != PIXFORMAT_JPEG){
                    unsigned long st = millis();
                    bool jpeg_converted = frame2jpg(_frame.fb, 80, &_jpg_buf, &_jpg_buf_len);
                    if(!jpeg_converted){
                        log_e("JPEG compression failed");
                        esp_camera_fb_return(_frame.fb);
                        _frame.fb = NULL;
                        _jpg_buf_len = 0;
                        _jpg_buf = NULL;
                        return 0;
                    }
                    log_i("JPEG: %lums, %uB", millis() - st, _jpg_buf_len);
                } else {
                    _jpg_buf_len = _frame.fb->len;
                    _jpg_buf = _frame.fb->buf;
                }

                //send boundary
                size_t blen = 0;
                if(index){
                    blen = strlen(STREAM_BOUNDARY);
                    memcpy(buffer, STREAM_BOUNDARY, blen);
                    buffer += blen;
                }
                //send header
                size_t hlen = sprintf((char *)buffer, STREAM_PART, JPG_CONTENT_TYPE, _jpg_buf_len);
                buffer += hlen;
                //send frame
                hlen = maxLen - hlen - blen;
                if(hlen > _jpg_buf_len){
                    maxLen -= hlen - _jpg_buf_len;
                    hlen = _jpg_buf_len;
                }
                memcpy(buffer, _jpg_buf, hlen);
                _frame.index += hlen;
                return maxLen;
            }

            size_t available = _jpg_buf_len - _frame.index;
            if(maxLen > available){
                maxLen = available;
            }
            memcpy(buffer, _jpg_buf+_frame.index, maxLen);
            _frame.index += maxLen;

            return maxLen;
        }
};

void initCamSettings(){
   sensor_t * s = esp_camera_sensor_get();
   int setting = 0;
    if(s != NULL){
      setting = getSetting("framesize");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = 14; 
        setFramesize(s, setting);
        setSetting("framesize", setting);
      }
      setFramesize(s,setting);

      setting = getSetting("quality");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.quality;
        setSetting("quality", setting);
      }
      s->set_quality(s, setting);

      setting = getSetting("contrast");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.contrast;
        setSetting("contrast", setting);
      }
      s->set_contrast(s, setting);

      setting = getSetting("brightness");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.brightness;
        setSetting("brightness", setting);
      }
      s->set_brightness(s, setting);

      setting = getSetting("saturation");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.saturation;
        setSetting("saturation", setting);
      }
      s->set_saturation(s, setting);
    
      setting = getSetting("sharpness");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.sharpness;
        setSetting("sharpness", setting);
      }
      s->set_sharpness(s, setting);
    
      setting = getSetting("awb");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.awb;
        setSetting("awb", setting);
      }
      s->set_whitebal(s, setting);

      setting = getSetting("hmirror");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.hmirror;
        setSetting("hmirror", setting);
      }
      s->set_hmirror(s, setting);

      setting = getSetting("vflip");
      if(setting == 99999){
        //means it's not been set before
        #ifdef VERBOSE
          Serial.println("setting not set. Settings with current cam setting");
        #endif
        setting = s->status.vflip;
        setSetting("vflip", setting);
      }
      s->set_vflip(s, setting);
    }
}

void saveCamSettings(){
     sensor_t * s = esp_camera_sensor_get();
     setSetting("framesize",(int)getFramesize(s));
     setSetting("quality",s->status.quality);
     setSetting("brightness",s->status.brightness);
     setSetting("contrast",s->status.contrast);
     setSetting("saturation",s->status.saturation);
     setSetting("sharpness",s->status.sharpness);
     setSetting("awb",s->status.awb);
     setSetting("hmirror",s->status.hmirror);
     setSetting("vflip",s->status.vflip);
     setSetting("specialEffect",s->status.special_effect);
     setSetting("awb_gain",s->status.awb_gain);
     setSetting("aec",s->status.aec);
     setSetting("aec2",s->status.aec2);
     setSetting("awb_gain",s->status.awb_gain);
     setSetting("ae_level",s->status.ae_level);
     setSetting("aec_value",s->status.aec_value);
     setSetting("agc",s->status.agc);
     setSetting("agc_gain",s->status.agc_gain);
}


void getCameraStatus(AsyncWebServerRequest *request){
    static char json_response[1024];

    sensor_t * s = esp_camera_sensor_get();
    if(s == NULL){
        request->send(501);
        return;
    }
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"framesize\":%u,",getSetting("framesize"));
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p+=sprintf(p, "\"awb\":%u,", s->status.awb);
    p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p+=sprintf(p, "\"aec\":%u,", s->status.aec);
    p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p+=sprintf(p, "\"denoise\":%u,", s->status.denoise);
    p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p+=sprintf(p, "\"agc\":%u,", s->status.agc);
    p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p+=sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    *p++ = '}';
    *p++ = 0;

    AsyncWebServerResponse * response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void setCameraVar(AsyncWebServerRequest *request){
  Serial.println("VAR");
    if(!request->hasArg("var") || !request->hasArg("val")){
      Serial.println("404");
        request->send(404);
        return;
    }
    String var = request->arg("var");
    const char * variable = var.c_str();
    int val = atoi(request->arg("val").c_str());

    sensor_t * s = esp_camera_sensor_get();
    if(s == NULL){
        request->send(501);
        return;
    }


    int res = 0;
    if(!strcmp(variable, "framesize")) res = setFramesize(s, val);
    else if(!strcmp(variable, "quality")) res = s->set_quality(s, val);
    else if(!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
    else if(!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
    else if(!strcmp(variable, "saturation")) res = s->set_saturation(s, val);
    else if(!strcmp(variable, "sharpness")) res = s->set_sharpness(s, val);
    else if(!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);
    else if(!strcmp(variable, "awb")) res = s->set_whitebal(s, val);
    else if(!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
    else if(!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
    else if(!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);
    else if(!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);
    else if(!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);
    else if(!strcmp(variable, "aec2")) res = s->set_aec2(s, val);
    else if(!strcmp(variable, "denoise")) res = s->set_denoise(s, val);
    else if(!strcmp(variable, "dcw")) res = s->set_dcw(s, val);
    else if(!strcmp(variable, "bpc")) res = s->set_bpc(s, val);
    else if(!strcmp(variable, "wpc")) res = s->set_wpc(s, val);
    else if(!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);
    else if(!strcmp(variable, "lenc")) res = s->set_lenc(s, val);
    else if(!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);
    else if(!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);
    else if(!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);
    else if(!strcmp(variable, "savesetting")) saveCamSettings();
    else if(!strcmp(variable, "reset")) ESP.restart();

    else {
        log_e("unknown setting %s", var.c_str());
        request->send(404);
        return;
    }
    log_d("Got setting %s with value %d. Res: %d", var.c_str(), val, res);

    AsyncWebServerResponse * response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}





String getContentType(String filename);

void setupCam();
void saveCamSettings();
String sendPhoto();

void setupAP();
void setupSTA();
void setupWiFi();
void setupServer();
void deserialiseJson(uint8_t *data, size_t len);


//WIFI variables
unsigned long wifiConnectStartTime;
bool isAP = false;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(4000);
  setupAPSSID();
  initPreferences();
  initButton();
  setupCam();
  initCamSettings();


#ifdef VERBOSE
  Serial.println("Connected Shelf ESP32 CAM");
#endif
  setupWiFi();
}

unsigned long prevMillis = 1800000;

void loop() {
  if(isAP){
   // dnsServer.processNextRequest();
    checkWebsocketRequests();
    noneblockingWiFiHandler();
    wifiStatusCheck();
  } else {
    if(millis() - prevMillis > 1800000){
      prevMillis = millis();
      sendPhoto();
    }
  }
  if(digitalRead(USER_BUTTON) == false && isAP == false){
    Serial.println("initialising ap");
    setupAP();
    setupServer();
  }
  vTaskDelay(1);
}

void setupWiFi(){
  if(isCredentials() == false){
    #ifdef VERBOSE 
      Serial.println("Setting up access point...");
    #endif
    setupAP();
    setupServer();
  } else {
    setupSTA();
  }
}

void setupSTA(){
  #ifdef VERBOSE
    Serial.println("Setting up WIFI.");
  #endif
  WiFi.disconnect();   //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_STA);

  WiFi.begin(getSSID().c_str(),getPass().c_str());
  wifiConnectStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiConnectStartTime) <= 8000) // try for 5 seconds
    {
      delay(500);
      Serial.print(".");
    }
  if(WiFi.status() != WL_CONNECTED){
    #ifdef VERBOSE
      Serial.println("Could not connect to WiFi, reverting to Access Point");
    #endif
    setInternetStatus(false);
    setupAP();
    setupServer();
  } else {
     #ifdef VERBOSE
      Serial.println("Connected!");
      Serial.println(WiFi.localIP());
    #endif
      setInternetStatus(true);
  }
}

void setupAP() {
  isAP = true;
#ifdef VERBOSE
  Serial.println("Setting up access point...");
#endif
  WiFi.disconnect();   //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_AP_STA);
  
 // dnsServer.start(DNS_PORT, "*", apIP);
 if(isCredentials()){
  WiFi.begin(getSSID().c_str(),getPass().c_str());
  wifiConnectStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiConnectStartTime) <= 8000) // try for 5 seconds
    {
      delay(500);
      Serial.print(".");
    }
  if(WiFi.status() != WL_CONNECTED){
    #ifdef VERBOSE
      Serial.println("Could not connect to WiFi, reverting to Access Point");
    #endif
    setInternetStatus(false);
  } else {
     #ifdef VERBOSE
      Serial.println("Connected!");
      Serial.println(WiFi.localIP());
    #endif
    setInternetStatus(true);
  }
 }
 #ifndef PASSWORD
  WiFi.softAP(AP_SSID);
#else
  WiFi.softAP(AP_SSID, PASSWORD);
#endif
  WiFi.softAPConfig(apIP, apIP, gateway);
}

void setupServer() {
#ifdef VERBOSE
  Serial.println("Setting up web server...");
#endif
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // bind websocket to async web server
  websocket.onEvent(wsEventHandler);
  server.addHandler(&websocket);

  // Root
 //camera stream
  server.on("/control", HTTP_GET, setCameraVar);
  server.on("/status", HTTP_GET, getCameraStatus);
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();
    if(!response){
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
});


server.on("/sendjpg", HTTP_GET, [](AsyncWebServerRequest *request)
{
  Serial.println("sending jpg");
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (fb == NULL) {
        Serial.println("Camera frame failed");
        request->send(501);
        return;
    }

    if(fb->format == PIXFORMAT_JPEG){
        AsyncFrameResponse * response = new AsyncFrameResponse(fb, JPG_CONTENT_TYPE);
        if (response == NULL) {
             Serial.println("Response alloc failed");
            request->send(501);
            return;
        }
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }

    size_t jpg_buf_len = 0;
    uint8_t * jpg_buf = NULL;
    unsigned long st = millis();
    bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
    esp_camera_fb_return(fb);
    if(!jpeg_converted){
        log_e("JPEG compression failed: %lu", millis());
        request->send(501);
        return;
    }
    log_i("JPEG: %lums, %uB", millis() - st, jpg_buf_len);

    AsyncBufferResponse * response = new AsyncBufferResponse(jpg_buf, jpg_buf_len, JPG_CONTENT_TYPE);
    if (response == NULL) {
        log_e("Response alloc failed");
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
});

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
#ifdef VERBOSE
    Serial.println("Serving file:  /index.html");
#endif
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  
  // Captive portal to keep the client
  server.on("*", HTTP_GET, [](AsyncWebServerRequest * request)
  {
  #ifdef VERBOSE
    Serial.print("Request: ");
    Serial.println(request->url());
  #endif
    if (LittleFS.exists(request->url())) {
      String contentType = getContentType(request->url());
      AsyncWebServerResponse * response = request->beginResponse(LittleFS, request->url(), contentType);
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    }
    else {
     
      request->redirect("http://" + apIP.toString());
    }
  });


  server.begin();
}

String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

void setupCam(){
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  // init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    Serial.println("PSRAM found!");
    //config.frame_size = FRAMESIZE_SVGA;
   // config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 6;  //0-63 lower number means higher quality
    config.fb_count = 1;
  } else {
    //config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();

  camera_fb_t* frameBuffer = nullptr;
  // Skip first N frames.
  for (int i = 0; i < 5; i++) {
    frameBuffer = esp_camera_fb_get();
    esp_camera_fb_return(frameBuffer);
    delay(500);
    frameBuffer = nullptr;
}

}

String sendPhoto() {
  String getAll;
  String getBody;
  sensor_t * s = esp_camera_sensor_get();
  
  camera_fb_t * fb = NULL;
  for (int i = 0; i < 5; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    delay(500);
    fb = nullptr;
  }
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connecting to server: " + serverName);

  //client.setInsecure(); //skip certificate validation
  String serverURL = serverName;
  if (client.connect(serverURL.c_str(), serverPort)) {
    Serial.println("Connection successful!");
    String head = "--ConnectedShelf\r\nContent-Disposition: form-data; name=\"image\"; filename=\"" + namePrefix + getName() + ".jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--ConnectedShelf--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + "/" + getName() + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=ConnectedShelf");
    client.println();
    client.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length() == 0) {
            state = true;
          }
          getAll = "";
        }
        else if (c != '\r') {
          getAll += String(c);
        }
        if (state == true) {
          getBody += String(c);
        }
        startTimer = millis();
      }
      if (getBody.length() > 0) {
        break;
      }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
  }

  esp_camera_fb_return(fb);
  return getBody;
}


void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg,data,len);
  }
  else if (type == WS_EVT_CONNECT)
  {
    Serial.println("Websocket client connection received");
    // store connected client
    for (int i = 0; i < MAX_NUM_CLIENTS; ++i)
      if (clients[i] == NULL)
      {
        clients[i] = client;
        break;
      }
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.println("Client disconnected");
    // remove client from storage
    for (int i = 0; i < MAX_NUM_CLIENTS; ++i)
      if (clients[i] == client)
        clients[i] = NULL;
  }
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String in = String((char*)data);
    Serial.println(in);
    if (len > 2) {
      if (in.indexOf("{\"") >= 0 && in.indexOf("SSID") >= 0 ) {
        Serial.println("it's networks");
        const uint8_t size = JSON_OBJECT_SIZE(2);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err) {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(err.c_str());
          return;
        }
        const char *SSIDIN = json["SSID"];
        Serial.println(SSIDIN);
        const char *PASSIN = json["PASS"];
        Serial.println(PASSIN);
        setCredentials(SSIDIN,PASSIN);
        json.clear();
        noneblockingWiFiConnect();

      } else if (in.indexOf("networks") >= 0) {
        // send network scan
       // blinkLed(50);
        wifiScanSend = true;
      } else if (in.indexOf("status") >= 0) {
        // send connected states
       // blinkLed(50);
        connectedStatusSend = true;


      } else if (in.indexOf("{\"") >= 0 && in.indexOf("NAME") >= 0 ) {
        Serial.println("it's name");
        const uint8_t size = JSON_OBJECT_SIZE(1);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err) {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(err.c_str());
          return;
        }
        const char *NAME = json["NAME"];
        Serial.println(NAME);
        setName(NAME);
      //  setNetwork(SSIDIN, PASSIN);
     //   connectToRouter(String(SSIDIN), String(PASSIN), 60000);
      //  connectedStatusSend = true;
        json.clear();
      }
    }
  }
}

void sendWiFiScan() {
  String scan = getScanAsJsonString();
  char buf[1000];
  scan.toCharArray(buf, scan.length() + 1);
  websocket.textAll(buf, scan.length());
  Serial.println(scan);
}

void sendConnectedStatus() {
  char buf[200];
  String out = getStatusAsJsonString();
  out.toCharArray(buf, out.length() + 1);
  websocket.textAll(buf, out.length());
  Serial.println(out);
}


//Functions triggered by buttons on webpage
void checkWebsocketRequests() {
  isWiFiScanReady();
  isConnectedStatusReady();
}

void isConnectedStatusReady() {
  if (connectedStatusSend == true) {
    sendConnectedStatus();
    connectedStatusSend = false;
  }
}

void isWiFiScanReady() {
  if (wifiScanSend == true) {
    sendWiFiScan();
   wifiScanSend = false;
  }
}

String getWifiAPName(){
  return AP_SSID;
}
void noneblockingWiFiHandler(){
  if(millis() - prevWifiMillis > 100 && isNBConnecting == true){
    prevWifiMillis = millis();
     if(WiFi.status() != WL_CONNECTED){
      if(millis() - noneBlockingWifiConnectStartTime > 8000){    
        #ifdef VERBOSE
        Serial.println("Could not connect to WiFi, reverting to Access Point");
        #endif
        setInternetStatus(false);
        isNBConnecting = false;
        sendConnectedStatus();
      }
     } else {
       #ifdef VERBOSE
      Serial.println("Connected!");
      Serial.println(WiFi.localIP());
    #endif
      setInternetStatus(true);
      isNBConnecting = false;
      sendConnectedStatus();
     }
  }
}

void noneblockingWiFiConnect(){
  WiFi.begin(getSSID().c_str(),getPass().c_str());
  wifiCheckMillis = millis();
  noneBlockingWifiConnectStartTime = millis();
  isNBConnecting = true;

}

void setupAPSSID(){
   // https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint64_t chipID = ESP.getEfuseMac();
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  AP_SSID = AP_SSID+"-"+out;
  Serial.println(AP_SSID);
}

void wifiStatusCheck(){
  if(millis() - wifiCheckMillis > 3000){
    if(WiFi.status() != WL_CONNECTED){
       if(getInternetStatus() == true){
          setInternetStatus(false);
          char buf[200];
          getWifiString().toCharArray(buf, getWifiString().length() + 1);
          websocket.textAll(buf, getWifiString().length());
          Serial.println(getWifiString());
      }
    } else if(WiFi.status() == WL_CONNECTED){
      if(getInternetStatus() == false){
        setInternetStatus(true);
        char buf[200];
        getWifiString().toCharArray(buf, getWifiString().length() + 1);
        websocket.textAll(buf, getWifiString().length());
        Serial.println(getWifiString());
      }
    }
  }
}


int setFramesize(sensor_t * s, int val){
  if(val == 14){
    //16:10 ratio
    s->set_framesize(s, FRAMESIZE_UXGA);
    delay(500);
    s->set_res_raw(s, 0, 0, 0, 0, 0, 100, 1600, 1000, 1600, 1000, true, true);
    setSetting("framesize",14);
  } else {
    s->set_framesize(s, (framesize_t)val);
    setSetting("framesize",val);
  }
  return 1;
}

int getFramesize(sensor_t * s){
  if(getSetting("framesize") != 14){
    return s->status.framesize;
  } else {
    return 14;
  }
}