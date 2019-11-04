#include <TimeLord.h>
//#include <Time.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 rtc;
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2); // set the LCD address to 0x27 for a 16 chars and 2 line display

// longitude (west values negative) and latitude (south values negative)
float const LONGITUDE = -111.7677;
float const LATITUDE = 41.1180;

//**** TIMEZONE ****/
int DSTime = -6;
int StandardTime = -7;
int TIMEZONE = StandardTime;

// RESET DATE/TIME
int reset = 1;

//*********** TEMP SENSORS ****************// 
#include <DallasTemperature.h>
#include <OneWire.h>

// Data wire is conntec to the Arduino digital pin 4
const int oneWireBus = 4;  

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);


//************ SWITCHES *****************//
int P1=43; // Button SET MENU'
int P2=41; // Button +
int P3=42; // Button -
int P4=44; // daily sunrise/sunset
int P5=45; // daily temp high/low
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
int LED_DAYLEN = 10;

//********** MOTOR ********
int enA = 15; // ~pwm
int in1 = 17;
int in2 = 19;

//************Variables**************//
int hourupg;
int minupg;
int yearupg;
int monthupg;
int dayupg;
int secupg;



// day light led and day length
long int  lightOn = 0; //manual light 0=off millistimestamp=ON

// how long should the chicks have light?
int daylen = 14; 

// door speed
int door_close_speed = 200;
int door_open_speed = 200;

// temp level 
float low_temp_level = 32.0;
float high_temp_level = 95.0;

DateTime Sunrise;
DateTime Sunset;

float tempHigh = -1000.00; // setup temp
float tempLow = 1000.00; // setup temp

// menu default
int menu=0; // set time and date and temp

TimeLord tardis; 

void setup(){

  rtc.begin();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    turnOnLCD();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC is NOT running!");
    delay(3000);  
    turnOffLCD();
  }

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (reset == 1){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are
  lcd.init(); //initialize the lcd


  pinMode(P1,INPUT); // set
  pinMode(P2,INPUT); // +
  pinMode(P3,INPUT); // -

  pinMode(P4, INPUT); // display daily sunrise sunset times
  pinMode(P5, INPUT); // display daily high low temp
  pinMode(P6, INPUT); // manual door override
  pinMode(P7, INPUT); // manual DAYLEN LED override

  pinMode(REED_CLOSED,INPUT); // lower reed for closed door on low/closed curcuit
  pinMode(REED_OPENED,INPUT); // upper reed for opened door on low/closed curcuit

  pinMode(LED_DOOR_OPEN, OUTPUT); 
  pinMode(LED_DOOR_CLOSE, OUTPUT);
  pinMode(HIGH_TEMP_WARNING, OUTPUT);
  pinMode(GOOD_TEMP_INDICATOR, OUTPUT);
  pinMode(LOW_TEMP_WARNING, OUTPUT);
  pinMode(LED_DAYLEN, OUTPUT);


  Serial.begin(9600);
  Wire.begin();
  sensors.begin();

  int menu  = 0;
}

void loop() {

  DayLightSavingCheck();
  DoorTimeCheck();
  LightTimeCheck();
  ManualLightTimer(); 
  
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  
  recordTemp();
  CheckDoorStatus();
  delay(500);   

  if(digitalRead(P6)==LOW){
    turnOnLCD();
    ManualDoorOverride();
    menu=0;
    turnOffLCD();
   
  } 
  if(digitalRead(P7)==LOW){

    turnOnLCD();
    ManualDayLenLightOverride();
    delay(500);
    turnOffLCD();
  }  
  if(digitalRead(P1)==LOW) { // set
    turnOnLCD();
    menu=menu+1;
    turnOffLCD();
  }

  if(digitalRead(P4)==LOW) { // sunrise sunset
    turnOnLCD();
    DisplaySunriseSunset();
    turnOffLCD();
  }
  if(digitalRead(P5)==LOW) { // high low temp
    turnOnLCD();
    DisplayDailyHighLowTemp();
    turnOffLCD();
  }
  

 

  if (menu>0){
    turnOnLCD();
  }

  if (menu==0){
    DisplayDateTime(); 
    menu=0;   
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
    DisplaySetDoorOpenSpeed();
  } 
  if (menu==9){
    DisplaySetDoorCloseSpeed();
  } 
  if (menu==10){
    DisplaySetDayLen();
  }   
  
  if (menu==11){
    StoreAgg(); 
    delay(500);
    menu=0;
    turnOffLCD();
  }
  delay(100);
}

void ManualLightTimer(){

      // if manual light on, only keep on for 5min(300,000ms)
    if (lightOn > 0 and ( (millis() - lightOn) > 300000) ){
       digitalWrite(LED_DAYLEN,LOW);
       lightOn=0;
    }
}

void recordTemp(){
  
  float temperatureF = sensors.getTempFByIndex(0);
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
  lcd.print(tempLow,0); 
  lcd.print("F");
  lcd.setCursor(0, 1);
  lcd.print("Daily High: ");
  lcd.print(tempHigh,0  );   
  lcd.print("F");
  delay(3000);
  lcd.clear();
}




void DayLightSavingCheck(){
  DateTime now = rtc.now();
  byte nowDay = now.day();
  byte nowMonth = now.month();
  // March 3 -> Nov 10 is DST
  // Rest of the year is Standard Time 
  if (  (nowMonth == 3 and nowDay >= 10) or (nowMonth >3 and nowMonth <11) or (nowMonth ==11 and nowDay < 3) ){
    TIMEZONE = DSTime;
  }else{
    TIMEZONE = StandardTime;
  } 
  tardis.TimeZone(TIMEZONE * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
    Serial.print("\n");
     Serial.print("\n");
     Serial.print("DST(-6) or STANDARD(-7)");
     Serial.print("\n");
     Serial.print(TIMEZONE);
     Serial.print("\n");

}



// LOW means the magnet is close to the reed switch
// HIGH means the magnet is not close to the switch

void CheckDoorStatus(){
  
  if(digitalRead(REED_OPENED) == LOW) { //|| digitalRead(REED_CLOSED) == HIGH) { //If the upper door reed pin reads low, the switch is closed. and the door is open
    digitalWrite(LED_DOOR_OPEN, HIGH);   // turn the LED on
    digitalWrite(LED_DOOR_CLOSE, LOW);   // turn the LED off 
  }
  else if (digitalRead(REED_CLOSED) == LOW) { //|| digitalRead(REED_OPENED) == HIGH ){ //If the lower reed pin reads low, the switch is closed. and the door is closed. or if hte open reed is far from magnet it is closed
    digitalWrite(LED_DOOR_OPEN, LOW);   // turn the LED on
    digitalWrite(LED_DOOR_CLOSE, HIGH);   // turn the LED off
  } else { // neither opened or closed, turn on both lights
    digitalWrite(LED_DOOR_OPEN, HIGH);   // turn the LED on
    digitalWrite(LED_DOOR_CLOSE, HIGH);   // turn the LED on
  }
}

void ManualDoorOverride(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door");
  delay(500);
  if(digitalRead(REED_OPENED) == LOW ||digitalRead(REED_CLOSED) == HIGH) { //If the upper door reed pin reads low, the switch is closed. and the door is open
    CloseDoor();
  }
  else if (digitalRead(REED_CLOSED) == LOW ||digitalRead(REED_OPENED) == HIGH ){ //If the lower reed pin reads low, the switch is closed. and the door is closed. or if hte open reed is far from magnet it is closed
    OpenDoor();
  }
}

void ManualDayLenLightOverride(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light");
  delay(500);
  if(digitalRead(LED_DAYLEN) == LOW) { //if the day length led is off turn it on
    lightOn = millis(); // timestamp time light is turned on
    lcd.setCursor(0, 0);
    lcd.print("light going on");
    delay(500);
    digitalWrite(LED_DAYLEN, HIGH);   // turn the LED on
  }
  else if (digitalRead(LED_DAYLEN) == HIGH){ //if the day length led is on turn it off
    lcd.setCursor(0, 0);
    lcd.print("light going off");
    delay(500);
    digitalWrite(LED_DAYLEN, LOW);   // turn the LED off
    lightOn=0;
  }
}


void OpenDoor()
{ 
  if ( digitalRead(REED_OPENED) == HIGH) { // if door is not open, open it
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Opening");
    delay(500);
    long int tooLong = millis()+5000; // 5sec
    while( digitalRead(REED_OPENED) == HIGH || millis() > tooLong) {
      // while the open reed is not connected turn on motor A to open door
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      // set speed to 200 out of possible range 0~255
      analogWrite(enA, door_open_speed);
    }// should exit while loop as soon as REED_OPENED is not HIGH

    // now turn off motors when door is opened
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);  
    
    digitalWrite(LED_DOOR_OPEN, HIGH);   // turn the LED on
    digitalWrite(LED_DOOR_CLOSE, LOW);   // turn the LED off 

    lcd.clear();

  }
}


void CloseDoor()
{

  if ( digitalRead(REED_CLOSED) == HIGH) { // if door is not closed, close it
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Closing");
    delay(500);
    long int tooLong = millis()+5000; // 5sec
    while( digitalRead(REED_CLOSED) == HIGH || millis() > tooLong) {
     // turn on motor A to close door
     digitalWrite(in1, LOW);
     digitalWrite(in2, HIGH);
     // set speed to 200 out of possible range 0~255
     analogWrite(enA, door_close_speed); 
   }// should exit while loop as soon as REED_CLOSED is not HIGH
   // now turn off motors when door is closed
   digitalWrite(in1, LOW);
   digitalWrite(in2, LOW);
   lcd.clear();  

 }
 digitalWrite(LED_DOOR_OPEN, LOW);   // turn the LED off
 digitalWrite(LED_DOOR_CLOSE, HIGH);   // turn the LED on  
}

void DoorTimeCheck (){
  // is it time to open or close the door?
  // get current time


  DateTime now = rtc.now();
  byte nowDay = now.day();
  byte nowMonth = now.month();
  byte nowYear = now.year();
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


  float sunsetTime   = sunsetHour + (static_cast<double>(sunsetMinute)/100);
  float doorCloseTime;
  
  if (sunsetMinute < 30){
    doorCloseTime = sunsetHour + (static_cast<double>(sunsetMinute)/100) + 0.30 ;//close door 30min after sunset
  }
  else{
    float diff = sunsetMinute - 30;
    sunsetHour++;
    sunsetMinute = diff;
    doorCloseTime = sunsetHour + (static_cast<double>(sunsetMinute)/100) ;
  }

  // if now is sunset time + 30 min: close the door
  if (nowTime  == doorCloseTime){
    CloseDoor();
  }
     Serial.print("\n");
     Serial.print("\n");
     Serial.print("DOOR TIME CHECK");
     Serial.print("\n");
     Serial.print("nowtime: ");
     Serial.print(nowTime);
     Serial.print("\n");
     Serial.print("sunsettime: ");
     Serial.print(sunsetTime);
     Serial.print("\n");
     Serial.print("doorcloseTime: ");
     Serial.print(doorCloseTime);
     Serial.print("\n");
     Serial.print("close door if ");
     Serial.print(nowTime);    
     Serial.print(" = ");
     Serial.print(doorCloseTime);     
     Serial.print("\n");
     Serial.print("\n");
  
}


void LightTimeCheck(){
  // get current time
  DateTime now = rtc.now();
  byte nowDay = now.day();
  byte nowMonth = now.month();
  byte nowYear = now.year();
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

  int diffTime = round(sunsetTime - sunriseTime);
 
   // if day length is less than daylen hrs turn on light before sunrise 
   // default daylen=12
  if (diffTime < daylen && nowTime == (sunriseTime - (daylen - diffTime))){
    digitalWrite(LED_DAYLEN, HIGH);   // turn the LED on 
  }
  // turn light off 2 hrs after sunrise
  if (diffTime < daylen && nowTime == (sunriseTime + 2)){
    digitalWrite(LED_DAYLEN, LOW);   // turn the LED off
  }
  Serial.print("\n");
  Serial.print("LIGHT TIME CHECK");
  Serial.print("\n");
  Serial.print("nowtime: ");
  Serial.print(nowTime);
  Serial.print("\n");
  Serial.print("difftime: ");
  Serial.print(diffTime);
  Serial.print("\n");
  Serial.print("sunrisetime: ");
  Serial.print(sunriseTime);
  Serial.print("\n");
  Serial.print("sunsettime: ");
  Serial.print(sunsetTime);
  Serial.print("\n");
  Serial.print(" light on time: ");
  Serial.print(sunriseTime - (daylen - diffTime));
  Serial.print("\n");
  Serial.print(" light off time: ");
  Serial.print(sunriseTime + 2);    
  Serial.print("\n");
}


void DisplaySunriseSunset(){ 
  

  DateTime now = rtc.now();

  byte nowDay = now.day();
  byte nowMonth = now.month();
  byte nowYear = now.year();
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
    if ((int) today[tl_minute] < 10){
      lcd.print(0);
    }
    lcd.print((int) today[tl_minute]);
  }
  lcd.setCursor(0, 1);
  if (tardis.SunSet(today)) // if the sun will set today (it might not, in the [ant]arctic)
  {
    lcd.print(" ");
    lcd.print("Sunset: ");
    lcd.print((int) today[tl_hour]);
    lcd.print(":");
    if ((int) today[tl_minute] < 10){
      lcd.print(0);
    }
    lcd.print((int) today[tl_minute]);
  }
  delay(2500);
  lcd.clear();

   byte sunset[] = {  
    0, 0, 12, nowDay, nowMonth, nowYear      }; // store today's date (at noon) in an array for TimeLord to use
  lcd.setCursor(0, 0);
  if (tardis.SunSet(sunset)) // if the sun will rise today (it might not, in the [ant]arctic)
  {

  int sunsetHour = ((int) sunset[tl_hour]);
  int sunsetMinute = ((int) sunset[tl_minute]);
    
    lcd.print("Sunset:  ");
    lcd.print((int) sunsetHour);
    lcd.print(":");
    if (sunsetMinute < 10){
      lcd.print(0);
    }
    lcd.print((int) sunsetMinute);

    lcd.setCursor(0, 1);
    //float doorcloseTime;
    int doorCloseHour;
    int doorCloseMin;
    if (sunsetMinute < 30){
      //doorcloseTime = sunsetHour + (static_cast<double>(sunsetMinute)/100) + 0.30 ;//close door 30min after sunset
      doorCloseMin = sunsetMinute + 30;
      doorCloseHour = sunsetHour;
    }
    else{
      float diff = sunsetMinute - 30;
      doorCloseHour = sunsetHour + 1;
      doorCloseMin =  diff;
 
      //doorcloseTime = sunsetHour + (static_cast<double>(sunsetMinute)/100) ;
    }


    lcd.print("Closing: ");
    lcd.print((int) doorCloseHour);
    lcd.print(":");
    if (doorCloseMin < 10){
      lcd.print(0);
    }
    lcd.print((int) doorCloseMin);


  }
  delay(3000);



  
  lcd.clear();

}

void DisplayDateTime (){
  lcd.clear();
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

  float temperatureF = sensors.getTempFByIndex(0);
  lcd.print(temperatureF,1); 
  lcd.print("F");

  if(temperatureF > low_temp_level && temperatureF < high_temp_level ){    
    digitalWrite(GOOD_TEMP_INDICATOR,HIGH);
    digitalWrite(LOW_TEMP_WARNING, LOW);
    digitalWrite(HIGH_TEMP_WARNING, LOW);   
  }
  else if (temperatureF <= low_temp_level){
    digitalWrite(GOOD_TEMP_INDICATOR,LOW);
    digitalWrite(LOW_TEMP_WARNING, HIGH);    
    digitalWrite(HIGH_TEMP_WARNING, LOW);
  }
  else if(temperatureF >= high_temp_level){
    digitalWrite(GOOD_TEMP_INDICATOR,LOW);
    digitalWrite(LOW_TEMP_WARNING, LOW);
    digitalWrite(HIGH_TEMP_WARNING, HIGH);        
  } 
}


void DisplaySetHour(){
  // time setting

  lcd.clear();
  DateTime now = rtc.now();

  if(digitalRead(P2)==LOW){
    if(hourupg==23){
      hourupg=0;
    }
    else {
      hourupg=hourupg+1;
    }
  }

  if(digitalRead(P3)==LOW) {
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
  delay(100);
}



void DisplaySetMinute(){

  // Setting the minutes
  lcd.clear();

  if(digitalRead(P2)==LOW){
    if (minupg==59){
      minupg=0;
    }
    else {
      minupg=minupg+1;
    }
  }
  if(digitalRead(P3)==LOW) {
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
  delay(100);
}



void DisplaySetYear(){

  // setting the year
  lcd.clear();
  if(digitalRead(P2)==LOW) {    
    yearupg=yearupg+1;
  }
  if(digitalRead(P3)==LOW){
    yearupg=yearupg-1;
  }
  lcd.setCursor(0,0);
  lcd.print("Set Year:");
  lcd.setCursor(0,1);
  lcd.print(yearupg,DEC);
  delay(100);
}



void DisplaySetMonth(){

  // Setting the month
  lcd.clear();
  if(digitalRead(P2)==LOW) {
    if (monthupg==12){
      monthupg=1;
    } 
    else {
      monthupg=monthupg+1;
    }
  }
  if(digitalRead(P3)==LOW){
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
  delay(100);
}



void DisplaySetDay(){

  // Setting the day
  lcd.clear();
  if(digitalRead(P2)==LOW) {
    if (dayupg==31){
      dayupg=1;
    } 
    else{
      dayupg=dayupg+1;
    }
  }
  if(digitalRead(P3)==LOW) {
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
  delay(100);

}


void DisplaySetDoorCloseSpeed(){

  lcd.clear();
  if(digitalRead(P2)==LOW) {
    door_close_speed=door_close_speed+5;
  }
  if(digitalRead(P3)==LOW) {
    door_close_speed=door_close_speed-5;
  }
  if (door_close_speed < 135){
    door_close_speed = 135; 
  }
  if (door_close_speed > 255){
    door_close_speed = 255; 
  }
  lcd.setCursor(0,0);
  lcd.print("Set DoorCloseSpd:");
  lcd.setCursor(0,1);
  lcd.print(door_close_speed,DEC);
  delay(100);
}

void DisplaySetDoorOpenSpeed(){

  lcd.clear();
  if(digitalRead(P2)==LOW) {
    door_open_speed=door_open_speed+5;
  }
  if(digitalRead(P3)==LOW) {
    door_open_speed=door_open_speed-5;
  }
  if (door_open_speed < 135){
    door_open_speed = 135; 
  }
  if (door_open_speed > 255){
    door_open_speed = 255; 
  }
  lcd.setCursor(0,0);
  lcd.print("Set DoorOpenSpd:");
  lcd.setCursor(0,1);
  lcd.print(door_open_speed,DEC);
  delay(100);
}


void DisplaySetDayLen(){

  lcd.clear();
  if(digitalRead(P2)==LOW) {
    daylen=daylen+1;
  }
  if(digitalRead(P3)==LOW) {
    daylen=daylen-1;
  }

  lcd.setCursor(0,0);
  lcd.print("Set Day Len:");
  lcd.setCursor(0,1);
  lcd.print(daylen,DEC);
  delay(100);

}


void DisplaySetLowTemp(){

  lcd.clear();
  if(digitalRead(P2)==LOW) {
    low_temp_level=low_temp_level+1;
  }
  if(digitalRead(P3)==LOW) {
    low_temp_level=low_temp_level-1;
  }

  lcd.setCursor(0,0);
  lcd.print("Set Low Temp:");
  lcd.setCursor(0,1);
  lcd.print(low_temp_level,0);
  delay(100);

}

void DisplaySetHighTemp(){


  lcd.clear();
  if(digitalRead(P2)==LOW) {
    high_temp_level=high_temp_level+1;
  }
  if(digitalRead(P3)==LOW) {
    high_temp_level=high_temp_level-1;
  }

  lcd.setCursor(0,0);
  lcd.print("Set High Temp:");
  lcd.setCursor(0,1);
  lcd.print(high_temp_level,0);
  delay(100);

}

void turnOnLCD(){
  lcd.backlight(); //open the backlight 
  lcd.begin(16, 2);
  lcd.clear();
}
void turnOffLCD(){
  lcd.noBacklight(); //open the backlight 
  lcd.clear();
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
