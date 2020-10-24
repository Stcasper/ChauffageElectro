/*
  
*/
#include <ESP8266WiFi.h>
#include <DHT.h>  

#define DHTPIN D7             // Pin sur lequel est branché le DHT
#define DHTTYPE DHT22         // DHT 22  (AM2302)

// valeurs pour le WiFi
//const char* ssid     = "rezomaizon";
//const char* password = "jepensequeprendresonpetitdejeunerlematinestmeilleur";
const char* ssid     = "Livebox-294C";
const char* password = "2DD96EE3FEAA2DCFDAD1DE7D2A";

// valeurs pour le serveur Web
const char* host     = "api.openweathermap.org";
const char* apikey   = "f9203a3faba4eba9b6f486e5ca3e86fe"; // il est possible d'utiliser la clé d'API suivante : 1a702a15a2f46e405e61804cf67c0d30
const char* town     = "Blere,fr";
//const char* town     = "vélizy-villacoublay,fr";
const int httpPort = 80;
const char* serverPerso = "stcasper.free.fr";
const int radiateur = 4 ;

/* Code d'erreur du capteur de température */
const byte DHT_SUCCESS = 0;        // Pas d'erreur
const byte DHT_TIMEOUT_ERROR = 1;  // Temps d'attente dépassé
const byte DHT_CHECKSUM_ERROR = 2; // Données reçues erronées

//Délais
const int timeoutMesure = 1;   // Interval entre deux mesures en minutes
const int timeoutWiFi = 10000;  // Timeout si le wifi ne répond pas au moment du démarrage en ms
const int timeoutReset = 12;    // Reboot régulier en heures 

// Variables
float temperature;          // Température extérieur
float temp_int;             // Température intérieure
float humidity;             // Humidité
float humidity_int;         // Humidité intérieure
float consigne=7;           // Température de consigne
float consigneSauve=7;      // Température de consigne de sauvegarde
float reboot=0;             // Consigne de reboot
int allume=0;               // Mise en route du chauffage (1 ou 0)
DHT dht(DHTPIN, DHTTYPE);   // Capteur de température

int modeChauf = 0;          // Mode de chauffage. 0 = Confort. 1 = Eco. 9 = Sauvegarde.

boolean debug = false;       // Mode debug
int temps = 0;
int tempsSauve = 0;
String url;
String data;

int led = D5;
int blink;
int chauffage = D2;

int boucle = 0;

int status = WL_IDLE_STATUS;


void setup() {
    if (debug) {
        Serial.begin(9600);
    }
    // Initialisation des entrées/sorties
    pinMode(led, OUTPUT);
    pinMode(chauffage, OUTPUT);
    pinMode(DHTPIN, INPUT_PULLUP);
  
    // Extinction du chauffage à l'init.
    digitalWrite(chauffage, LOW);
  
    // Allumage la led de fonctionnement
    digitalWrite(led, HIGH);
  
    // Initialisation de la sonde de température
    dht.begin();

    // On ajoute un délai de démarage proportionel au numéro du radiateur pour éviter les soucis au redémarage après coupure de courant.
    delay(radiateur*5000);
    boucle = 0;
}


void loop() {
    // Si le wifi n'est pas connecté on passe en mode sauvegarde
    if (WiFi.status() != WL_CONNECTED){
        if (debug) {
            Serial.print("WiFi non connecté");
        }
        modeChauf=9;
    }
    
    if (modeChauf==0) {

        if (boucle==9){
          boucle = 0;
        }
        WiFiClient client;

        if (boucle==0) {
            
          /* Début de la mesure extérieure */
          url = String("/data/2.5/weather?q=") + town + "&appid=" + apikey;
     
          if (debug){
              Serial.print("Connexion au serveur : ");
              Serial.println(host);
          }
  
          client.connect(host, httpPort);
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                      "Host: " + host + "\r\n" +
                      "Connection: close\r\n\r\n");
          delay(1000);
          lectureValeurs (client, String("\"temp\":"),String("\"humidity\":"));
          /* Fin de la lecture de la température extérieure */
  
          /*Lecture de la consigne de reboot */
          url = String("/temperature/temp_rest_re7.php?rquest=read_reboot&Radiateur=")+String(radiateur);
     
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
          }
          client.connect(serverPerso, httpPort);
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                      "Host: " + serverPerso + "\r\n" +
                      "Connection: close\r\n\r\n");
          delay(1000);
          reboot = lectureConsigne (client, String("\"Reboot\":\""));
          
          if (debug) {
              Serial.print("Reboot ? ");
              Serial.println(reboot);
            }
          if (reboot){
            // Remise à 0 du flag reboot
            url = String("/temperature/temp_rest_re7.php?rquest=write_reboot");
            data = "Radiateur=" + (String)radiateur;
            client.connect(serverPerso, httpPort);
         
            if (debug) {
              Serial.print("demande URL: ");
              Serial.println(url);
            }
            client.println(String("POST ") + url + " HTTP/1.1");          
            client.println("Host: " + (String)serverPerso);
            client.println("Content-Type: application/x-www-form-urlencoded");
            client.println("Connection: close");
            client.println("User-Agent: Arduino/1.0");
            client.print("Content-Length: ");
            client.println(data.length());
            client.println();
            client.print(data);
            client.println();
  
            //On attend un peu
            delay(3000);
            //Reboot de la carte
            ESP.restart();
          }
          /*Fin de l'éventuel reboot */
    
          /*lecture de la température de consigne */
          url = String("/temperature/temp_rest_re7.php?rquest=read_consigne&Radiateur=")+String(radiateur);
     
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
          }
          client.connect(serverPerso, httpPort);
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                      "Host: " + serverPerso + "\r\n" +
                      "Connection: close\r\n\r\n");
          delay(1000);
          consigne = lectureConsigne (client, String("\"Temperature\":\""));
          /*Fin de la lecture de la température de consigne */
  
          /* Mesure de la température de la pièce */
          mesureTemp();
          
          // Gestion des erreurs de lecture de température
          if (isnan(temp_int)) { //Si la lecture de température intérieure a foiré, on tente une nouvelle lecture
            delay(2000);
            mesureTemp();
  
            if (isnan(temp_int)) { //Si la lecture de température intérieure a encore foiré on fixe une valeur basse
              //temp_int=2;
              //modeChauf=8; //La sonde à foiré deux fois de suite, on passe en mode de sécurité.
            }
          }
  
          /* Prise de décision chauffage*/  
          if (temp_int<consigne) {
            allume = 1;
            digitalWrite(chauffage, HIGH);
          }
          else {
            allume = 0;
            digitalWrite(chauffage, LOW);
          }
          /* Fin prise de décision chauffage*/
  
          /* Sauvegarde des logs */
          url = String("/temperature/temp_rest_re7.php?rquest=insert_temp");
          data = "temp_ext=" + (String)temperature +"&temp=" + (String)temp_int + "&humi=" + (String)humidity_int + "&Consigne=" + (String)consigne + "&Radiateur=" + (String)radiateur + "&Allume=" + (String)allume;
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
            Serial.println(url);
          }
          
          client.connect(serverPerso, httpPort);
         
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.println(String("POST ") + url + " HTTP/1.1");          
          client.println("Host: " + (String)serverPerso);
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.println("Connection: close");
          client.println("User-Agent: Arduino/1.0");
          client.print("Content-Length: ");
          client.println(data.length());
          client.println();
          client.print(data);
          client.println();
          /* Fin sauvegarde des logs */

          /* Sauvegarde des logs minute*/
          url = String("/temperature/temp_rest_re7.php?rquest=insert_temp_minute");
          data = "rad=" + (String)radiateur + "&temp_ext=" + (String)temperature +"&temp=" + (String)temp_int + "&Consigne=" + (String)consigne + "&Allume=" + (String)allume;
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
            Serial.println(url);
            Serial.println(data);
          }
          
          client.connect(serverPerso, httpPort);
         
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.println(String("POST ") + url + " HTTP/1.1");          
          client.println("Host: " + (String)serverPerso);
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.println("Connection: close");
          client.println("User-Agent: Arduino/1.0");
          client.print("Content-Length: ");
          client.println(data.length());
          client.println();
          client.print(data);
          client.println();
          /* Fin sauvegarde des logs minute*/
        }
        else {
            
          /*Lecture de la consigne de reboot */
          url = String("/temperature/temp_rest_re7.php?rquest=read_reboot&Radiateur=")+String(radiateur);
     
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
          }
          client.connect(serverPerso, httpPort);
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                      "Host: " + serverPerso + "\r\n" +
                      "Connection: close\r\n\r\n");
          delay(1000);
          reboot = lectureConsigne (client, String("\"Reboot\":\""));
          
          if (debug) {
              Serial.print("Reboot ? ");
              Serial.println(reboot);
            }
          if (reboot){
            // Remise à 0 du flag reboot
            url = String("/temperature/temp_rest_re7.php?rquest=write_reboot");
            data = "Radiateur=" + (String)radiateur;
            client.connect(serverPerso, httpPort);
         
            if (debug) {
              Serial.print("demande URL: ");
              Serial.println(url);
            }
            client.println(String("POST ") + url + " HTTP/1.1");          
            client.println("Host: " + (String)serverPerso);
            client.println("Content-Type: application/x-www-form-urlencoded");
            client.println("Connection: close");
            client.println("User-Agent: Arduino/1.0");
            client.print("Content-Length: ");
            client.println(data.length());
            client.println();
            client.print(data);
            client.println();
  
            //On attend un peu
            delay(3000);
            //Reboot de la carte
            ESP.restart();
          }
          /*Fin de l'éventuel reboot */
    
          /*lecture de la température de consigne */
          url = String("/temperature/temp_rest_re7.php?rquest=read_consigne&Radiateur=")+String(radiateur);
     
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
          }
          client.connect(serverPerso, httpPort);
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                      "Host: " + serverPerso + "\r\n" +
                      "Connection: close\r\n\r\n");
          delay(1000);
          consigne = lectureConsigne (client, String("\"Temperature\":\""));
          /*Fin de la lecture de la température de consigne */
  
          /* Mesure de la température de la pièce */
          mesureTemp();
          
          // Gestion des erreurs de lecture de température
          if (isnan(temp_int)) { //Si la lecture de température intérieure a foiré, on tente une nouvelle lecture
            delay(2000);
            mesureTemp();
  
            if (isnan(temp_int)) { //Si la lecture de température intérieure a encore foiré on fixe une valeur basse
              //temp_int=2;
              //modeChauf=8; //La sonde à foiré deux fois de suite, on passe en mode de sécurité.
            }
          }
  
          /* Prise de décision chauffage*/  
          if (temp_int<consigne) {
            allume = 1;
            digitalWrite(chauffage, HIGH);
          }
          else {
            allume = 0;
            digitalWrite(chauffage, LOW);
          }
          /* Fin prise de décision chauffage*/
  
          /* Sauvegarde des logs */
          url = String("/temperature/temp_rest_re7.php?rquest=insert_temp_minute");
          data = "rad=" + (String)radiateur +  "&temp_ext=" + (String)temperature +"&temp=" + (String)temp_int + "&Consigne=" + (String)consigne + "&Allume=" + (String)allume;
          if (debug){
            Serial.print("Connexion au serveur : ");
            Serial.println(serverPerso);
            Serial.println(url);
            Serial.println(data);
          }
          
          client.connect(serverPerso, httpPort);
         
          if (debug) {
            Serial.print("demande URL: ");
            Serial.println(url);
          }
          client.println(String("POST ") + url + " HTTP/1.1");          
          client.println("Host: " + (String)serverPerso);
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.println("Connection: close");
          client.println("User-Agent: Arduino/1.0");
          client.print("Content-Length: ");
          client.println(data.length());
          client.println();
          client.print(data);
          client.println();
          /* Fin sauvegarde des logs */
        }
  
        /* Pause avant la prochaine mesure et reboot régulier */
        delay((timeoutMesure*60000)-12000);
        temps = temps + timeoutMesure;
        if (temps >= (timeoutReset*(60/timeoutMesure))) {
          ESP.restart(); // Redémarrage régulier
        }
        boucle++;
    }
    else if (modeChauf==8) {
        //On fait une mesure de température
        //Si elle est ok on repasse en modeChauf 0
        //Sinon on va lire la température remontée par le radiateur le plus proche
          // On déclenche en fonction 
          // On fait une pause de 10 minutes
          // On repasse en modeChauf 0
        //Sinon si on a pas de radiateur voisin on compare la tempé intérieure et extérieure et on déclenche toutes les N heures
            // Au dela de 10 degrés d'écart = 15 minutes toutes les heures
            // Entre 8 et 10 degrés d'écart = 20 minutes environ toutes les 1h30 la nuit et toutes les 2h la journée
            // Entre 5 et 8 degrés d'écart = 20 minutes environ toutes les 2h30 la nuit
            // En dessous de 5 degrés d'écart = 20 minutes toutes les 12h
            
        //On fait une pause de 10 minutes
    }
    else if (modeChauf==9) {
        if (debug) {
            Serial.print("Connexion au WiFi ");
            Serial.println(ssid);
        }
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);    // On se connecte
        WiFi.waitForConnectResult();
        Serial.println(WiFi.status());
        delay(1000);
        temps = 0;
        blink=HIGH;
    
        while (WiFi.status() != WL_CONNECTED && tempsSauve < timeoutWiFi) { // On attend
            delay(500);
            tempsSauve=tempsSauve+500;
            digitalWrite(led, blink);
            if (blink==HIGH){
                blink = LOW;
            }
            else {
                blink = HIGH;
            }
            if (debug) {
                Serial.print(".");
            }
        }
        // Si on dépasse le temps de connexion au WIFI
        if (tempsSauve >= timeoutWiFi) {
            /* Mesure de la température de la pièce */
            mesureTemp();
            
            /* Prise de décision chauffage en mode secours*/  
            if (temp_int<consigneSauve) {
                allume = 1;
                digitalWrite(chauffage, HIGH);
                delay(timeoutMesure*60000);
            }
            else {
                allume = 0;
                digitalWrite(chauffage, LOW);
            }
            /* Fin prise de décision chauffage en mode secours*/
            ESP.restart(); 
        }

        // Si on arrive jusqu'ici c'est que le WIFI est connecté
        // Passage du wifi en mode éco énergie
        // wifi_set_sleep_type(LIGHT_SLEEP_T);
        // Passage en mode confort
        modeChauf=0;
        if (debug) {
            Serial.println("");  // on affiche les paramètres
            Serial.println("WiFi connecté");
            Serial.print("Adresse IP du module ESP: ");
            Serial.println(WiFi.localIP());
            Serial.print("Adresse IP de la box : ");
            Serial.println(WiFi.gatewayIP());
        }
    }
}


/* Lecture de la température et de l'humidité, avec gestion des erreurs */
byte mesureTemp (){
    humidity_int = dht.readHumidity();
    // Lecture de la température en Celcius
    temp_int = dht.readTemperature();
    if (debug) {
      Serial.print("Temperature : ");
      Serial.print(temp_int);
      Serial.print(" | Humidite : ");
      Serial.println(humidity_int);
    }
    
    return DHT_SUCCESS;
}

byte lectureValeurs(WiFiClient cli, String keyword, String keyword2) {
  boolean inBody = false; // on est dans l'en-tête

  // On lit les données reçues, s'il y en a
  while (cli.available()) {
    String line = cli.readStringUntil('\r');

    if (line.length() == 1) inBody = true; /* passer l'en-tête jusqu'à une ligne vide */

   if (inBody) {  // ligne du corps du message, on cherche le mot clé
      int pos = line.indexOf(keyword);

      if (pos > 0) { /* mot clé trouvé */
        // indexOf donne la position du début du mot clé, en ajoutant sa longueur
        // on se place à la fin.
        pos += keyword.length();
        temperature = atof(&line[pos]) - 273.15;
        if (debug) {
          Serial.println(temperature);
        }
      }

      pos = line.indexOf(keyword2);

      if (pos > 0) { /* mot clé trouvé */
        // indexOf donne la position du début du mot clé, en ajoutant sa longueur
        // on se place à la fin.
        pos += keyword2.length();
        humidity = atoi(&line[pos]);
        if (debug) {
          Serial.println(humidity);
        }
      }
    }
  }
}

float lectureConsigne(WiFiClient cli, String keyword) {
  boolean inBody = false; // on est dans l'en-tête
  float resultat;

  // On lit les données reçues, s'il y en a
  while (cli.available()) {
    String line = cli.readStringUntil('\r');

    if (line.length() == 1) inBody = true; /* passer l'en-tête jusqu'à une ligne vide */
    if (inBody) {  // ligne du corps du message, on cherche le mot clé
      int pos = line.indexOf(keyword);
      if (pos > 0) { /* mot clé trouvé */
        // indexOf donne la position du début du mot clé, en ajoutant sa longueur
        // on se place à la fin.
        pos += keyword.length();
        resultat = atof(&line[pos]);
        if (debug) {
          Serial.println(resultat);
        }
      }
    }
  }
  return resultat;
}


