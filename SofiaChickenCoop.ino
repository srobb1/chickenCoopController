#include <TimeLord.h>
#include <Time.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 rtc;
#include <LiquidCrystal.h>
LiquidCrystal lcd(22,23,24,25,26,27);

// longitude (west values negative) and latitude (south values negative)
float const LONGITUDE =111.7677;
float const LATITUDE = 41.1180;
const int TIMEZONE = 8;

// RESET DATE/TIME
int reset = 1;

//*********** SENSORS ****************// 
int tempSensorPin = 0;

//************ SWITCHES *****************//
int P1=43; // Button SET MENU'
int P2=41; // Button +
int P3=42; // Button -
int P4=44; // daily sunrise/sunset
int P5=45; // daily temp high/low was 50
int P6=46; // manual open/close
int P7=47; // manual light on/off
const int REED_CLOSED = 48 ; // REED+Magnet = closed curcuit = Closed Door
const int REED_OPENED = 49; //  REED+Magnet = closed curcuit = Opened Door


//********** LEDS **********//
int LED_DOOR_OPEN = 32;
int LED_DOOR_CLOSE = 33;
int HIGH_TEMP_WARNING = 34;
int GOOD_TEMP_INDICATOR = 35;
int LOW_TEMP_WARNING = 36;
int LED_DAYLEN = 12;

//********** MOTOR ********
int enA = 13; // ~pwm
int in1 = 15;
int in2 = 17;

//************Variables**************//
int hourupg;
int minupg;
int yearupg;
int monthupg;
int dayupg;
int secupg;

// temp level 
int  low_temp_level = 32;
int high_temp_level = 95;

DateTime Sunrise;
DateTime Sunset;

float tempHigh;
float tempLow;

// menu default
int menu =0; // set time and date and temp
int menu1=0; // veiw sunrise/sunset info
int menu2=0; // view temp high/low for day

TimeLord tardis; 

void setup(){
  lcd.begin(16, 2);
  lcd.clear();

  pinMode(P1,INPUT); // set
  pinMode(P2,INPUT); // +
  pinMode(P3,INPUT); // -

  pinMode(P4, INPUT); // display daily sunrise sunset times
  pinMode(P5, INPUT); // display daily high low temp
  pinMode(P6, INPUT); // manual door override
  pinMode(P7, INPUT); // manual DAYLEN LED override

  pinMode(REED_CLOSED,INPUT); // lower reed for closed door on low/closed curcuit
  pinMode(REED_OPENED,INPUT); // upper reed for opened door on low/closed curcuit

  pinMode(LED_DOOR_OPEN, OUTPUT);  // LED OPEN
  pinMode(LED_DOOR_CLOSE, OUTPUT); // LED OPEN
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (reset == 1){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.begin(9600);
  Wire.begin();

  int menu=0;
  int menu1=0;
  int menu2=0;

}



void loop() {
// rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  DoorTimeCheck();
  LightTimeCheck();
  recordTemp();

  delay(1000);   

  if(digitalRead(P6)){
    ManualDoorOverride();
  } 
  if(digitalRead(P7)){
    ManualDayLenLightOverride();
  }  
  if(digitalRead(P1)) { // set
    menu=menu+1;
  }
  if(digitalRead(P4)) { // sunrise sunset
    menu1=menu1+1;
  }
  if(digitalRead(P5)) { // high low temp
    menu2=menu2+1;
  }


  if (menu1>0){
    DisplaySunriseSunset(); 
    menu1=0;
    menu=0;
  }
  if (menu2>0){
    DisplayDailyHighLowTemp(); 
    menu2=0;
    menu=0;
  }


  if (menu==0){
    DisplayDateTime(); 
  }
  if (menu==1){
    DisplaySetHour();
  }
  if (menu==2){
    DisplaySetMinute();
  }
  if (menu==3){
    DisplaySetYear();
  }
  if (menu==4){
    DisplaySetMonth();
  }
  if (menu==5){
    DisplaySetDay();
  }
  if (menu==6){
    DisplaySetLowTemp();
  }
  if (menu==7){
    DisplaySetHighTemp();
  }  
  if (menu==8){
    StoreAgg(); 
    delay(500);
    menu=0;
  }
  delay(100);
}



void recordTemp(){

  //getting the voltage reading from the temperature sensor
  int reading = analogRead(tempSensorPin);  

  // converting that reading to voltage, for 3.3v arduino use 3.3
  float voltage = reading * 5.0;
  voltage /= 1024.0; 

  // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
  //to degrees ((voltage - 500mV) times 100)
  // now convert to Fahrenheit
  float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  Serial.print(temperatureF); 
  Serial.println(" degrees F");

  DateTime now = rtc.now();

  int nowHour = now.hour();
  int nowMinute = now.minute();

  if (nowHour == 12 and nowMinute == 0){
    tempHigh = temperatureF ;
    tempLow =  temperatureF;
  } 
  else if (temperatureF > tempHigh){
    tempHigh = temperatureF;
  } 
  else if (temperatureF < tempLow){
    tempLow = temperatureF;
  }
}

void DisplayDailyHighLowTemp(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Daily Low: ");
  lcd.print(tempLow); 
  lcd.println(" F");
  lcd.setCursor(0, 1);
  lcd.print("Daily High: ");
  lcd.print(tempHigh);   
  lcd.println(" F");
  delay(2000);
  lcd.clear();
}

void ManualDoorOverride(){
  if(digitalRead(REED_OPENED == LOW)) { //If the upper door reed pin reads low, the switch is closed. and the door is open
    CloseDoor();
  }
  else if (REED_CLOSED == LOW){ //If the lower reed pin reads low, the switch is closed. and the door is closed
    OpenDoor();
  }//else{
  //  //maybe something is wrong, just open it
  //  OpenDoor() 
  //}
}

void ManualDayLenLightOverride(){
  if(digitalRead(LED_DAYLEN == LOW)) { //if the day length led is off turn it on
     digitalWrite(LED_DAYLEN, HIGH);   // turn the LED on
  }
  else if (REED_CLOSED == HIGH){ //if the day length led is on turn it off
     digitalWrite(LED_DAYLEN, LOW);   // turn the LED off
  }
}


void OpenDoor()
{
  // turn on motor A to open door
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  // set speed to 200 out of possible range 0~255
  analogWrite(enA, 200);

  if(digitalRead(REED_OPENED == LOW)) {
    // now turn off motors when door is opened
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }  
}


void CloseDoor()
{
  // turn on motor A to close door
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  // set speed to 200 out of possible range 0~255
  analogWrite(enA, 200);

  if(digitalRead(REED_CLOSED == LOW)) {
    // now turn off motors when door is closed
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }   
}

int DoorTimeCheck (){
  // is it time to open or close the door?
  // get current time
  tardis.TimeZone(TIMEZONE * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are
  DateTime now = rtc.now();
  int nowDay = now.day();
  int nowMonth = now.month();
  int nowYear = now.year();
  int nowHour = now.hour();
  int nowMinute = now.minute();
  float nowTime = nowHour + (static_cast<double>(nowMinute)/100);

  // is is sunrise?
  int sunriseHour = 0;
  int sunriseMinute = 0;

  byte sunrise[] = {  
    0, 0, 12, nowDay, nowMonth, nowYear      }; // store today's date (at noon) in an array for TimeLord to use
  if (tardis.SunRise(sunrise)) {// if the sun will rise today (it might not, in the [ant]arctic)
    sunriseHour = ((int) sunrise[tl_hour]);
    sunriseMinute = ((int) sunrise[tl_minute]);
  }

  float sunriseTime = sunriseHour + (static_cast<double>(sunriseMinute)/100) ;

  // if now is sunrise time: open the door
  if (nowTime  == sunriseTime){
    digitalWrite(LED_DOOR_OPEN, HIGH);   // turn the LED on
    digitalWrite(LED_DOOR_CLOSE, LOW);   // turn the LED off 
    OpenDoor();
  }

  // Is it sunset?
  int sunsetHour = 0;
  int sunsetMinute = 0;

  byte sunset[] = {  
    0, 0, 12, nowDay, nowMonth, nowYear      }; // store today's date (at noon) in an array for TimeLord to use
  if (tardis.SunSet(sunset)) {// if the sun will rise today (it might not, in the [ant]arctic)
    sunsetHour = ((int) sunset[tl_hour]);
    sunsetMinute = ((int) sunset[tl_minute]);
  }


  float sunsetTime;
  if (sunsetMinute <= 30){
    sunsetTime = sunsetHour + (static_cast<double>(sunsetMinute)/100) + 0.30 ;//close door 30min after sunset
  }
  else{
    float diff = sunsetMinute - 30;
    sunsetHour++;
    sunsetMinute = diff;
    sunsetTime = sunsetHour + (static_cast<double>(sunsetMinute)/100) ;

  }

  // if now is sunset time + 30 min: close the door
  if (nowTime  == sunsetTime){
    digitalWrite(LED_DOOR_OPEN, LOW);   // turn the LED off
    digitalWrite(LED_DOOR_CLOSE, HIGH);   // turn the LED on 
    CloseDoor();
  }
}


int LightTimeCheck (){
  // is it time to open or close the door?
  // get current time
  tardis.TimeZone(TIMEZONE * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are
  DateTime now = rtc.now();
  int nowDay = now.day();
  int nowMonth = now.month();
  int nowYear = now.year();
  int nowHour = now.hour();
  int nowMinute = now.minute();
  float nowTime = nowHour + (static_cast<double>(nowMinute)/100);

  // is is sunrise?
  int sunriseHour = 0;
  int sunriseMinute = 0;

  byte sunrise[] = {  
    0, 0, 12, nowDay, nowMonth, nowYear      }; // store today's date (at noon) in an array for TimeLord to use
  if (tardis.SunRise(sunrise)) {// if the sun will rise today (it might not, in the [ant]arctic)
    sunriseHour = ((int) sunrise[tl_hour]);
    sunriseMinute = ((int) sunrise[tl_minute]);
  }

  float sunriseTime = sunriseHour + (static_cast<double>(sunriseMinute)/100) ;


  // Is it sunset?
  int sunsetHour = 0;
  int sunsetMinute = 0;

  byte sunset[] = {  
    0, 0, 12, nowDay, nowMonth, nowYear      }; // store today's date (at noon) in an array for TimeLord to use
  if (tardis.SunSet(sunset)) {// if the sun will rise today (it might not, in the [ant]arctic)
    sunsetHour = ((int) sunset[tl_hour]);
    sunsetMinute = ((int) sunset[tl_minute]);
  }

  float  sunsetTime = sunsetHour + (static_cast<double>(sunsetMinute)/100);

  float diffTime = sunsetTime - sunriseTime;
 
    // if day length is less than 14 hrs turn on light
  if (diffTime < 14 && nowTime == (sunsetTime - 1)){
    digitalWrite(LED_DAYLEN, HIGH);   // turn the LED on 
  }
  if (diffTime < 14 && nowTime == (sunriseTime + diffTime)){
    digitalWrite(LED_DAYLEN, LOW);   // turn the LED off
  }
  
}


void DisplaySunriseSunset (){ 
  tardis.TimeZone(TIMEZONE * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are
  DateTime now = rtc.now();

  int nowDay = now.day();
  int nowMonth = now.month();
  int nowYear = now.year();
  int nowHour = now.hour();
  int nowMinute = now.minute();
  lcd.clear();
  byte today[] = {  
    0, 0, 12, nowDay, nowMonth, nowYear      }; // store today's date (at noon) in an array for TimeLord to use
  lcd.setCursor(0, 0);
  if (tardis.SunRise(today)) // if the sun will rise today (it might not, in the [ant]arctic)
  {
    lcd.print("Sunrise: ");
    lcd.print((int) today[tl_hour]);
    lcd.print(":");
    lcd.print((int) today[tl_minute]);
  }
  lcd.setCursor(0, 1);
  if (tardis.SunSet(today)) // if the sun will set today (it might not, in the [ant]arctic)
  {
    lcd.print(" ");
    lcd.print("Sunset: ");
    lcd.print((int) today[tl_hour]);
    lcd.print(":");
    lcd.print((int) today[tl_minute]);
  }
  delay(2000);
}

void DisplayDateTime (){
  // We show the current date and time
  DateTime now = rtc.now();
  monthupg=now.month();
  dayupg=now.day();
  yearupg=now.year();
  hourupg=now.hour();
  minupg=now.minute();
  secupg=now.second();
  lcd.setCursor(0, 0);  
  if (monthupg<=9) {
    lcd.print("0");
  }
  lcd.print(monthupg, DEC);
  lcd.print("/");
  if (dayupg<=9) {
    lcd.print("0");
  }
  lcd.print(dayupg, DEC);
  lcd.print("/");
  lcd.print(yearupg, DEC);

  lcd.setCursor(0, 1);

  if (hourupg<=9) {
    lcd.print("0");
  }
  lcd.print(hourupg, DEC);
  lcd.print(":");

  if (minupg<=9) {
    lcd.print("0");
  }
  lcd.print(minupg, DEC);
  lcd.print(":");
  if (secupg<=9) {
    lcd.print("0");
  }
  lcd.print(secupg, DEC);  
  lcd.setCursor(9, 1);  
  DisplayTemp();
}


void DisplayTemp(){
  int reading = analogRead(tempSensorPin);  

  // converting that reading to voltage, for 3.3v arduino use 3.3
  float voltage = reading * 5.0;
  voltage /= 1024.0; 

  // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
  //to degrees ((voltage - 500mV) times 100)
  // now convert to Fahrenheit
  float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  lcd.print(temperatureF); 
  lcd.print(" F");

  if(temperatureF > low_temp_level and temperatureF < high_temp_level ){    
    digitalWrite(GOOD_TEMP_INDICATOR,HIGH);
    digitalWrite(LOW_TEMP_WARNING, LOW);
    digitalWrite(HIGH_TEMP_WARNING, LOW);   
  }
  else if (temperatureF <= low_temp_level){
    digitalWrite(GOOD_TEMP_INDICATOR,LOW);
    digitalWrite(LOW_TEMP_WARNING, HIGH);    
  }
  else if(temperatureF >= high_temp_level){
    digitalWrite(GOOD_TEMP_INDICATOR,LOW);
    digitalWrite(HIGH_TEMP_WARNING, HIGH);        
  } 
}


void DisplaySetHour(){
  // time setting

  lcd.clear();
  DateTime now = rtc.now();

  if(digitalRead(P2)==HIGH){
    if(hourupg==23){
      hourupg=0;
    }
    else {
      hourupg=hourupg+1;
    }
  }

  if(digitalRead(P3)==HIGH) {
    if(hourupg==0) {
      hourupg=23;
    }
    else  {
      hourupg=hourupg-1;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Set Hour:");
  lcd.setCursor(0,1);
  lcd.print(hourupg,DEC);
  delay(200);
}



void DisplaySetMinute(){

  // Setting the minutes
  lcd.clear();

  if(digitalRead(P2)==HIGH){
    if (minupg==59){
      minupg=0;
    }
    else {
      minupg=minupg+1;
    }
  }
  if(digitalRead(P3)==HIGH) {
    if (minupg==0){
      minupg=59;
    }
    else{
      minupg=minupg-1;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Set Minutes:");
  lcd.setCursor(0,1);
  lcd.print(minupg,DEC);
  delay(200);
}



void DisplaySetYear(){

  // setting the year
  lcd.clear();
  if(digitalRead(P2)==HIGH) {    
    yearupg=yearupg+1;
  }
  if(digitalRead(P3)==HIGH){
    yearupg=yearupg-1;
  }
  lcd.setCursor(0,0);
  lcd.print("Set Year:");
  lcd.setCursor(0,1);
  lcd.print(yearupg,DEC);
  delay(200);
}



void DisplaySetMonth(){

  // Setting the month
  lcd.clear();
  if(digitalRead(P2)==HIGH) {
    if (monthupg==12){
      monthupg=1;
    } 
    else {
      monthupg=monthupg+1;
    }
  }
  if(digitalRead(P3)==HIGH){
    if (monthupg==1) {
      monthupg=12;
    }
    else {
      monthupg=monthupg-1;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Set Month:");
  lcd.setCursor(0,1);
  lcd.print(monthupg,DEC);
  delay(200);
}



void DisplaySetDay(){

  // Setting the day
  lcd.clear();
  if(digitalRead(P2)==HIGH) {
    if (dayupg==31){
      dayupg=1;
    } 
    else{
      dayupg=dayupg+1;
    }
  }
  if(digitalRead(P3)==HIGH) {
    if (dayupg==1){
      dayupg=31;
    } 
    else {
      dayupg=dayupg-1;
    }
  }

  lcd.setCursor(0,0);
  lcd.print("Set Day:");
  lcd.setCursor(0,1);
  lcd.print(dayupg,DEC);
  delay(200);

}


void DisplaySetLowTemp(){

  lcd.clear();
  if(digitalRead(P2)==HIGH) {
    low_temp_level=low_temp_level+1;
  }
  if(digitalRead(P3)==HIGH) {
    low_temp_level=low_temp_level-1;
  }

  lcd.setCursor(0,0);
  lcd.print("Set Low Temp:");
  lcd.setCursor(0,1);
  lcd.print(low_temp_level,DEC);
  delay(200);

}

void DisplaySetHighTemp(){


  lcd.clear();
  if(digitalRead(P2)==HIGH) {
    high_temp_level=high_temp_level+1;
  }
  if(digitalRead(P3)==HIGH) {
    high_temp_level=high_temp_level-1;
  }

  lcd.setCursor(0,0);
  lcd.print("Set High Temp:");
  lcd.setCursor(0,1);
  lcd.print(high_temp_level,DEC);
  delay(200);

}


void StoreAgg(){

  // Variable saving
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SAVING IN");
  lcd.setCursor(0,1);
  lcd.print("PROGRESS");
  rtc.adjust(DateTime(yearupg,monthupg,dayupg,hourupg,minupg,0));
  delay(1000);
  lcd.clear();
}



