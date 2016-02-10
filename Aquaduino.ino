//THIS IS A TEST

#include "pitches.h"
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h> // Inclusion de la librairie OneWire
#include <Wire.h> 

#define DS18B20 0x28     // Adresse 1-Wire du DS18B20
#define BROCHE_ONEWIRE 7 // Broche utilisée pour le bus 1-Wire

//wifi
char serialbuffer[150];//serial buffer for request url
String NomduReseauWifi = "wayne"; // Garder les guillements
String MotDePasse      = "antoinee"; // Garder les guillements
String ApiKey          = "YCBS447TES9C1OTU";
#define IP "184.106.153.149" // thingspeak.com
String GET = "GET /update?key=YCBS447TES9C1OTU&field1=";
int lastSending = -10;
int sendFrequency = 1;  //en minutes

//Pins
int buzzerPin = 4;

int blueLedPin = 18;
int greenLedPin = 19;
int redLedPin = 20;

int forceModePin = 9;
int forceModePinState = 0;
int foodModePin = 6;
int foodModePinState = 0;
boolean feeding = false;     //Pour savoir si la bouffe est en cours de distribution
long feedingInitTime = 0;

//relais
int relaiLumiere = 15;      //La lumière                   
int relaiChauffage = 14;  
int relaiPompe = 16;                     
int relaiBulleur = 10 ;                      


int dayTime = 0;

////////////////////////////
////CARACTÉRISTIQUES////////
int morningTime = 11; //inclus
int eveningTime = 20;   //inclus
float targetTemp = 25;
float deltaTemp = 0.25f;
float deltaAlert = 0.75f;

int horaireReveil[] = {06,45};

int ledIntensite = 100;  //Valeur de 0 à 255 (0 = fort, 255 = éteint) 
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

//Réveil et sons
int melody[][4] = {{
	NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5
},{
  NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4
}};

int reveil[] = {
	NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4,
	NOTE_C5, NOTE_B4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4,
	NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4,
	NOTE_G4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_D5, NOTE_G4,
};
int noteDurations[][4] = {{
	6, 6, 6, 3
},{
  6, 6, 6, 3
}};
int reveilDurations[] = {
	12,12,12,12,6,6,12,12,12,12,6,6,
	12,12,12,12,12,12,6,12,12,12,12,12,12,6,
	12,12,12,12,6,6,12,12,12,12,6,6,
	12,12,12,12,6,12,12,12,12,12,12,6,6, 1.5
};

int reveilDone = 0;

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
  delay(800);             // Et on attend la fin de la mesure

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
		digitalWrite(relaiLumiere, HIGH);
		digitalWrite(relaiBulleur, HIGH);
	}

	if(!lightOn){
		digitalWrite(relaiLumiere, LOW);
		digitalWrite(relaiBulleur, LOW);
	}
}

////////////////////
/////////MUSIC//////
////////////////////
void PlayMusic(int musicNumber, int index){
	switch (musicNumber) {
		case 1 :
		for (int thisNote = 0; thisNote<4 ; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 2000 / noteDurations[index][thisNote];
    tone(buzzerPin, melody[index][thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(buzzerPin);
}
break;
case 2:
for (int thisNote = 0; thisNote < 52; thisNote++) {

    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int reveilDuration = 2000 / reveilDurations[thisNote];
    tone(buzzerPin, reveil[thisNote], reveilDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = reveilDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(buzzerPin);
}
break;
}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////SETUP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  Serial.begin(115200);
  Serial1.begin(115200);
  
  //Display init
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(5, 0);  // (Colonne,ligne)
  Serial.println("Displaying introduction...");
  lcd.print("AUTOFISH");
  lcd.setCursor(4, 1);
  lcd.print("says Hello");
  //PlayMusic(1);
  Serial.println("End of introduction...");
  
  //wifi
  //initESP8266();
  
  pinMode(buzzerPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(forceModePin, INPUT_PULLUP);
  pinMode(foodModePin, INPUT_PULLUP);
  pinMode(relaiLumiere,OUTPUT);
  pinMode(relaiChauffage,OUTPUT);
  pinMode(relaiBulleur,OUTPUT);
  pinMode(relaiPompe,OUTPUT);
  
  digitalWrite(buzzerPin,LOW);
  digitalWrite(relaiPompe,LOW);

  analogWrite(redLedPin,255);
  analogWrite(blueLedPin,255);
  analogWrite(greenLedPin,255);

  //RTC init

  lcd.setCursor(0,3);
  lcd.print(" RTC Module init    ");
  
  RTC.begin();
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  DateTime initNow = RTC.now();
  thisMonth = initNow.day();
  //thisMonth = initNow.month();
  //Serial.println((String)initNow.day()+"/" + (String)initNow.month()+"/" + (String)initNow.year()); 
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////LOOP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
//	if (! RTC.isrunning()) 
//	Serial.println("RTC is NOT running!");

////////////////////////////
//////WIFI HAS SOMETHING TO SAY?
////////////////////////////

//output everything from ESP8266 to the Arduino Micro Serial output
  while (Serial1.available() > 0) {
    Serial.write(Serial1.read());
  }
  
  if (Serial.available() > 0) {
     //read from serial until terminating character
     int len = Serial.readBytesUntil('\n', serialbuffer, sizeof(serialbuffer));
  
     //trim buffer to length of the actual message
     String message = String(serialbuffer).substring(0,len-1);
     Serial.println("message: " + message);

  }//output everything from ESP8266 to the Arduino Micro Serial output
  while (Serial1.available() > 0) {
    Serial.write(Serial1.read());
  }


///La date qui sert partout
DateTime now = RTC.now();

//SendToWifi
int minutes = now.minute();
//Serial.println("Do i send to wifi ? last time was at " + (String)lastSending + ", this is " + (String)minutes);
    if((minutes - lastSending) >= sendFrequency){
      Serial.println("Sending to Wifi");
    SendToWifi(String(lastTemp));
    lastSending = minutes;
    if (lastSending == 59)
    lastSending = -1;
    
    }
////////////////////////
//// ----Réveil---///////
////////////////////////

if(now.hour() == horaireReveil[0] && now.minute() == horaireReveil[1] && reveilDone == 0){
	PlayMusic(2,0);
	reveilDone = 1;
}
if(now.hour() == horaireReveil[0] && now.minute() == horaireReveil[1]+2)
reveilDone = 0;


//Serial.println((String)now.day()+"/" + (String)now.month()+"/" + (String)now.year()); 
///////////////////////
//// ----FOOD---///////
///////////////////////
foodModePinState = digitalRead(foodModePin);

if(!feeding && foodModePinState == LOW)
{
	feeding = true;
	feedingInitTime = now.minute();
	digitalWrite(relaiPompe,HIGH);
 Serial.println("Time for feeding !");
}

if(feeding)
{
	if(feedingInitTime < 55)
	{ 
		if(now.minute() >= feedingInitTime+5)
		{
			feeding = false;
			digitalWrite(relaiPompe,LOW);
		}
	}

	if(feedingInitTime >=55)
	{ 
		if(now.minute()+60 >= feedingInitTime+5)
		{
			feeding = false;
			digitalWrite(relaiPompe,LOW);
		}
	}

}


/////////////////////////////
/////////RESET Mean date %////////
/////////EVERY MONTH////////
/////////////////////////////

if(now.day() == thisMonth + 1 || now.day() - thisMonth > 9)
//if(now.month() == thisMonth + 1)
{
	thisMonth = now.day() ;
	GoodTempCounter = 0;
	BadTempCounter = 0;
}


///////////////////////
//// TEMPERATURE///////
///////////////////////
float temp;

if(getTemperature(&temp)) {	    // Affiche la température  // Lit la température ambiante à ~1Hz
	
	lastTemp = temp;
	Serial.print("Temperature : ");
	Serial.print(temp);
  Serial.println(" degrés");

    if(!alertTemp && temp != 0 && ((temp > targetTemp + deltaAlert) || (temp < targetTemp - deltaAlert)))
    {
    	//if(!mute)
    	//digitalWrite(buzzerPin, HIGH);
    	analogWrite(redLedPin, ledIntensite);
    	analogWrite(blueLedPin, 255);
    	alertTemp = true;
    	Serial.print("ALERT ");
    	Serial.println(temp);
    }
    if (alertTemp && temp != 0 && temp < (targetTemp + deltaAlert) && temp > (targetTemp - deltaAlert))
    {
    	digitalWrite(buzzerPin, LOW);
    	analogWrite(redLedPin,255);
    	alertTemp = false;
    	Serial.println("ALERT IS OVER");
    }


    if(temp != 0 && ((temp < targetTemp - deltaTemp) || (temp > targetTemp + deltaTemp)))
    {
    	//Serial.println("BAD TEMPERATURE");
    	BadTempCounter ++;
    	if(!alertTemp)
    	{
    		analogWrite(blueLedPin, ledIntensite);
    		analogWrite(greenLedPin, 255);
    		analogWrite(redLedPin,255);
    	}
    	if(alertTemp)
    	BadTempCounter++;
    }

    else
    {
    	Serial.println("GOOD TEMPERATURE");
    	GoodTempCounter++;
    	analogWrite(blueLedPin, 255);
    	analogWrite(greenLedPin, ledIntensite);
    	analogWrite(redLedPin,255);
    }



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
}

///////////////////////
///////// LIGHT///////
//////////////////////

//////FORCEMODE//////
forceModePinState = digitalRead(forceModePin);
//Serial.println(forceModePinState);

if(forceModePinState == LOW && !ForceMode)
{
	ForceMode = true;
	Serial.println("Enable Forced Mode");
	if(!dayTime)
	ChangeLight(dayTime);      
}

if(forceModePinState == HIGH && ForceMode)
{
	ForceMode = false;
	Serial.println("Disable Forced Mode");
}
///////////////////////

if(!ForceMode)
{
	int hour = now.hour();
	if((hour >= morningTime && hour < eveningTime) && !dayTime)
	{
		dayTime = 1;
		ChangeLight(dayTime);
    PlayMusic(1,0);
	}
	if((hour < morningTime || hour >= eveningTime) && dayTime)
	{
		dayTime = 0;
		ChangeLight(dayTime);
    PlayMusic(1,1);
	} 
}

else if(ForceMode)
if(!dayTime)
{
	ChangeLight(true); 
	dayTime = 1;
}

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
	lcd.print("night at ");
	lcd.print(eveningTime);
	lcd.print("");
}

else {
	lcd.print("day at ");
	lcd.print(morningTime);
	lcd.print("h ");
}


//////////////////// 
///----LIGNE 4----//
lcd.setCursor(0, 3);  // (Colonne,ligne)
if(!ForceMode)  
lcd.print("MODE AUTO   ");
if(ForceMode)  
lcd.print("MODE FORCE  ");

lcd.print((GoodTempCounter/(GoodTempCounter + BadTempCounter)),1);
lcd.print("%");

}

/****************************************************************/
/*                Fonction qui initialise l'ESP8266             */
/****************************************************************/
void initESP8266()
{  
  Serial.println("Restarting WiFi !");
  lcd.setCursor(0,3);
  lcd.print("ESP8266 Module init");
  Serial1.println("AT+RST");
  delay(500);
  Serial1.println("AT+CWMODE=1");
  lcd.setCursor(0,3);
  lcd.print("Looking for WiFi...");
  delay(500);
  
  //Serial1.println("AT+RST");
  //connect to wifi network
  Serial1.println("AT+CWJAP=\""+ NomduReseauWifi + "\",\"" + MotDePasse +"\"");
  delay(2000);

  lcd.clear();
}

/******************************************/
/*ENVOYER A THINGSPEAK*********************/
/******************************************/

void SendToWifi(String tenmpF){

  Serial.println("Sending data to thingsPeak");
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  Serial.println(cmd);
  Serial1.println(cmd);
  delay(2000);
  if(Serial1.find("ERROR")){
    Serial.println("Échec de l'envoi");
    initESP8266();
    return;
  }
  Serial.println("Just sent " + cmd);
  cmd = GET;
  cmd += tenmpF;
  cmd += "\r\n";
  Serial1.print("AT+CIPSEND=");
  Serial1.println(cmd.length());
  if(Serial1.find(">")){
    Serial1.println(cmd);
    Serial.println("Just sent " + cmd);
  }else{
    Serial1.println("AT+CIPCLOSE");
    Serial.println("RATÉ");
    initESP8266();
  }
}




