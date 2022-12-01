#include <ESP8266WiFi.h>        // Gestió de la Wifi
#include <PubSubClient.h>       // Gestió del servidor MQTT
#include "FastLED.h"            // Gestió de la tira de LEDs Direccionables

#define PIN_TIRA_LED  2     // Número de GPIO! , no de PIN
#define NUM_LEDS      134   // LEDs que té la tira
#define COLOR_INICI   100   // Tonalitat del color verd en mode HSV
#define ESPERA        30

CRGB leds[NUM_LEDS];    // Array que defineix la tira de LEDs

// WiFi network
const char* ssid = "";
const char* password = "";

// MQTT Server
const char* mqttServer = "";
const int mqttPort = 1883;
const char* mqttUser= "";
const char* mqttPassword = "";

WiFiClient airQClient;
PubSubClient client(airQClient);

unsigned long last_time = 0;
int resetCount = 0;

#define CO2_MAXIM 3000

void lecturaTopic(char* topic, byte* payload, unsigned int length) {
   char mqttReceived[10];
   int co2, altura;
   int transicio;

   Serial.print("S'ha rebut una dada publicada. ");   
   for (int i=0; i<length; i++) {
      mqttReceived[i] = (char)payload[i];
   }
   mqttReceived[length] = 0;
   co2 = String(mqttReceived).toInt();
   Serial.print("Topic: ");   Serial.print(topic);   Serial.print("  Valor: ");   Serial.println(co2);

   if (co2 > CO2_MAXIM) co2 = CO2_MAXIM;
   altura = map(co2, 0, CO2_MAXIM, 0, NUM_LEDS);
   if (altura < 0) altura = 0;
   if (altura > NUM_LEDS) altura = NUM_LEDS;

   transicio = random(1, 9);
   Serial.print("Transició = ");   Serial.println(transicio);
   if (transicio == 1) transicio1_puja_baixa(altura);
   if (transicio == 2) transicio2_baixa_cuc(altura);
   if (transicio == 3) transicio3_explosio_centre(altura);
   if (transicio == 4) transicio4_explosio_doble(altura);
   if (transicio == 5) transicio5_creuament_extrems(altura);
   if (transicio == 6) transicio6_blocs_up_baixa(altura);
   if (transicio == 7) transicio7_blocs_down_baixa(altura);
   if (transicio == 8) transicio8_lectura_puja(altura);
}

void connecta_wifi(void) {
   Serial.print("Connectant al Wifi ");
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   int timeout = 0;
   while ((WiFi.status() != WL_CONNECTED) && (timeout < 20)) {
      Serial.print(".");
      timeout++;
      delay(500);    // timeout en 10 segons
   }
   Serial.println("");
   if (WiFi.status() == WL_CONNECTED) {   
      Serial.println("WiFi connectat");
      Serial.print("IP address: ");   Serial.println(WiFi.localIP());
      // Defineix el servidor MQTT
      client.setServer(mqttServer, mqttPort);
      // Defineix què farà al rebre Topics
      client.setCallback(lecturaTopic);
   }
}

void connecta_mqtt(void) {
   Serial.println("Connectant amb el servidor MQTT ...");
   int timeout = 0;
   while (!client.connected()  && (timeout < 5)) {
      if (client.connect("ClubMaker", mqttUser, mqttPassword)) {
         Serial.println("Connectat amb l'MQTT Server");
         client.subscribe("ClubMaker/feeds/co2");
      }
      else {
         Serial.print("Connexió MQTT fallida amb error ");   Serial.println(client.state());
         timeout++;
         delay(2000);    // timeout en 10 segons
      }
   }
}

void setup() {
   // Inicialitza la tira de LEDs
   FastLED.addLeds<NEOPIXEL, PIN_TIRA_LED> (leds, NUM_LEDS);
   FastLED.clear(true);
   FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
   FastLED.setBrightness(50);

   // Inicialitza la consola sèrie
   Serial.begin(9600);   while(!Serial);
   Serial.println("");

   // Connecta amb el Wifi
   connecta_wifi();
}

void loop() {
   // Comprova connexions Wifi i Mqtt
   if (WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) connecta_mqtt();
      client.loop();
   }
   else {
      Serial.println("Wifi no connectat");
      // mostra el valor en vermell indicant que NO hi ha connexió
      unsigned long now = millis();
      if (now - last_time > 30000) {
         for (int i=0; i<NUM_LEDS; i++) {
            if (leds[i] != CRGB(0, 0, 0)) leds[i] = CRGB(255, 0, 0);
            FastLED.show();  
            delay(ESPERA);
         }
         last_time = now;
      }
   }

   delay(5000);
   resetCount++;
   if (resetCount==4320) { ESP.reset(); }       // Reset ESP cada 21600seg (6h)
}

void transicio1_puja_baixa(int altura) {
   int color;

   for (int i=0; i<NUM_LEDS; i++) {
      color = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color < 0) color = 0;      // A 3/4 de tira ja pintem en vermell
      leds[i] = CHSV(color, 255, 255);
      FastLED.show();  
      delay(ESPERA);
   }
   for (int i=NUM_LEDS-1; i>altura; i--) {
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio2_baixa_cuc(int altura) {
   int color, llum;
   int cua = 20;

   for (int i=NUM_LEDS-1; i>=0-cua; i--) {
      color = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color < 0) color = 0;
      for (int j=0; j<=cua; j++) {
         llum = (cua-j)*255/cua;
         if (j == cua) llum = 0;    // pinta la cua + un últim apagat per esborrar l'anterior
         if ( ((i+j) >= 0) && ((i+j) < NUM_LEDS) ) {
            leds[i+j] = CHSV(color, 255, llum);
         }
      }
      // if ( ((i+cua) >= 0) && ((i+cua) < NUM_LEDS) ) leds[i+cua] = CRGB(0, 0, 0);    // ja no cal perquè hem posat llum = 0
      FastLED.show();
      delay(ESPERA);
   }
   for (int i=0; i<=altura; i++) {
      color = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color < 0) color = 0;
      leds[i] = CHSV(color, 255, 255);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio3_explosio_centre(int altura) {
   int pos_dn, pos_up;
   int color_dn, color_up;
   int llum;
   int cua = 20;

   for(int pos=0; pos<NUM_LEDS/2+cua; pos++) {
      pos_dn = NUM_LEDS/2 - pos - 1;
      pos_up = NUM_LEDS/2 + pos;
      color_dn = COLOR_INICI - pos_dn*COLOR_INICI*4/3/NUM_LEDS;
      color_up = COLOR_INICI - pos_up*COLOR_INICI*4/3/NUM_LEDS;
      if (color_up < 0) color_up = 0;
      for (int j=0; j<=cua; j++) {
         llum = (cua-j)*255/cua;
         if (j == cua) llum = 0;    // pinta la cua + un últim apagat per esborrar l'anterior
         if ( ((pos_dn+j) >= 0) && ((pos_dn+j) < NUM_LEDS/2) ) {
            leds[pos_dn+j] = CHSV(color_dn, 255, llum);
            leds[pos_up-j] = CHSV(color_up, 255, llum);
         }
      }
      FastLED.show();
      delay(ESPERA);
   }
   for (int i=0; i<=altura; i++) {
      color_dn = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color_dn < 0) color_dn = 0;
      leds[i] = CHSV(color_dn, 255, 255);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio4_explosio_doble(int altura) {
   int pos1_dn, pos2_dn, pos1_up, pos2_up;
   int color1_dn, color2_dn, color1_up, color2_up;
   int llum;
   int cua = 10;

   for(int pos=0; pos<NUM_LEDS/2+NUM_LEDS/4+cua; pos++) {
      pos1_dn = NUM_LEDS/2 - pos - 1;
      pos1_up = NUM_LEDS/2 + pos;
      pos2_dn = pos1_dn + NUM_LEDS/4;
      pos2_up = pos1_up - NUM_LEDS/4;
      color1_dn = COLOR_INICI - pos1_dn*COLOR_INICI*4/3/NUM_LEDS;
      color1_up = COLOR_INICI - pos1_up*COLOR_INICI*4/3/NUM_LEDS;
      color2_dn = COLOR_INICI - pos2_dn*COLOR_INICI*4/3/NUM_LEDS;
      color2_up = COLOR_INICI - pos2_up*COLOR_INICI*4/3/NUM_LEDS;
      if (color1_up < 0) color1_up = 0;
      if (color2_up < 0) color2_up = 0;
      for (int j=0; j<=cua; j++) {
         llum = (cua-j)*255/cua;
         if (j == cua) llum = 0;    // pinta la cua + un últim apagat per esborrar l'anterior
         if ( ((pos1_dn+j) >= 0) && ((pos1_dn+j) < NUM_LEDS/2) ) {
            leds[pos1_dn+j] = CHSV(color1_dn, 255, llum);
            leds[pos1_up-j] = CHSV(color1_up, 255, llum);
         }
         if ( ((pos2_dn+j) >= 0) && ((pos2_dn+j) < NUM_LEDS/2) ) {
            leds[pos2_dn+j] = CHSV(color2_dn, 255, llum);
            leds[pos2_up-j] = CHSV(color2_up, 255, llum);
         }
      }
      FastLED.show();
      delay(ESPERA);
   }
   for (int i=0; i<=altura; i++) {
      color1_dn = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color1_dn < 0) color1_dn = 0;
      leds[i] = CHSV(color1_dn, 255, 255);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio5_creuament_extrems(int altura) {
   int pos_up, pos_dn;
   int color_up, color_dn;
   int llum;
   int cua = 20;

   for(int pos=0; pos<NUM_LEDS+cua; pos++) {
      pos_up = pos;
      pos_dn = NUM_LEDS - pos -1;
      color_up = COLOR_INICI - pos_up*COLOR_INICI*4/3/NUM_LEDS;
      color_dn = COLOR_INICI - pos_dn*COLOR_INICI*4/3/NUM_LEDS;
      if (color_dn < 0) color_dn = 0;
      if (color_up < 0) color_up = 0;
      for (int j=cua; j>=0; j--) {    // pintem la cua de darrera cap endavant perquè al creuar-se destaquin els leds més encesos
         llum = (cua-j)*255/cua;
         if (j == cua) llum = 0;
         if ( ((pos_up-j) >= 0) && ((pos_up-j) < NUM_LEDS) ) {
            if ( (j != cua) || ((pos_dn+cua) > pos_up) || ((pos_dn+cua) < (pos_up-cua)) ) {    // no esborrem l'anterior quan es solapen al centre
               leds[pos_up-j] = CHSV(color_up, 255, llum);
               leds[pos_dn+j] = CHSV(color_dn, 255, llum);
            }
         }
      }
      FastLED.show();
      delay(ESPERA);
   }
   for (int i=0; i<=altura; i++) {
      color_up = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color_up < 0) color_up = 0;
      leds[i] = CHSV(color_up, 255, 255);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio6_blocs_up_baixa(int altura) {
   int color;
   int bloc = 20;

   for (int i=0; i<bloc; i++) {
      for (int j=0; j<NUM_LEDS; j+=bloc) {
         color = COLOR_INICI - (j+i)*COLOR_INICI*4/3/NUM_LEDS;
         if (color < 0) color = 0;
         if ((j+i) < NUM_LEDS) leds[j+i] = CHSV(color, 255, 255);
      }
      FastLED.show();  
      delay(ESPERA*2);
   }
   for (int i=NUM_LEDS-1; i>altura; i--) {
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio7_blocs_down_baixa(int altura) {
   int color;
   int bloc = 20;

   for (int i=0; i<bloc; i++) {
      for (int j=NUM_LEDS-1; j>=0; j-=bloc) {
         color = COLOR_INICI - (j-i)*COLOR_INICI*4/3/NUM_LEDS;
         if (color < 0) color = 0;
         if ((j-i) >= 0) leds[j-i] = CHSV(color, 255, 255);
      }
      FastLED.show();  
      delay(ESPERA*2);
   }
   for (int i=NUM_LEDS-1; i>altura; i--) {
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();  
      delay(ESPERA);
   }
}

void transicio8_lectura_puja(int altura) {
   int color;

   for (int i=0; i<NUM_LEDS; i++) {
      for (int j=NUM_LEDS-1; j>i; j--) {
         leds[j] = leds[j-1];
      }
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();  
      delay(ESPERA);
   }
   for (int i=0; i<=altura; i++) {
      color = COLOR_INICI - i*COLOR_INICI*4/3/NUM_LEDS;
      if (color < 0) color = 0;
      leds[i] = CHSV(color, 255, 255);
      FastLED.show();  
      delay(ESPERA);
   }
}
