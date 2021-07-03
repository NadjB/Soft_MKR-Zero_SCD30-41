// Basic demo for readings from Adafruit SCD30
#include <Adafruit_SCD30.h>
#include <SensirionI2CScd4x.h>
#include <RTCZero.h>
#include <SD.h>
#include <DEV_Config.h>
#include <EPD_2in13.h>
#include <GUI_Paint.h>
#include <avr/dtostrf.h>


#define nbrLine 20
unsigned char BlackImage[((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8 ) : (EPD_WIDTH / 8 + 1)) * nbrLine]; //50 line
PAINT_TIME sPaint_time;
char s[10];

Adafruit_SCD30  scd30;
SensirionI2CScd4x scd4x;
RTCZero rtc;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 8;
const byte hours = 19;

/* Change these values to set the current initial date */
const byte day = 25;
const byte month = 6;
const byte year = 21;


uint16_t error;
uint16_t SCD41_co2;
float SCD41_temperature;
float SCD41_humidity;

const int chipSelect = SS1;

int loadDataCheck;  //Checks if data needs to be loaded
String fileName = "datalog.txt"; 
bool createNewFile = true;
bool sdPresent = false;
bool serialWasDisconnected = true;

float voltage = 0.0;

bool scdCal = false;
uint32_t tBegin;
uint16_t scd41CorrectionFactor = 0;

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  //Serial.begin(115200);
  DEV_ModuleInit();
  Wire.begin();

  Serial.println("Adafruit SCD30 test!");

  // Setup SCD30
    if (!scd30.begin()) {
      Serial.println("Failed to find SCD30 chip");
      while (1) { delay(10); }
    }
    Serial.println("SCD30 Found!");



    // if (!scd30.setMeasurementInterval(10)){
    //   Serial.println("Failed to set measurement interval");
    //   while(1){ delay(10);}
    // }
    
    delay(1000);
    scd30.setMeasurementInterval(5); //to match the 5s of the SCD41
    Serial.print("Measurement Interval: "); 
    Serial.print(scd30.getMeasurementInterval()); 
    Serial.println(" seconds");
    scd30.selfCalibrationEnabled(false);

  // Setup SCD41  
    scd4x.begin(Wire);
    scd4x.stopPeriodicMeasurement();
    delay(1000);
    scd4x.startPeriodicMeasurement();  
    scd4x.setAutomaticSelfCalibration(0);

  //Setup rtc
    //Serial.println("Initializing RTC");
    
    rtc.begin();
    rtc.setTime(hours, minutes, seconds);
    rtc.setDate(day, month, year);

    //rtc.setAlarmTime(0, 0, 0);
    //rtc.enableAlarm(rtc.MATCH_SS); //alarm attached every minute
    
    //rtc.attachInterrupt(dataCheck);
  // set up the ePaper:
    if (EPD_Init(FULL_UPDATE) != 0) 
    {
        Serial.print("e-Paper init failed\r\n");
    }
    EPD_Clear();
    DEV_Delay_ms(500);

    Paint_NewImage(BlackImage, EPD_WIDTH, nbrLine, 0, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(0xff);
    
    Paint_DrawString_EN(0, 0, "Temperature", &Font16, BLACK, WHITE);
    EPD_DisplayWindows(BlackImage, 0, 0, 122, 20);
    Paint_Clear(0xff);
    Paint_DrawString_EN(0, 0, "Humidity", &Font16, BLACK, WHITE);
    EPD_DisplayWindows(BlackImage, 0, 75, 122, 95);
    Paint_Clear(0xff);
    Paint_DrawString_EN(0, 0, "CO2", &Font16, BLACK, WHITE);
    EPD_DisplayWindows(BlackImage, 0, 150, 122, 170);
    DEV_Delay_ms(500);//Analog clock 1s
    EPD_TurnOnDisplay();

    if (EPD_Init(PART_UPDATE) != 0) 
    {
      Serial.print("e-Paper init failed\r\n");
    }

    tBegin = millis();
    //scdCal = true; //to launch the calibration !! The calibration should work but was not tested !!

  

}

void loop() {

  uint16_t co2;
  uint16_t temperature;
  uint16_t humidity;

  if (((millis()-tBegin) > 180000) && scdCal) //launch calibration after running for 3mn
  {
    int ppmInEvironement = 666;
    scd4x.stopPeriodicMeasurement();
    delay(500);
    scd4x.performForcedRecalibration(ppmInEvironement, scd41CorrectionFactor);
    delay(500);
    scd4x.startPeriodicMeasurement();
    delay(500);

    scd30.forceRecalibrationWithReference(ppmInEvironement);
    delay(500);
    
    scdCal = false; 
  }

  if (scd30.dataReady()){

    if (!scd30.read()){ Serial.println("Error reading sensor data"); return; }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    convertSCD41measures(co2, temperature, humidity);

    printEpaper();

    if (Serial){
      //Serial.println("Serial available");
      if (serialWasDisconnected){
        //Serial.println("Serial send header");
        serialWasDisconnected = false;
        printCSVhead(Serial);
      }
      //Serial.println("Serial send data");
      printCSVvalues(Serial);      
    } else {
      //Serial.println("Serial not available");
      serialWasDisconnected = true;
    }
    
  } else {
    //Serial.println("No data");
  }

  delay(100);
}


void printEpaper()
{
  Paint_Clear(0xff);
  dtostrf(scd30.temperature, 6, 2, s);
  Paint_DrawString_EN(0, 0, s, &Font20, WHITE, BLACK);
  Paint_DrawCircle(90, 6, 3, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_1X1);
  Paint_DrawString_EN(95, 0, "C", &Font20, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 20, 122, 40);

  Paint_Clear(0xff);
  dtostrf(SCD41_temperature, 6, 2, s);
  Paint_DrawString_EN(0, 0, s, &Font20, WHITE, BLACK);
  Paint_DrawCircle(90, 6, 3, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_1X1);
  Paint_DrawString_EN(95, 0, "C", &Font20, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 45, 122, 65);

  Paint_Clear(0xff);
  dtostrf(scd30.relative_humidity, 6, 2, s);
  Paint_DrawString_EN(0, 0, s, &Font20, WHITE, BLACK);
  Paint_DrawString_EN(85, 0, "%", &Font20, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 95, 122, 115);

  Paint_Clear(0xff);
  dtostrf(SCD41_humidity, 6, 2, s);
  Paint_DrawString_EN(0, 0, s, &Font20, WHITE, BLACK);
  Paint_DrawString_EN(85, 0, "%", &Font20, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 120, 122, 140);

  Paint_Clear(0xff);
  //dtostrf(scd30.CO2, 6, 2, s);
  //Paint_DrawString_EN(0, 0, s, &Font16, WHITE, BLACK);
  Paint_DrawNum(15, 0, (int)scd30.CO2, &Font20, WHITE, BLACK);
  Paint_DrawString_EN(89, 0, "ppm", &Font16, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 175, 122, 195);

  Paint_Clear(0xff);
  //dtostrf(scd30.CO2, 6, 2, s);
  //Paint_DrawString_EN(0, 0, s, &Font16, WHITE, BLACK);
  Paint_DrawNum(15, 0, (int)SCD41_co2, &Font20, WHITE, BLACK);
  Paint_DrawString_EN(89, 0, "ppm", &Font16, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 200, 122, 220);

  Paint_Clear(0xff);
  //dtostrf((analogRead(ADC_BATTERY)* 4.2 / 1023.0), 6, 2, s);
  //Paint_DrawString_EN(50, 0, s, &Font16, WHITE, BLACK);
  Paint_DrawString_EN(30, 0, "Batt:", &Font16, WHITE, BLACK);
  Paint_DrawNum(80, 0, getBatteryPercent(), &Font16, WHITE, BLACK);
  Paint_DrawString_EN(110, 0, "%", &Font16, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 230, 122, 250);
  /*
  Paint_Clear(0xff);
  Paint_DrawTime(0, 0, &sPaint_time, &Font16, WHITE, BLACK);
  EPD_DisplayPartWindows(BlackImage, 0, 230, 122, 250);
  */

  EPD_TurnOnDisplay();
}

void convertSCD41measures(uint16_t co2, uint16_t temperature, uint16_t humidity){
  SCD41_co2 = co2;
  SCD41_temperature = (temperature * 175.0 / 65536.0 - 45.0);
  SCD41_humidity = (humidity * 100.0 / 65536.0);
}



template<typename T>
void printCSVhead(T& out)
{
  String s = "";
  s += ("Time|yy-mm-dd_hh:mm:ss");
  s += ('\t');
  s += ("ms");
  s += ('\t');
  s += ("SCD30_Temp|C");
  s += ('\t');
  s += ("SCD30_R-Hum|%");
  s += ('\t');
  s += ("SCD30_CO2|ppm");
  s += ('\t');
  s += ("SCD41_Temp|C");
  s += ('\t');
  s += ("SCD41_R-Hum|%");
  s += ('\t');
  s += ("SCD41_CO2|ppm");
  s += ('\t');
  s += ("Batterie|V");
  s += ('\t');
  s += ("Batterie|%");
  out.println(s);
}

template<typename T>
//void printCSVvalues(T& scd30)
void printCSVvalues(T& out)
{
  String s = "";
  s += (getTime());
  s += ('\t');
  s += (millis());
  s += ('\t');
  s += (scd30.temperature);
  s += ('\t');
  s += (scd30.relative_humidity);
  s += ('\t');
  s += (scd30.CO2);
  s += ('\t');
  s += (SCD41_temperature);
  s += ('\t');
  s += (SCD41_humidity);
  s += ('\t');
  s += (SCD41_co2);
  s += ('\t');
  s += analogRead(ADC_BATTERY)* 4.2 / 1023.0;// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  s += ('\t');
  s += getBatteryPercent();
  out.println(s);
}

int getBatteryPercent()
{
  float voltage = (analogRead(ADC_BATTERY)* 4.2 / 1023.0);
  float voltageMax = 4.16; //should be 4.2V
  float voltageMin = 3.4;  //should be 3V (battery) or 3.3V (Microcontroller) 
  float voltageRange = voltageMax - voltageMin;
  int percent = -1;
  if (voltage >= voltageMax)
  {
    percent = 100;
  }
  else if (voltage <= voltageMin)
  {
    percent = 0;
  }
  else
  {
    percent = (int)(100.0/voltageRange*(voltage-voltageMin));
  }
  return percent;

}

String formatIntToStr(int i)
{
  /*
  Fait en sorte que l'on ait deux chiffres 
  pout les dates & heurs
  */
  String s = "";
  if (i < 10)
     s = "0" + String(i);
  else
    s = String(i);
  return s;

}

String getTime(void) 
{
  /*
  retourne : yy-mm-dd_hh:mm:ss
  */
  String returnString = "";

  returnString += formatIntToStr(rtc.getYear());
  returnString += "-";
  returnString += formatIntToStr(rtc.getMonth());
  returnString += "-";
  returnString += formatIntToStr(rtc.getDay());

  returnString += "_";

  returnString += formatIntToStr(rtc.getHours());
  returnString += ":";
  returnString += formatIntToStr(rtc.getMinutes());
  returnString += ":";
  returnString += formatIntToStr(rtc.getSeconds());

  return returnString;
}
