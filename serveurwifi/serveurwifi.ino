//---------- Bilbiothèque ----------//
#include <Arduino.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
//---------------------------------//


//--------- Fichier local ---------//


//---------------------------------//

//------------ Define ------------//
#define DNS_Name "ESP32_informations"
#define EEPROM_Size 4096
#define size_datas 35
#define sizeTabDatas 4
//--------------------------------//

//----------- Variable -----------//
const char* ssid     = "ESP32_C3_Wifi";
const char* password = "123456789";

char SSID[size_datas] ;
char PASSWORD[size_datas];
char MQTT_USER[size_datas];
char MQTT_PSW[size_datas];

uint8_t Func;
const int port = 732;
char* Datas[sizeTabDatas];
//-------------------------------//

//------------ Class ------------//
WiFiUDP udp;
//-------------------------------//



void setup()
{
  //-----Initialisation EEPROM-----//
  EEPROM.begin(EEPROM_Size);
  LectureEEPROM();
  //------------------------------//

  //-----Initialisation WIFI------//
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPIP();
  //------------------------------//

  //-----Initialisation DNS-------//
  MDNS.begin(DNS_Name);
  //------------------------------//

  //-----Initialisation UDP-------//
  udp.begin(port);
  //------------------------------//

}


void loop() {

  //----------- Variable -----------//
  bool good = false;//variable message reçue

  uint16_t packetSize = udp.parsePacket();//Donnée présente dans le buffer
  char incomingPacket[255];//Variable message reçue
  //-------------------------------//

  //------ Réception Message ------//
  //Buffer non vide
  if (packetSize) {
    //Lecture message recue
    int len = udp.read(incomingPacket, 255);

    //Le message contient t'il quelque chose
    if (len > 0) {
      //Fin du message
      incomingPacket[len] = '\0';
      //Message reçue oui
      good = true;
      //Ouverture canal pour réponse
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
    }
  }
  //-------------------------------//


  //Si pas de message reçue on recommence la lecture
  if(!good){
    return;
  }

  //----------- Traitement -----------//
  Func = atoi(incomingPacket);//Conversion char en fonction reçue

  //Fonction spécial d'écriture
  if (Func == 0) {
    //Fin première lecture
    udp.endPacket();

    //----------- Variable -----------//
    int8_t count = 0;
    int64_t timeout = millis() + 5000;//réglage timeout
    //-------------------------------//

    good = false;

    //---------- Réception Message ----------//
    //début seconde lecture avec les données
    //Timeout pour évité de resté bloquer
    while (timeout > millis() && !good) {

      //Nombes de données présente dans le buffer
      packetSize = udp.parsePacket();

      //Buffer non vide
      if (packetSize) {
        //Lecture message recue
        int len = udp.read(incomingPacket, 255);

        //Le message contient t'il quelque chose
        if (len > 0) {
          //Fin du message
          incomingPacket[len] = '\0';
          //Split des datas
          splitString(incomingPacket, ';', Datas, &count, sizeTabDatas);
          good = true;
          //Ecriture des nouvelles données dans la mémoire
          EcritureAll();
          //Chargement de la mémoire dans les variables à renvoyer
          LectureEEPROM();
          //Libérations mémoire pour évité le blocage RAM
          freeTokens(Datas, sizeTabDatas);
        }
      }
    }
    //---------------------------------------//
  }

  //Traitement code fonction différent de 0
  TraitementCode();
  
  //Fin du message
  udp.endPacket();

  //-------------------------------//

}

//Code traitement renvoie données relatif au numéro envoyer hormis zéro
// 1 : Nom livebox
// 2 : Mot de passe Wifi
// 3 : Nom utilisateur MQTT
// 4 : Mot de passe MQTT
// X : Unknow pour un code non dans la liste
void TraitementCode(){

  // Nom Livebox
  if(Func == 1){
    udp.printf(SSID);
  }
  // Mot de passe Wifi
  else if(Func == 2){
    udp.printf(PASSWORD);
  }
  // Nom utilisateur MQTT
  else if(Func == 3){
    udp.printf(MQTT_USER);
  }
  // Mot de passe MQTT
  else if(Func == 4){
    udp.printf(MQTT_PSW);
  }
  // autre numéro
  else {
    udp.printf("Unknow");
  }
}

// Lecture de la mémoire EEPROM vers les variables associé
// Décallage automatique avec la tailles des données
// 0 : Nom livebox
// 1 : Mot de passe Wifi
// 2 : Nom utilisateur MQTT
// 3 : Mot de passe MQTT
void LectureEEPROM(){
  EEPROM.get(0*size_datas, SSID);
  EEPROM.get(1*size_datas, PASSWORD);
  EEPROM.get(2*size_datas, MQTT_USER);
  EEPROM.get(3*size_datas, MQTT_PSW);
}

// Ecriture des données en fonction des valeurs dans 
// Le tableau Datas
// Décallage automatique avec la tailles des données
// 0 : Nom livebox
// 1 : Mot de passe Wifi
// 2 : Nom utilisateur MQTT
// 3 : Mot de passe MQTT
void EcritureAll(){

  EcritureEEPROM(0, Datas[0]);
  EcritureEEPROM(1, Datas[1]);
  EcritureEEPROM(2, Datas[2]);
  EcritureEEPROM(3, Datas[3]);
  //Mise à jour des données comme sur Git
  EEPROM.commit();

}

// Ecriture de la donnée dans l'EEPROM
// Attention à l'adresse pour évité une réécriture
// Value n'as pas de taille fini
//Aucune réécriture sur la valeur est la même
void EcritureEEPROM(int8_t adress, char value[]){

  // Variable d'un tableau fixe pour évité une taille indéterminé de value
  char newValue[size_datas];
  strcpy(newValue, value);
  // Variable ancienne valeur
  char oldValue[size_datas];

  // Récuperation ancienne valeur
  EEPROM.get(adress*size_datas, oldValue);
  // Vérification différence entre ancienne et nouvelle valeur
  if (strcmp(oldValue, value) != 0) {
    // Ecriture nouvelle valeur
    EEPROM.put(adress*size_datas, newValue);
  }

}

//----------- Fonction à ne pas toucher !-----------------//


//Fonction Split changement configuration (Ne Jamais toucher !)
void splitString(char* input, char delimiter, char* output[], int8_t* count, int8_t maxTokens) {
  char delimiterStr[2] = {delimiter, '\0'}; // Convertir le délimiteur en une chaîne de caractères
  char* token = strtok(input, delimiterStr); // Utiliser la chaîne de caractères comme délimiteur
  *count = 0;

  while (token != NULL && *count < maxTokens) {
    output[*count] = (char*)malloc(strlen(token) + 1); // Alloue de la mémoire pour le token
    strcpy(output[*count], token);
    (*count)++;
    token = strtok(NULL, delimiterStr);
  }
}


//Libération mémoire (Ne Jamais toucher !)
void freeTokens(char* output[], int maxTokens) {
  for (int i = 0; i < maxTokens; ++i) {
    if (output[i] != NULL) {
      free(output[i]);
      output[i] = NULL;
    }
  }
}

//---------------------------------------//