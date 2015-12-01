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

//relais
int relaiLumiere = 15;      //La lumière                   
int relaiChauffage = 14;                       
int relaiBulleur = 16 ;                      
int RELAY4 = 10;

int dayTime = 0;
int morningTime = 9 ;
int eveningTime = 19;
float targetTemp = 25;
boolean ForceMode = false;
int forceModePushed = 0;


//LiquidCrystal_I2C lcd(0x20, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Addr, En, Rw, Rs, d4, d5, d6, d7, backlighpin, polarity
LiquidCrystal_I2C lcd(0x3F,20,4); 
//LiquidCrystal_I2C lcd(0x37,20,4); 

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
  pinMode(relaiLumiere,OUTPUT);
  pinMode(relaiChauffage,OUTPUT);
  pinMode(relaiBulleur,OUTPUT);
  
  digitalWrite(ledPin,HIGH);

  //Display init
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0);  // (Colonne,ligne)
  lcd.print("Démarrage du module Aquaduino...");
  delay(500);

  //RTC init
  RTC.begin();
}


void loop() 
{

DateTime now = RTC.now();

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

    if(temp != 0 && (temp > targetTemp + 0.75f) || (temp < targetTemp - 0.75f))
    {
      digitalWrite(buzzerPin, HIGH);
    }
    else
    digitalWrite(buzzerPin, LOW);

  
    if(temp != 0 && temp > targetTemp + 0.25f)
    {
      
      Serial.println("TEMPERATURE TOO HIGH");
      digitalWrite(relaiChauffage,LOW);
    }

    if(temp != 0 && temp < targetTemp - 0.25f)
    {
      digitalWrite(buzzerPin, LOW);
      Serial.println("TEMPERATURE GOOD");
      digitalWrite(relaiChauffage,HIGH);
    }
  }
    
///////////////////////
///////// LIGHT///////
//////////////////////

    //////FORCEMODE//////
    forceModePinState = digitalRead(forceModePin);
    Serial.println(forceModePinState);
    
    if(forceModePinState == LOW && ForceMode && forceModePushed == 0)
    {
      forceModePushed = 1;
      ForceMode = false;
      Serial.println("Enable Forced Mode");
      if(!dayTime)
      ChangeLight(dayTime);      
    }
    
    if(forceModePinState == LOW && !ForceMode && forceModePushed == 0)
    {
      forceModePushed = 1;
      ForceMode = true;
      Serial.print("Disable Forced Mode");
    }
    if(forceModePinState == HIGH)
    forceModePushed = 0;
      
    
    
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
    }
     
    

    

///////////////////////
/////////DISPLAY///////
///////////////////////

  
}





