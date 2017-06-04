#include "pitches.h"
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h> // Inclusion de la librairie OneWire
#include <Wire.h>

//Capteur de température
#define DS18B20 0x28     // Adresse 1-Wire du DS18B20

#define BROCHE_ONEWIRE 7 // Broche utilisée pour le bus 1-Wire

//wifi
//char serialbuffer[150];//serial buffer for request url
//String NomduReseauWifi2 = "freebox_manon"; // Garder les guillements
//String NomduReseauWifi = "NSAisWatchingYou"; // Garder les guillements
//String MotDePasse2      = "AB5ADE27D8"; // Garder les guillements
//String MotDePasse      = "epthfmlKsKih!"; // Garder les guillements
//int wifiRestartCounter = 0;
//String ApiKey          = "YCBS447TES9C1OTU";
//#define IP "184.106.153.149" // thingspeak.com
//String GET = "GET /update?key=YCBS447TES9C1OTU&field1=";
//int lastSending = -10;
//#define sendFrequency 1  //en minutes

//Pins
//int buzzerPin = 4;

#define blueLedPin 20
#define greenLedPin 19
#define redLedPin 18

#define forceModePin 9
int forceModePinState = 0;
#define foodModePin 6
int foodModePinState = 0;
boolean feeding = false;     //Pour savoir si la bouffe est en cours de distribution
long feedingInitTime = 0;

//relais
#define relaiLumiere 14      //La lumière                   
#define relaiChauffage 16 
#define relaiPompe 15                     
#define relaiBulleur 10                      

DateTime now; 
int dayTime = 0;
boolean lights = false;
int hour =0;

////////////////////////////
////CARACTÉRISTIQUES////////
int morningTime = 11; //inclus
int eveningTime = 20;   //inclus
float temp=0;
float targetTemp = 25;
float deltaTemp = 0.25f;
float deltaAlert = 0.75f;

//int horaireReveil[] = {06,25};

int ledIntensite = 255;  //Valeur de 0 à 255 (0 = fort, 255 = éteint)
int greenLedisOn, redLedisOn, blueLedisOn = 0; 

///////////////////////////
///////////////////////////

//compteurs
float GoodTempCounter = 0;
float BadTempCounter = 0;
int thisMonth;
float lastTemp = 0;


boolean ForceMode = false;
int forceModePushed = 0;
int heatMode = 0;
int mute = 1;

//alertes
boolean alertTemp = false;
boolean alertPH = false;

//Écran
LiquidCrystal_I2C lcd(0x27,20,4); 

//Horloge
RTC_DS1307 RTC; //L'horloge RTC

///////////////////////
//// TEMPERATURE///////
//////////////////////
OneWire ds(BROCHE_ONEWIRE); // Création de l'objet OneWire ds

// Fonction récupérant la température depuis le DS18B20
// Retourne true si tout va bien, ou false en cas d'erreur
boolean getTemperature(float *temp)
{
	byte data[9], addr[8];
  // data : Données lues depuis le scratchpad
  // addr : adresse du module 1-Wire détecté

  if (!ds.search(addr)) { // Recherche un module 1-Wire
    ds.reset_search();    // Réinitialise la recherche de module
    return false;         // Retourne une erreur
}

   if (OneWire::crc8(addr, 7) != addr[7]) // Vérifie que l'adresse a été correctement reçue
    return false;                        // Si le message est corrompu on retourne une erreur

  if (addr[0] != DS18B20) // Vérifie qu'il s'agit bien d'un DS18B20
    return false;         // Si ce n'est pas le cas on retourne une erreur

  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sélectionne le DS18B20

  ds.write(0x44, 1);      // On lance une prise de mesure de température
  delay(1000);             // Et on attend la fin de la mesure

  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sélectionne le DS18B20
  ds.write(0xBE);         // On envoie une demande de lecture du scratchpad

  for (byte i = 0; i < 9; i++) // On lit le scratchpad
    data[i] = ds.read();       // Et on stock les octets reçus

  // Calcul de la température en degré Celsius
  *temp = ((data[1] << 8) | data[0]) * 0.0625; 

  // Pas d'erreur
  return true;
}

///////////////////////
//////////LIGHT////////
///////////////////////
void ChangeLight(boolean lightOn)
{
	if(lightOn){
		digitalWrite(relaiLumiere, LOW);
    lights = true;
	}

	if(!lightOn){
		digitalWrite(relaiLumiere, HIGH);
    lights = false;
	}
}

void SwitchLeds(int blue, int green, int red)
{   
      blueLedisOn = blue;
      greenLedisOn = green;
      redLedisOn = red;
      
      digitalWrite(redLedPin, 1-red);
      digitalWrite(greenLedPin, 1-green);
      digitalWrite(blueLedPin, 1-blue);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////SETUP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

 
  //Display init
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(5, 0);  // (Colonne,ligne)
  lcd.print("AUTOFISH");
  lcd.setCursor(4, 1);
  lcd.print("says Hello");
  //PlayMusic(1);
  Serial.println("End of introduction...");
  //delay(2000);
  Serial.begin(9600);
  //Serial1.begin(115200);
    Serial.println("Displaying introduction...");

  //wifi
  //initESP8266();

  //SendToWifi("206");

  
  pinMode(blueLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(blueLedPin, HIGH);
  pinMode(forceModePin, INPUT_PULLUP);
  pinMode(foodModePin, INPUT_PULLUP);

  pinMode(relaiChauffage,OUTPUT);
  digitalWrite(relaiChauffage,LOW);
  delay(50);

  pinMode(relaiLumiere,OUTPUT);
  digitalWrite(relaiLumiere, HIGH);
  delay(50);
 
  //analogWrite(redLedPin,255);
  //analogWrite(blueLedPin,255);
  //analogWrite(greenLedPin,255);

  //Display init
  //lcd.init(); 
  //lcd.backlight();
  //lcd.setCursor(5, 0);  // (Colonne,ligne)
  //Serial.println("Displaying introduction...");
  //lcd.print("AUTOFISH");
  //lcd.setCursor(4, 1);
  //lcd.print("says Hello");
  //PlayMusic(1);
  Serial.println("End of introduction...");
  //SwitchLeds(0,0,1);
  RTC.begin();
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  DateTime initNow = RTC.now();
  thisMonth = initNow.month();
  //thisMonth = initNow.month();
  //Serial.println((String)initNow.day()+"/" + (String)initNow.month()+"/" + (String)initNow.year()); 
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////LOOP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
  
	if (! RTC.isrunning()) 
	Serial.println("RTC is NOT running!");
////////////////////////////
//////WIFI HAS SOMETHING TO SAY?
////////////////////////////




//output everything from ESP8266 to the Arduino Micro Serial output
  //while (Serial1.available() > 0) {
  //  Serial.write(Serial1.read());
  //}
  
  //if (Serial1.available() > 0) {
  //   //read from serial until terminating character
  //  int len = Serial.readBytesUntil('\n', serialbuffer, sizeof(serialbuffer));
  
     //trim buffer to length of the actual message
  //   String message = String(serialbuffer).substring(0,len-1);
  //  Serial.println("message: " + message);

  //}//output everything from ESP8266 to the Arduino Micro Serial output
  //while (Serial1.available() > 0) {
  //  Serial.write(Serial1.read());
  //}


///La date qui sert partout
now = RTC.now();

//PUSHBUTTON
foodModePinState = digitalRead(foodModePin);
if(foodModePinState == LOW)
{
  ChangeLight(!lights);
  Serial.print("Forcing the lights to change !");
  delay(1000);
}

/////////////////////////////
/////////RESET Mean date %////////
/////////EVERY MONTH////////
/////////////////////////////

//if(now.month() == thisMonth + 1 || now.month() - thisMonth > 9)
////if(now.month() == thisMonth + 1)
//{
//	thisMonth = now.month() ;
//	GoodTempCounter = 0;
//	BadTempCounter = 0;
//}


///////////////////////
//// TEMPERATURE///////
///////////////////////
temp = 0;
SwitchLeds(0,0,0);
if(getTemperature(&temp)) {	    // Affiche la température  // Lit la température ambiante à ~1Hz
	
	lastTemp = temp;
	Serial.print("Temperature : ");
	Serial.print(temp);
  Serial.print(" degrés");
  //Serial.print(", heatmode =");
  //Serial.println(heatMode);
  //Serial.println((String)now.day()+"/" + (String)now.month()+"/" + (String)now.year() +", " + (String)now.hour() + "h " + (String)now.minute() + "min " + (String)now.hour() + " sec"); 
    
    ////LED
    if(temp !=0){
    if(temp > targetTemp + deltaAlert){
      if (redLedisOn == 0)
          SwitchLeds(0,0,1);
          Serial.println("Temp is pretty hot");
    }

    else if (temp < targetTemp - deltaAlert){
        if(blueLedisOn == 0)
          SwitchLeds(1,0,0);
                    Serial.println("Temp is pretty cold");
    }

    //if (temp <= (targetTemp + deltaAlert)  && temp >= (targetTemp - deltaAlert))
    else{
        if (greenLedisOn == 0)
          SwitchLeds(0,1,0);
                    Serial.println("Temp is pretty good");
    }
    }
    // RELAI
    //Finalement on gère le relai
    if(temp !=0 && temp <= targetTemp - deltaTemp)
    {
    	digitalWrite(relaiChauffage,HIGH);
    	heatMode = 1;
    } 
    else if(temp !=0 && temp >= targetTemp + deltaTemp)
    {
    	digitalWrite(relaiChauffage,LOW);
    	heatMode = 0;                                         
    } 
    delay(1000);
}
  //Serial.println((String)now.day()+"/" + (String)now.month()+"/" + (String)now.year() +", " + (String)now.hour() + "h " + (String)now.minute() + "min " + (String)now.hour() + " sec"); 

///////////////////////
///////// LIGHT///////
//////////////////////
///////////////////////
  hour = 0;
	hour = now.hour();
	if((hour >= morningTime && hour < eveningTime) && !dayTime)
	{
		dayTime = 1;
		ChangeLight(dayTime);
	}
	if((hour < morningTime || hour >= eveningTime) && dayTime)
	{
		dayTime = 0;
		ChangeLight(dayTime);
	} 

  



/*
///////////////////////
/////////DISPLAY///////
///////////////////////
////////////////////
///----LIGNE 1----//
lcd.setCursor(0, 0);  // (Colonne,ligne)
if (now.day() < 10)
lcd.print("0");
lcd.print(now.day());
lcd.print("/");
if (now.month() < 10)
lcd.print("0");
lcd.print(now.month());
lcd.print("/");
lcd.print(now.year());
lcd.print("  -  ");
if (now.hour() < 10)
lcd.print("0");
lcd.print(now.hour());
lcd.print("h");
if (now.minute() < 10)
lcd.print("0");
lcd.print(now.minute());

////////////////////
///----LIGNE 2----//
 lcd.setCursor(0, 1);  // (Colonne,ligne)
 lcd.print("Eau a ");
 lcd.print(lastTemp);
 lcd.print(" degres ");
 if(heatMode == 1)
 lcd.print("H");
 if(heatMode == 0)
 lcd.print("C");

////////////////////
///----LIGNE 3----//
lcd.setCursor(0, 2);  // (Colonne,ligne)
if(alertTemp){
  lcd.print("Alerte ! ");
}
else{
  if(dayTime == 1)
  lcd.print("Daytime, ");
   if(dayTime == 0)
  lcd.print("Night,   ");
}

if(dayTime == 1){
	lcd.print("nuit a ");
	lcd.print(eveningTime);
	lcd.print("h");
}

else {
	lcd.print("jour a ");
	lcd.print(morningTime);
	lcd.print("h");
}


//////////////////// 
///----LIGNE 4----//
lcd.setCursor(0, 3);  // (Colonne,ligne)
if(!ForceMode)  
lcd.print("MODE AUTO   ");
if(ForceMode)  
lcd.print("MODE FORCE  ");

lcd.print(100*(GoodTempCounter/(GoodTempCounter + BadTempCounter)),0);
lcd.print("%");

*/


}


//
///****************************************************************/
///*                Fonction qui initialise l'ESP8266             */
///****************************************************************/
//void initESP8266()
//{  
//  Serial.println("Restarting WiFi !");
//  lcd.setCursor(0,3);
//  lcd.print("ESP8266 Module init");
//  Serial.print("ESP8266 Module init");
//  //Serial1.println("AT");
//  //delay(2000);
//  //  while (Serial1.available() > 0) {
//  //  Serial.write(Serial1.read());
//  //}
//  Serial1.println("AT+RST");
//  delay(2000);
//  Serial1.println("AT+CWMODE=1");
//  lcd.setCursor(0,3);
//  lcd.print("Looking for WiFi...");
//  Serial.print("Looking for WiFi...");
//    delay(3000);
//  //Serial1.println("AT+RST");
//  //delay(2000);
//  //connect to wifi network
//  Serial1.println("AT+CWJAP=\""+ NomduReseauWifi + "\",\"" + MotDePasse +"\"");
//  delay(5000);
//  lcd.clear();
//}
//
///******************************************/
///*ENVOYER A THINGSPEAK*********************/
///******************************************/
//
//void SendToWifi(String tenmpF){
//
//  Serial.println("Sending data to thingsPeak");
//  String cmd = "AT+CIPSTART=\"TCP\",\"";
//  cmd += IP;
//  cmd += "\",80";
//  Serial.println(cmd);
//  Serial1.println(cmd);
//  delay(2000);
//  if(Serial1.find("ERROR")){
//    Serial.println("Échec de l'envoi");
//    //RestartESP8266();
//    lcd.setCursor(17,3);
//    lcd.print("404");
//    return;
//  }
//  Serial.println("Just sent " + cmd);
//  cmd = GET;
//  cmd += tenmpF;
//  cmd += "\r\n";
//  Serial1.print("AT+CIPSEND=");
//  Serial1.println(cmd.length());
//  if(Serial1.find(">")){
//    Serial1.println(cmd);
//    Serial.println("Just sent " + cmd);
//        lcd.setCursor(17,3);
//    lcd.print(" On");
//  }else{
//    Serial1.println("AT+CIPCLOSE");
//    Serial.println("RATÉ");
//        lcd.setCursor(0,3);
//    lcd.print("                    ");
//    //RestartESP8266();
//    lcd.setCursor(17,3);
//    lcd.print("Off");
//  }
//}
//
//void RestartESP8266()
//{
//  Serial.println("Restarting WiFi !");
//  lcd.setCursor(0,3);
//  lcd.print("ESP8266 Module init");
//  Serial1.println("AT+RST");
//  delay(2000);
//  lcd.clear();
//}

