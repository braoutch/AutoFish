//THIS IS A TEST

#include "pitches.h"
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h> // Inclusion de la librairie OneWire
#include <Wire.h> 

#define DS18B20 0x28     // Adresse 1-Wire du DS18B20
#define BROCHE_ONEWIRE 7 // Broche utilisée pour le bus 1-Wire

//wifi
String NomduReseauWifi = "Entrez le nom de votre Box ou point d'accès Wifi"; // Garder les guillements
String MotDePasse      = "Entrez le nom du mot de passe de votre Box ou point d'accès Wifi"; // Garder les guillements
String ApiKey          = "YCBS447TES9C1OTU";
#define IP "184.106.153.149" // thingspeak.com
String GET = "GET /update?key=YCBS447TES9C1OTU&field1=";
int lastSending = -1;

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
int relaiBulleur = 16 ;                      
int relaiPompe = 10;

int dayTime = 0;

////////////////////////////
////CARACTÉRISTIQUES////////
int morningTime = 11; //inclus
int eveningTime = 20;   //inclus
float targetTemp = 25;
float deltaTemp = 0.25f;
float deltaAlert = 0.75f;

int horaireReveil[] = {6,45};
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
int melody[] = {
  NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5
};
int reveil[] = {
  NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4,
  NOTE_C5, NOTE_B4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4,
  NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D4,
  NOTE_G4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_D5, NOTE_G4,
};
int noteDurations[] = {
  6, 6, 6, 3
};
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
void PlayMusic(int musicNumber){
  switch (musicNumber) {
    case 1 :
      for (int thisNote = 0; thisNote<4 ; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 2000 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);

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
  Serial.begin(9600);

  //wifi
  Serial1.begin(9600);  
  initESP8266();
  
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
  digitalWrite(relaiPompe,HIGH);

  analogWrite(redLedPin,255);
  analogWrite(blueLedPin,255);
  analogWrite(greenLedPin,255);
  
  //Display init
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(5, 1);  // (Colonne,ligne)
  Serial.print("Displaying introduction...");
  lcd.print("AUTOFISH");
  lcd.setCursor(4, 2);
  lcd.print("says Hello");
  PlayMusic(1);
  Serial.print("End of introduction...");

  //RTC init
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  DateTime initNow = RTC.now();
  thisMonth = initNow.day();
  //thisMonth = initNow.month();
  Serial.println((String)initNow.day()+"/" + (String)initNow.month()+"/" + (String)initNow.year()); 
  lcd.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////LOOP////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
  if (! RTC.isrunning()) 
    Serial.println("RTC is NOT running!");
    
///La date qui sert partout
DateTime now = RTC.now();
////////////////////////
//// ----Réveil---///////
////////////////////////

if(now.hour() == horaireReveil[0] && now.minute() == horaireReveil[1] && reveilDone == 0){
  PlayMusic(2);
  reveilDone = 1;
}
if(now.hour() == horaireReveil[0] && now.minute() == horaireReveil[1]+2)
reveilDone = 0;


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
    lastTemp = temp;
    Serial.print("Temperature : ");
    Serial.print(temp);
    Serial.write(176); // caractère °
    Serial.write('C');
    Serial.println();

    //SendToWifi

    if(now.minute() - lastSending >= 1){
    SendToWifi(String(lastTemp));
    lastSending = now.minute();
    }

    if(!alertTemp && temp != 0 && ((temp > targetTemp + deltaAlert) || (temp < targetTemp - deltaAlert)))
    {
      if(!mute)
      digitalWrite(buzzerPin, HIGH);
      analogWrite(redLedPin,100);
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
      Serial.print("ALERT IS OVER");
    }


    if(temp != 0 && ((temp < targetTemp - deltaTemp) || (temp > targetTemp + deltaTemp)))
    {
      Serial.println("BAD TEMPERATURE");
      BadTempCounter ++;
      if(!alertTemp)
      {
      analogWrite(blueLedPin, 100);
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
      analogWrite(greenLedPin, 100);
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

if(alertTemp)
lcd.print("Alerte ! ");

else
{
  if(dayTime == 1 && !ForceMode){
    lcd.print("Daytime, night at ");
    lcd.print(eveningTime);
    lcd.print("");
  }

  else {
    lcd.print("Night,   day at ");
    lcd.print(morningTime);
    lcd.print("   ");
  }
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
  Serial.println("**********************************************************");  
  Serial.println("**************** DEBUT DE L'INITIALISATION ***************");
  Serial.println("**********************************************************");  
  envoieAuESP8266("AT+RST");
  recoitDuESP8266(2000);
  Serial.println("**********************************************************");
  envoieAuESP8266("AT+CWMODE=3");
  recoitDuESP8266(5000);
  Serial.println("**********************************************************");
  envoieAuESP8266("AT+CWJAP=\""+ NomduReseauWifi + "\",\"" + MotDePasse +"\"");
  recoitDuESP8266(10000);
  Serial.println("**********************************************************");
  //envoieAuESP8266("AT+CIFSR");
  //recoitDuESP8266(1000);
  Serial.println("**********************************************************");
  //envoieAuESP8266("AT+CIPMUX=1");   
  //recoitDuESP8266(1000);
  Serial.println("**********************************************************");
  //envoieAuESP8266("AT+CIPSERVER=1,80");
  //recoitDuESP8266(1000);
  Serial.println("**********************************************************");
  Serial.println("***************** INITIALISATION TERMINEE ****************");
  Serial.println("**********************************************************");
  Serial.println("");  
}

/****************************************************************/
/*        Fonction qui envoie une commande à l'ESP8266          */
/****************************************************************/
void envoieAuESP8266(String commande)
{  
  Serial1.println(commande);
}
/****************************************************************/
/*Fonction qui lit et affiche les messages envoyés par l'ESP8266*/
/****************************************************************/
void recoitDuESP8266(const int timeout)
{
  String reponse = "";
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(Serial1.available())
    {
      char c = Serial1.read();
      reponse+=c;
    }
  }
  Serial.print(reponse);   
}

/******************************************/
/*ENVOYER A THINGSPEAK*********************/
/******************************************/
void SendToWifi(String tenmpF){
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  Serial.println(cmd);
  delay(2000);
  if(Serial.find("Error")){
    return;
  }
  cmd = GET;
  cmd += tenmpF;
  cmd += "\r\n";
  Serial.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  if(Serial.find(">")){
    Serial.print(cmd);
  }else{
    Serial.println("AT+CIPCLOSE");
  }
}



