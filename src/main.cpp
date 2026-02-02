#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <HardwareSerial.h>

const char* ssid = "EAU-Terminal";//Nom du SSID (Wifi)
const char* password = "12345678";//Code du Wifi

#define RATIO_DIVISEUR =2;
#define ADC_ATTEN = ADC_11db;
#define ADC_MAX = 8191.0;
#define VREF = 2.5;

AsyncWebServer server(80);//Port du serveur Web 
WebSocketsServer webSocket = WebSocketsServer(81);//port du Websocket
String htmlPage = R"rawliteral(

ICI La page WEB HTMP CSS JS 
//Le code pour le websocket est dans le fichier olf_main.html

)rawliteral";

IPAddress local_ip(192, 168, 1, 1);        // IP de l'ESP32
IPAddress gateway(192, 168, 1, 1);         // La passerelle peut être la même que l'IP
IPAddress subnet(255, 255, 255, 0);        // Masque de sous-réseau



void setup() {

  
  Serial.begin(115200); //Port serie de L'ESP 32
  //Demarrage du WIFI Serveur WEB et Websocket
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);/// Normalement ok 
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  Serial.println("Web server + WebSocket started.");// Tout est Start

  // définition des Entrées Sorties voir pour ajouter les Pull UP 
  pinMode(15,OUTPUT);


}// Fin du Setup
//declaration des fonction est varibles Global 

//Fonction de l'ecture du websocket 
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_TEXT:

      Serial.write(payload,length);//pour le port USB
      Serial.write('\r');
      break;
    default:
      break;
  }
}

void loop() {
  webSocket.loop();//start la boucle du websocket 
  
  while (Serial.available()) {//quand il y a des message sur le port serie 
    char c = Serial.read();
    webSocket.broadcastTXT("ser:"+String(c))+"::";//envoi le message sur le websocket 
  }

}
