#define DISPLAY_UPDATE(x) display.update(x)
// #define ERASE_DISPLAY(x) display.eraseDisplay(x);
// #define DISPLAY_UPDATE(x) 
#define ERASE_DISPLAY(x)


#include <FS.h>          // this needs to be first, or it all crashes and burns...

// include library, include base class, make path known
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBoldOblique9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoOblique9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifBoldItalic9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>

#include <time.h>

#include "timezones.h"
//#define DEFALUT_FONT  FreeMono9pt7b
// #define DEFALUT_FONT  FreeMonoBoldOblique9pt7b
// #define DEFALUT_FONT FreeMonoBold9pt7b
// #define DEFALUT_FONT FreeMonoOblique9pt7b
#define DEFALUT_FONT FreeSans9pt7b
// #define DEFALUT_FONT FreeSansBold9pt7b
// #define DEFALUT_FONT FreeSansBoldOblique9pt7b
// #define DEFALUT_FONT FreeSansOblique9pt7b
// #define DEFALUT_FONT FreeSerif9pt7b
// #define DEFALUT_FONT FreeSerifBold9pt7b
// #define DEFALUT_FONT FreeSerifBoldItalic9pt7b
// #define DEFALUT_FONT FreeSerifItalic9pt7b

const GFXfont *fonts[] = {
    &FreeMono9pt7b,
    &FreeMonoBoldOblique9pt7b,
    &FreeMonoBold9pt7b,
    &FreeMonoOblique9pt7b,
    &FreeSans9pt7b,
    &FreeSansBold9pt7b,
    &FreeSansBoldOblique9pt7b,
    &FreeSansOblique9pt7b,
    &FreeSerif9pt7b,
    &FreeSerifBold9pt7b,
    &FreeSerifBoldItalic9pt7b,
    &FreeSerifItalic9pt7b
};

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include "SD.h"
#include "SPI.h"
#include <SPIFFS.h>
#include <FS.h>
#include "esp_wifi.h"
#include "Esp.h"
#include "board_def.h"
#include <Button2.h>

#include <WiFiManager.h>  
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#include <SPIFFS.h>

WiFiManager wifiManager;

#define FILESYSTEM SPIFFS

#define USE_AP_MODE

/*100 * 100 bmp fromat*/
//https://www.onlineconverter.com/jpg-to-bmp
#define BADGE_CONFIG_FILE_NAME "/badge.data"
#define DEFALUT_AVATAR_BMP "/avatar.bmp"
#define DEFALUT_QR_CODE_BMP "/qr.bmp"
#define WIFI_SSID "Put your wifi ssid"
#define WIFI_PASSWORD "Put your wifi password"
#define CHANNEL_0 0
#define IP5306_ADDR 0X75
#define IP5306_REG_SYS_CTL0 0x00

void displayInit(void);
void drawBitmap(const char *filename, int16_t x, int16_t y, bool with_color);

const char* IMG_CAST[] = {
    "/suno_open.bmp",
    "/suno_awen.bmp",
    "/suno_pini.bmp",
    "/pimeja_open.bmp",
    "/pimeja_awen.bmp",
    "/pimeja_pini.bmp",
};

String timezone_name("Etc/UCT");

typedef enum {
    RIGHT_ALIGNMENT = 0,
    LEFT_ALIGNMENT,
    CENTER_ALIGNMENT,
} Text_alignment;

GxIO_Class io(SPI, ELINK_SS, ELINK_DC, ELINK_RESET);
GxEPD_Class display(io, ELINK_RESET, ELINK_BUSY);

static const uint16_t input_buffer_pixels = 20;       // may affect performance
static const uint16_t max_palette_pixels = 256;       // for depth <= 8
uint8_t mono_palette_buffer[max_palette_pixels / 8];  // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w
uint8_t input_buffer[3 * input_buffer_pixels];        // up to depth 24
const char *path[2] = {DEFALUT_AVATAR_BMP, DEFALUT_QR_CODE_BMP};

Button2 *pBtns = nullptr;
uint8_t g_btns[] = BUTTONS_MAP;

void button_handle(uint8_t gpio)
{
    switch (gpio) {
#if BUTTON_1
    case BUTTON_1: {
        // esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_1, LOW);
        esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << BUTTON_1)), ESP_EXT1_WAKEUP_ALL_LOW);
        Serial.println("Going to sleep now");
        delay(2000);
        esp_deep_sleep_start();
    }
    break;
#endif

#if BUTTON_2
    case BUTTON_2: {
        static int i = 0;
        Serial.printf("Show Num: %d font\n", i);
        i = ((i + 1) >= sizeof(fonts) / sizeof(fonts[0])) ? 0 : i + 1;
        display.setFont(fonts[i]);
        // showMianPage();
    }
    break;
#endif

#if BUTTON_3
    case BUTTON_3: {
        static bool index = 1;
        if (!index) {
            // showMianPage();
            index = true;
        } else {
            // showQrPage();
            index = false;
        }
    }
    break;
#endif
    default:
        break;
    }
}

bool anybuttonsdown=false;

void button_callback(Button2 &b)
{
    anybuttonsdown=false;
    for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
        if (pBtns[i] == b) {
            Serial.printf("btn: %u press\n", pBtns[i].getAttachPin());
            // button_handle(pBtns[i].getAttachPin());
            anybuttonsdown=true;
        }
    }
}

void button_init()
{
    uint8_t args = sizeof(g_btns) / sizeof(g_btns[0]);
    pBtns = new Button2[args];
    for (int i = 0; i < args; ++i) {
        pBtns[i] = Button2(g_btns[i]);
        pBtns[i].setPressedHandler(button_callback);
    }
}

void button_loop()
{
    for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
        pBtns[i].loop();
    }
}

void displayText(const String &str, int16_t y, uint8_t alignment)
{
    int16_t x = 0;
    int16_t x1, y1;
    uint16_t w, h;
    display.setCursor(x, y);
    display.getTextBounds(str, x, y, &x1, &y1, &w, &h);

    switch (alignment) {
    case RIGHT_ALIGNMENT:
        display.setCursor(display.width() - w - x1, y);
        break;
    case LEFT_ALIGNMENT:
        display.setCursor(0, y);
        break;
    case CENTER_ALIGNMENT:
        display.setCursor(display.width() / 2 - ((w + x1) / 2), y);
        break;
    default:
        break;
    }
    display.println(str);
}




// void showMianPage(void)
// {
//     displayInit();
//     display.fillScreen(GxEPD_WHITE);
//     drawBitmap(DEFALUT_AVATAR_BMP, 10, 10, true);
//     displayText(String(info.name), 30, RIGHT_ALIGNMENT);
//     displayText(String(info.company), 50, RIGHT_ALIGNMENT);
//     displayText(String(info.email), 70, RIGHT_ALIGNMENT);
//     displayText(String(info.link), 90, RIGHT_ALIGNMENT);
//     display.update();
// }


uint16_t read16(File &f)
{
    // BMP data is stored little-endian, same as Arduino.
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(File &f)
{
    // BMP data is stored little-endian, same as Arduino.
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}

void drawBitmap(const char *filename, int16_t x, int16_t y, bool with_color)
{
    File file;
    bool valid = false; // valid format to be handled
    bool flip = true;   // bitmap is stored bottom-to-top
    uint32_t startTime = millis();
    if ((x >= display.width()) || (y >= display.height()))
        return;
    Serial.println();
    Serial.print("Loading image '");
    Serial.print(filename);
    Serial.println('\'');

    file = FILESYSTEM.open(filename, FILE_READ);
    if (!file) {
        Serial.print("File not found");
        return;
    }

    // Parse BMP header
    if (read16(file) == 0x4D42) {
        // BMP signature
        uint32_t fileSize = read32(file);
        uint32_t creatorBytes = read32(file);
        uint32_t imageOffset = read32(file); // Start of image data
        uint32_t headerSize = read32(file);
        uint32_t width = read32(file);
        uint32_t height = read32(file);
        uint16_t planes = read16(file);
        uint16_t depth = read16(file); // bits per pixel
        uint32_t format = read32(file);
        if ((planes == 1) && ((format == 0) || (format == 3))) {
            // uncompressed is handled, 565 also
            Serial.print("File size: ");
            Serial.println(fileSize);
            Serial.print("Image Offset: ");
            Serial.println(imageOffset);
            Serial.print("Header size: ");
            Serial.println(headerSize);
            Serial.print("Bit Depth: ");
            Serial.println(depth);
            Serial.print("Image size: ");
            Serial.print(width);
            Serial.print('x');
            Serial.println(height);
            // BMP rows are padded (if needed) to 4-byte boundary
            uint32_t rowSize = (width * depth / 8 + 3) & ~3;
            if (depth < 8)
                rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
            if (height < 0) {
                height = -height;
                flip = false;
            }
            uint16_t w = width;
            uint16_t h = height;
            if ((x + w - 1) >= display.width())
                w = display.width() - x;
            if ((y + h - 1) >= display.height())
                h = display.height() - y;
            valid = true;
            uint8_t bitmask = 0xFF;
            uint8_t bitshift = 8 - depth;
            uint16_t red, green, blue;
            bool whitish, colored;
            if (depth == 1)
                with_color = false;
            if (depth <= 8) {
                if (depth < 8)
                    bitmask >>= depth;
                file.seek(54); //palette is always @ 54
                for (uint16_t pn = 0; pn < (1 << depth); pn++) {
                    blue = file.read();
                    green = file.read();
                    red = file.read();
                    file.read();
                    whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                    colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                    if (0 == pn % 8)
                        mono_palette_buffer[pn / 8] = 0;
                    mono_palette_buffer[pn / 8] |= whitish << pn % 8;
                    if (0 == pn % 8)
                        color_palette_buffer[pn / 8] = 0;
                    color_palette_buffer[pn / 8] |= colored << pn % 8;
                }
            }
            display.fillScreen(GxEPD_WHITE);
            uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
            for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) {
                // for each line
                uint32_t in_remain = rowSize;
                uint32_t in_idx = 0;
                uint32_t in_bytes = 0;
                uint8_t in_byte = 0; // for depth <= 8
                uint8_t in_bits = 0; // for depth <= 8
                uint16_t color = GxEPD_WHITE;
                file.seek(rowPosition);
                for (uint16_t col = 0; col < w; col++) {
                    // for each pixel
                    // Time to read more pixel data?
                    if (in_idx >= in_bytes) {
                        // ok, exact match for 24bit also (size IS multiple of 3)
                        in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
                        in_remain -= in_bytes;
                        in_idx = 0;
                    }
                    switch (depth) {
                        case 24:
                            blue = input_buffer[in_idx++];
                            green = input_buffer[in_idx++];
                            red = input_buffer[in_idx++];
                            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                            break;
                        case 16: {
                            uint8_t lsb = input_buffer[in_idx++];
                            uint8_t msb = input_buffer[in_idx++];
                            if (format == 0) {
                                // 555
                                blue = (lsb & 0x1F) << 3;
                                green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                                red = (msb & 0x7C) << 1;
                            } else {
                                // 565
                                blue = (lsb & 0x1F) << 3;
                                green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                                red = (msb & 0xF8);
                            }
                            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                        }
                        break;
                        case 1:
                        case 4:
                        case 8: {
                            if (0 == in_bits) {
                                in_byte = input_buffer[in_idx++];
                                in_bits = 8;
                            }
                            uint16_t pn = (in_byte >> bitshift) & bitmask;
                            whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                            colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                            in_byte <<= depth;
                            in_bits -= depth;
                        }
                        break;
                    }
                    if (whitish) {
                        color = GxEPD_WHITE;
                    } else if (colored && with_color) {
                        color = GxEPD_RED;
                    } else {
                        color = GxEPD_BLACK;
                    }
                    uint16_t yrow = y + (flip ? h - row - 1 : row);
                    display.drawPixel(x + col, yrow, color);
                } // end pixel
            }     // end line
            Serial.print("loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        }
    }
    file.close();
    if (!valid) {
        Serial.println("bitmap format not handled.");
    }
}

void displayInit(void)
{
    static bool isInit = false;
    if (isInit) {
        return;
    }
    isInit = true;
    display.init();
    display.setRotation(0);
    ERASE_DISPLAY();
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&DEFALUT_FONT);
    display.setTextSize(0);

//     if (SDCARD_SS > 0) {
//         display.fillScreen(GxEPD_WHITE);
// #if !(TTGO_T5_2_2)
//         SPIClass sdSPI(VSPI);
//         sdSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_SS);
//         if (!SD.begin(SDCARD_SS, sdSPI))
// #else
//         if (!SD.begin(SDCARD_SS))
// #endif
//         {
//             displayText("SDCard MOUNT FAIL", 50, CENTER_ALIGNMENT);
//         } else {
//             displayText("SDCard MOUNT PASS", 50, CENTER_ALIGNMENT);
//             uint32_t cardSize = SD.cardSize() / (1024 * 1024);
//             displayText("SDCard Size: " + String(cardSize) + "MB", 70, CENTER_ALIGNMENT);
//         }
//         display.update();
//         delay(2000);
//     }
}

bool setPowerBoostKeepOn(int en)
{
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en)
        Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
    else
        Wire.write(0x35); // 0x37 is default reg value
    return Wire.endTransmission() == 0;
}

const char*  WIFI_PORTAL_NAME = "ilo-tenpo";
const char*  WIFI_IP_NAME = "192.168.4.1";

void configModeCallback (WiFiManager *myWiFiManager) {
    displayInit();
    display.fillScreen(GxEPD_WHITE);
    displayText(String("o toki tawa ilo kon \"")+ WIFI_PORTAL_NAME+String("\"."), 30, LEFT_ALIGNMENT);
    displayText(String("o lukin e lipu \"")+ WIFI_IP_NAME+String("\"."), 70, LEFT_ALIGNMENT);

    DISPLAY_UPDATE();
}

void cantConnect(){
    displayInit();
        display.fillScreen(GxEPD_WHITE);
        displayText(String("mi ken ala toki tawa ilo kon \"")+wifiManager.getWiFiSSID()+String("\"."), 30, LEFT_ALIGNMENT);

        DISPLAY_UPDATE();
        delay(5000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wifiManager.server->hasArg(name)) {
    value = wifiManager.server->arg(name);
  }
  return value;
}

WiFiManagerParameter custom_field;


void saveParams(){


    Serial.println("not dirty; saving config");
    DynamicJsonDocument doc(1024);
    doc["timezone"] = timezone_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    
    serializeJsonPretty(doc,Serial);
    serializeJson(doc,configFile);
    
    configFile.close();
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  
  String new_timezone_name = getParam("timezone");

    if (new_timezone_name!=timezone_name ){
        timezone_name = new_timezone_name;

        saveParams();
    }

}

String format_time(int houroffset,int halfhouroffset){
    String sign = String(houroffset<0?"-" : ( halfhouroffset<0?"-" : "+"));
    String hour = String(houroffset<0?-houroffset:houroffset);
    return sign+hour+":"+   String(halfhouroffset==0?"00":"30");
}



void webServerCallback(){
    wifiManager.server->serveStatic("/timezones.html",FILESYSTEM,"/timezones.html");
}


void setupWifi(){
    esp_wifi_start();
    String dropdownwifi = String ("<br/><label for='timezone'>Timezone:</label></p><select id='timezone' name='timezone'>");

    int timezonecount=sizeof(timezones) / sizeof(timezones[0]);
    for( int i = 0; i < timezonecount; i+=2)
    {
      String zone_name=String(timezones[i]);

      //const char* zone_data=timezones[i+1];

      String selected_str = (zone_name == timezone_name) ? String(" selected "):String("");
      dropdownwifi+=String("<option value='")+zone_name+String("'")+selected_str+String(">")+zone_name+String("</option>");
    }
    dropdownwifi += String("</select>");

    new (&custom_field) WiFiManagerParameter(dropdownwifi.c_str()); // custom html input
    wifiManager.setWebServerCallback(webServerCallback);
    wifiManager.addParameter(&custom_field);
    wifiManager.setSaveParamsCallback(saveParamCallback);
    Serial.println("Aasd2");
    
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setTimeout(300);
    Serial.println("Aasd3");

    button_loop();
    anybuttonsdown = false;
    Serial.println("BUTTONS");
    for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
        
        Serial.println(i);
        if (digitalRead(g_btns[i])==LOW){
            Serial.println("LOW");
            anybuttonsdown = true;
        } else {
            Serial.println("HIGH");

        }
    }
    if (anybuttonsdown){
        
        for (int i=0;i<5;i++){
            ledcWriteTone(CHANNEL_0, 400);
            delay(20);
            ledcWriteTone(CHANNEL_0, 800);
            delay(20);
        }  


            ledcWriteTone(CHANNEL_0, 0);

        if (!wifiManager.startConfigPortal(WIFI_PORTAL_NAME)){
            cantConnect();
        }
    } else if (! wifiManager.autoConnect(WIFI_PORTAL_NAME)) {
        cantConnect();
    }
    Serial.println("Aasd4");

}






void setupSpiffs(){
  //clean FS, for testing
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument doc(1024);

        deserializeJson(doc,buf.get());
        serializeJson(doc,Serial);

        if (!doc.isNull()) {
          Serial.println("\nparsed json");
          const char* timezone_cstr = doc["timezone"];
          timezone_name = String(timezone_cstr);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

int time_of_day = 0;

inline double ms_to_minutes(long int i){
    return i/(1000.0*60.0);
}

inline String wakeupReason(){
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch(wakeup_reason)
	{
		case 1  : return String("Wakeup caused by external signal using RTC_IO"); break;
		case 2  : return String("Wakeup caused by external signal using RTC_CNTL"); break;
		case 3  : return String("Wakeup caused by timer"); break;
		case 4  : return String("Wakeup caused by touchpad"); break;
		case 5  : return String("Wakeup caused by ULP program"); break;
		default : return String("Wakeup was not caused by deep sleep"); break;
	}

}

double voltage(){
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC_ATTEN_DB_2_5, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
  float measurement = (float) analogRead(35);
  float battery_voltage = (measurement / 4095.0) * 7.05;
  return battery_voltage;
}


void setup()
{
    Serial.begin(115200);
    delay(500);

    if (SPEAKER_OUT > 0) {
        ledcSetup(CHANNEL_0, 1000, 8);
        ledcAttachPin(SPEAKER_OUT, CHANNEL_0);
        int i = 3;
        while (i--) {
            ledcWriteTone(CHANNEL_0, 1000);
            delay(20);
            ledcWriteTone(CHANNEL_0, 0);
            delay(100);
        }
    }

    button_init();

    setupSpiffs();

    setupWifi();



#ifdef ENABLE_IP5306
    Wire.begin(I2C_SDA, I2C_SCL);
    bool ret = setPowerBoostKeepOn(1);
    Serial.printf("Power KeepUp %s\n", ret ? "PASS" : "FAIL");
#endif

// It is only necessary to turn on the power amplifier power supply on the T5_V24 board.
#ifdef AMP_POWER_CTRL
    pinMode(AMP_POWER_CTRL, OUTPUT);
    digitalWrite(AMP_POWER_CTRL, HIGH);
#endif



    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, -1);
    if (!FILESYSTEM.begin()) {
        Serial.println("FILESYSTEM is not database");
        Serial.println("Please use Arduino ESP32 Sketch data Upload files");
        while (1) {
            delay(1000);
        }
    }


    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        // showMianPage();
    }


    // while(!timeClient.update()) {
    //     timeClient.forceUpdate();
    // }
    

    const char *ntpServer = "pool.ntp.org";
    String timezonespecs("UTC0");
    int timezonecount=sizeof(timezones) / sizeof(timezones[0]);
    for( int i = 0; i < timezonecount; i+=2)
    {
      String zone_name=String(timezones[i]);
        if (zone_name == timezone_name){
            timezonespecs=String(timezones[i+1]);
        }
    }

    configTzTime(timezonespecs.c_str(),ntpServer);


    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
    }

    long milliseconds_since_midnight = ((timeinfo.tm_hour*60+timeinfo.tm_min)*60+timeinfo.tm_sec)*1000;  

    
    //time since 6am

    const long milliseconds_in_hour = ( long)60*60*1000;
    const long milliseconds_in_day = ( long)24*milliseconds_in_hour;

    long milliseconds_since_6am = ( long)(milliseconds_since_midnight+(18*milliseconds_in_hour))%milliseconds_in_day;
    
    const long milliseconds_in_window = (long)milliseconds_in_hour*4;

    
//five minutes into the future
    long pretendoffset_ms = 5*60*1000;

    long time_of_day_index = ((milliseconds_since_6am+pretendoffset_ms)/milliseconds_in_window)%6;
    time_of_day = time_of_day_index;
    
    displayInit();
    display.fillScreen(GxEPD_WHITE);


    // displayText("tenpo ni li:", 10, LEFT_ALIGNMENT);

    String time_string = String(timeinfo.tm_hour)+String(":")+String(timeinfo.tm_min);
    Serial.print(time_string);


    // displayText(timezone_name, 110, LEFT_ALIGNMENT);

    // const char* tz = getenv("TZ");

    // displayText(String(tz), 170, LEFT_ALIGNMENT);

    // displayText(String("Is DST? ")+String(timeinfo.tm_isdst), 210, LEFT_ALIGNMENT);

    // Serial.println("time of day:");
    // Serial.println(time_of_day);


    // delay(120000);


    long ms_current_window_started = milliseconds_in_window*time_of_day_index;
    long ms_into_current_window = (long)milliseconds_since_6am-ms_current_window_started;
    
    long ms_remaining_in_current_window = (long)milliseconds_in_window - ms_into_current_window;

    //tenpo suno open = index 0 is 6am = 6 hours = 6*60 mins = 6*60*60 seconds = 6*60*60*1000 miliseconds

    drawBitmap(IMG_CAST[time_of_day],0,150+(296-275)/2, true);

    displayText(time_string, 15, LEFT_ALIGNMENT);
    displayText(String(time_of_day_index), 15, RIGHT_ALIGNMENT);
    displayText(String(ms_to_minutes(ms_into_current_window)), 30, LEFT_ALIGNMENT);
    displayText(String(ms_to_minutes(ms_remaining_in_current_window)), 50, LEFT_ALIGNMENT);
    displayText(String((unsigned long)(ms_remaining_in_current_window*1000)), 70, LEFT_ALIGNMENT);

    uint16_t battery_voltage = voltage();
    displayText(String(battery_voltage)+String("V"), 90, LEFT_ALIGNMENT);
    
    displayText(wakeupReason(),110,LEFT_ALIGNMENT);
        
    DISPLAY_UPDATE();
    esp_wifi_stop();
    WiFi.disconnect();
  	WiFi.mode(WIFI_OFF);
    
    //https://www.reddit.com/r/esp32/comments/idinjr/36ma_deep_sleep_in_an_eink_ttgo_t5_v23/
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    gpio_deep_sleep_hold_en();
    display.powerDown();
    esp_deep_sleep ((unsigned long)ms_remaining_in_current_window*1000);
}


void loop(){
    Serial.println("shouldn't get here eep");
}