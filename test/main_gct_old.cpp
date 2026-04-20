#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <HardwareSerial.h>

const char* ssid = "Wifi_PC";//Nom du SSID (Wifi)
const char* password = "12345678";//Code du Wifi

AsyncWebServer server(80);//Port du serveur Web 
WebSocketsServer webSocket = WebSocketsServer(81);//port du Websocket
String htmlPage = R"rawliteral(

<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <title>GCT Interface</title>
  <style>
    /* Style de base conservé du terminal */
    body {
      background-color: #121212;
      color: #00ff90;
      font-family: 'Courier New', monospace;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      justify-content: center; /* Centre verticalement */
      align-items: center;     /* Centre horizontalement */
      height: 100vh;
    }

    h2 {
      text-align: center;
      margin-bottom: 2rem;
      color: #00ff90;
      font-size: 1.5rem;
      text-transform: uppercase;
      letter-spacing: 2px;
    }

    /* Container des boutons */
    .button-container {
      display: flex;
      flex-direction: column;
      gap: 1rem;
      width: 90%;
      max-width: 400px;
    }

    /* Style des boutons identique à l'original */
    button {
      padding: 0.5rem 1rem;
      font-size: 1.1rem;
      font-family: 'Courier New', monospace;
      background-color: #00ff90;
      color: #000;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      transition: background 0.2s, transform 0.1s;
      height: 55px; /* Légèrement plus grand pour le confort */
      font-weight: bold;
    }

    button:hover {
      background-color: #00c170;
      transform: scale(1.02);
    }

    button:active {
      transform: scale(0.98);
    }

    /* Adaptation mobile */
    @media (orientation: portrait) {
      button {
        font-size: 1.3rem;
        height: 70px;
      }
    }
  </style>
</head>
<body>

  <h2>Contrôle électrovannes GCT</h2>

  <div class="button-container">
    <button onclick="sendCommand('ACTION_1')">ELECTROVANNES SELECTIVES</button>
    <button onclick="sendCommand('ACTION_2')">ELECTROVANNES PERMISSIVES A</button>
    <button onclick="sendCommand('ACTION_3')">ELECTROVANNES PERMISSIVES B</button>
    <button onclick="sendCommand('ACTION_4')">CYCLE COMPLET</button>
  </div>

  <script>
    // Conservation de la logique WebSocket si besoin 
    let socket;
    try {
      socketA = new WebSocket("ws://192.168.1.2:81/");
      
    } catch (e) {
      console.warn("Connexion WebSocket impossible.");
    }

 if (socketA) {
      socketA.onmessage = function(event) {
        const split_data = event.data.split(":");///type_msg:value ///
        const type_msg = split_data[0];
        
        console.log(split_data);
        //console.log(type_msg);

        if (type_msg === "rssiA"){
          const value = split_data[1];
          console.log("Rssi= "+value);

        }

      };
    }


    function sendCommand(cmd) {
      console.log("Envoi de :", cmd);
      if (socketA?.readyState === WebSocket.OPEN) {
        socketA.send(cmd);
		    
      } else {
        alert("Action simulée : " + cmd + "\n(Le serveur ESP32 n'est pas connecté)");
      }
    }

  </script>
</body>
</html>

)rawliteral";

IPAddress local_ip(192, 168, 1, 2);        // IP de l'ESP32
IPAddress gateway(192, 168, 1, 1);         // La passerelle peut être la même que l'IP
IPAddress subnet(255, 255, 255, 0);        // Masque de sous-réseau

void onwebsocketevent(uint8_t num,WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type){
    case WStype_TEXT:
    Serial.write(payload,lenght);
    Serial.write("\r");
    break;
    default:
    break;
  }
}

void setup() {

  Serial.begin(9800); //Port serie de L'ESP 32
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_ip,gateway,subnet)){
    Serial.println("Ya une couile avec le WIFI");
  }
  WiFi.begin(ssid);
  server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send(200,"text/html",htmlPage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onwebsocketevent);
  while (WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print("x");
  }

  
 



}// Fin du Setup
//declaration des fonction est varibles Global 


void loop() {
  webSocket.loop();
  Serial.println(WiFi.RSSI());
  webSocket.broadcastTXT("rssiA:"+String(WiFi.RSSI()));

  delay(500);

}
