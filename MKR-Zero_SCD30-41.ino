// Basic demo for readings from Adafruit SCD30
#include <Adafruit_SCD30.h>
#include <SensirionI2CScd4x.h>
#include <RTCZero.h>
#include <SD.h>

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

void setup(void) {
  Serial.begin(115200);

  Serial.println("Adafruit SCD30 test!");

  // Try to initialize!
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");


  scd4x.begin(Wire);
  scd4x.stopPeriodicMeasurement();
  delay(1000);
  scd4x.startPeriodicMeasurement();  

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

  //Setup rtc
    //Serial.println("Initializing RTC");
    
    rtc.begin();
    rtc.setTime(hours, minutes, seconds);
    rtc.setDate(day, month, year);

    //rtc.setAlarmTime(0, 0, 0);
    //rtc.enableAlarm(rtc.MATCH_SS); //alarm attached every minute
    
    //rtc.attachInterrupt(dataCheck);

}

void loop() {

  uint16_t co2;
  uint16_t temperature;
  uint16_t humidity;
  if (scd30.dataReady()){

    if (!scd30.read()){ Serial.println("Error reading sensor data"); return; }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    convertSCD41measures(co2, temperature, humidity);
    if (Serial){
      //Serial.println("Serial available");
      if (serialWasDisconnected){
        //Serial.println("Serial send header");
        serialWasDisconnected = false;
        printCSVhead(Serial);
      }
      //Serial.println("Serial send data");
      printCSVvalues(Serial);      
    }
    else
    {
      //Serial.println("Serial not available");
      serialWasDisconnected = true;
    }

    /*
    Serial.print("Temperature: ");
    Serial.print(scd30.temperature);
    Serial.println(" degrees C");
    
    Serial.print("Relative Humidity: ");
    Serial.print(scd30.relative_humidity);
    Serial.println(" %");
    
    Serial.print("CO2: ");
    Serial.print(scd30.CO2, 3);
    Serial.println(" ppm");
    Serial.println("");

    if (error) {
        Serial.print("Error trying to execute readMeasurement()");
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        Serial.print("Co2:");
        Serial.print(SCD41_co2);
        Serial.print("\t");
        Serial.print("Temperature:");
        Serial.print(SCD41_temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(SCD41_humidity);
    }*/
  } else {
    //Serial.println("No data");
  }

  delay(100);
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
  Fait en sorte que l'on est deux chiffres 
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
