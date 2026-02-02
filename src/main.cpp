#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <HardwareSerial.h>

/*
Port COM info : 
19200 BAUD Rate
8N1H

Commande a taper
\\\
aq ft 4000
Alarm off
CLEAR EVENT LED
DG
Alarm 1
q

depui le PC EAU Entre faire revenir le curseur de putty au debut de la ligne 

ligne 275 dans 
le JS socket.send(msg + "\n"); supression du \n

Strucure WS 
4 1er caractere le role du truck 
le reste les data 
*/

const char* ssid = "EAU-Terminal";
const char* password = "12345678";

#define RATIO_DIVISEUR =2;
#define ADC_ATTEN = ADC_11db;
#define ADC_MAX = 8191.0;
#define VREF = 2.5;

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
HardwareSerial MySerial(1); // UART1
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
