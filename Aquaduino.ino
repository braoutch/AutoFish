#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h> // Inclusion de la librairie OneWire
#include <Wire.h> 

#define DS18B20 0x28     // Adresse 1-Wire du DS18B20
#define BROCHE_ONEWIRE 7 // Broche utilisée pour le bus 1-Wire


//Pins
int buzzerPin = 4;
int ledPin = 8;
int forceModePin = 9;
int forceModePinState = 0;
int foodModePin = 6;
int foodModePinState = 0;
boolean feeding = false;     //Pour savoir si la bouffe est en cours de distribution
long feedingInitTime = 0;

//relais
int relaiLumiere = 15;      //La lumière                   
int relaiChauffage = 14;                       
int relaiBulleur = 16 ;                      
int relaiPompe = 10;

int dayTime = 0;

////////////////////////////
////CARACTÉRISTIQUES////////
int morningTime = 9 ;
int eveningTime = 19;
float targetTemp = 25;
float deltaTemp = 0.25f;
float deltaAlert = 0.75f;
///////////////////////////
///////////////////////////

//compteurs
float GoodTempCounter = 0;
float BadTempCounter = 0;
int thisMonth;


boolean ForceMode = false;
int forceModePushed = 0;
int heatMode = 0;
int mute = 1;

//alertes
boolean alertTemp = false;
boolean alertPH = false;

//Écran
//LiquidCrystal_I2C lcd(0x20, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Addr, En, Rw, Rs, d4, d5, d6, d7, backlighpin, polarity
LiquidCrystal_I2C lcd(0x3F,20,4); 
//LiquidCrystal_I2C lcd(0x37,20,4); 

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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////SETUP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  // make the pushbutton's pin an input:
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(forceModePin, INPUT_PULLUP);
  pinMode(foodModePin, INPUT_PULLUP);
  pinMode(relaiLumiere,OUTPUT);
  pinMode(relaiChauffage,OUTPUT);
  pinMode(relaiBulleur,OUTPUT);
  pinMode(relaiPompe,OUTPUT);
  
  digitalWrite(buzzerPin,LOW);
  digitalWrite(relaiPompe,HIGH);

  //Display init
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0);  // (Colonne,ligne)
  lcd.print("Démarrage du module Aquaduino...");
  delay(500);

  //RTC init
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  DateTime initNow = RTC.now();
  thisMonth = initNow.day();
  //thisMonth = initNow.month();
  Serial.println((String)initNow.day()+"/" + (String)initNow.month()+"/" + (String)initNow.year()); 
  
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////LOOP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
///La date qui sert partout
DateTime now = RTC.now();
Serial.println((String)now.day()+"/" + (String)now.month()+"/" + (String)now.year()); 
///////////////////////
//// ----FOOD---///////
///////////////////////
foodModePinState = digitalRead(foodModePin);

if(!feeding && foodModePinState == LOW)
{
  feeding = true;
  feedingInitTime = now.minute();
  digitalWrite(relaiPompe,LOW);
}

if(feeding)
{
  if(feedingInitTime < 55)
  { 
   if(now.minute() >= feedingInitTime+5)
   {
    feeding = false;
    digitalWrite(relaiPompe,HIGH);
  }
}

if(feedingInitTime >=55)
{ 
 if(now.minute()+60 >= feedingInitTime+5)
 {
  feeding = false;
  digitalWrite(relaiPompe,HIGH);
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
  // Lit la température ambiante à ~1Hz
  if(getTemperature(&temp)) {
    // Affiche la température
    Serial.print("Temperature : ");
    Serial.print(temp);
    Serial.write(176); // caractère °
    Serial.write('C');
    Serial.println();
    

    if(!alertTemp && temp != 0 && ((temp > targetTemp + deltaAlert) || (temp < targetTemp - deltaAlert)))
    {
      if(!mute)
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(ledPin,HIGH);
      alertTemp = true;
      Serial.print("ALERT ");
      Serial.println(temp);
    }
    if (alertTemp && temp != 0 && temp < (targetTemp + deltaAlert) && temp > (targetTemp - deltaAlert))
    {
      digitalWrite(buzzerPin, LOW);
      digitalWrite(ledPin,LOW);
      alertTemp = false;
      Serial.print("ALERT IS OVER");
    }


    if(temp != 0 && ((temp < targetTemp - deltaTemp) || (temp > targetTemp + deltaTemp)))
    {
      Serial.println("BAD TEMPERATURE");
      BadTempCounter ++;
    }

    else
    {
      Serial.println("GOOD TEMPERATURE");
      GoodTempCounter++;
    }

    
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
    Serial.println(forceModePinState);
    
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
      Serial.print("Disable Forced Mode");
    }
    ///////////////////////

    if(!ForceMode)
    {
      int hour = now.hour();
      hour = 11;
      if((hour >= morningTime || hour <= eveningTime) && !dayTime)
      {
        dayTime = 1;
        ChangeLight(dayTime);
      }
      if((hour < morningTime || hour > eveningTime) && dayTime)
      {
        dayTime = 0;
        ChangeLight(dayTime);
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
 lcd.print("Eau à ");
 lcd.print(temp);
 lcd.print("°C  HEAT ");
 if(heatMode == 1)
 lcd.print("ON");
 if(heatMode == 0)
 lcd.print("OFF");

////////////////////
///----LIGNE 3----//
lcd.setCursor(0, 2);  // (Colonne,ligne)
if(alertTemp)
lcd.print("MAUVAISE TEMPÉRATURE");
else if(alertPH)
lcd.print("MAUVAIS PH");
else
{
  lcd.print("All Good");
  
  if(dayTime == 1){
    lcd.print(" Night at ");
    lcd.print(eveningTime);
  }

  if(dayTime == 0){
    lcd.print(", Day at ");
    lcd.print(morningTime);
  }
}

//////////////////// 
///----LIGNE 4----//
lcd.setCursor(0, 3);  // (Colonne,ligne)
if(!ForceMode)  
lcd.print("MODE AUTO   ");
if(ForceMode)  
lcd.print("MODE FORCÉ  ");

lcd.print((GoodTempCounter/(GoodTempCounter + BadTempCounter)),1);
lcd.print("%");

}





