#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include "LittleFS.h"

/* ================= WIFI ================= */
const char* ssid = "Wifi_PC";                         //Nom du SSID (Wifi)
const char* password = "gct123456";                   //Code du Wifi
IPAddress local_ip(192, 168, 1, 2);                   // IP de l'ESP32
IPAddress gateway(192, 168, 1, 1);                    // La passerelle peut être la même que l'IP
IPAddress subnet(255, 255, 255, 0);                   // Masque de sous-réseau

/* ================= WEBSOCKET ================= */
AsyncWebServer server(80);                            //Port du serveur Web 
WebSocketsServer webSocket = WebSocketsServer(81);    //port du Websocket

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
      font-family: 'Courier New', monospace;
      margin: 50px;
      padding: 0;
      display: flex;
      flex-direction: column;
      justify-content: flex-start;
      align-items: center;     /* Centre horizontalement */
      min-height: 100vh;
    }

    h1 {
      text-align: center;
      margin-bottom: 1rem;
      color: #00ff90;
      font-size: 1.5rem;
      text-transform: uppercase;
      letter-spacing: 2px;
    }

    h2 {
      text-align: center;
      margin-bottom: 1rem;
      color: #00ff90;
      font-size: 1.5rem;
      text-transform: uppercase;
      letter-spacing: 2px;
    }

    .FDC-label {
      color: #00ff90;
    }

    button {
      padding: 10px 20px;
      font-size: 16px;
      cursor: pointer;
    }
    button:hover {
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

    .commande {
      display: flex;
      align-items: center;
      margin-bottom: 12px;
      width: 360px;
    }

    .btn-marche {
      flex: 1;
      padding: 0.5rem 1.1rem;
      font-size: 1rem;
      font-family: 'Courier New', monospace;
      background-color: #00ff90;
      color: #000000;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      height: 50px;
      font-weight: bold;
      max-width: 260px;
      text-align: center;
    }

    .btn-arret {
      width: 60px;
      height: 30px;
      margin-left: 10px;
      display: flex;
      justify-content: center;
      align-items: center;
      background-color: #ff0000;
      color: #ffffff;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 1.3rem;
      font-family: 'Courier New', monospace;
      font-weight: bold;
    }
  
    /* Conteneur global des deux vannes */
    .vannes-container {
      display: flex;
      justify-content: center;
      gap: 80px;               /* espace entre vanne 1 et vanne 2 */
      margin-top: 20px;
    }

    /* Bloc d'une vanne */
    .vanne {
      display: flex;
      flex-direction: column;
      align-items: center;
      font-family: 'Courier New', monospace;
    }

    /* Titre vanne */
    .vanne-label {
      font-weight: bold;
      font-size: 1.2rem;
      margin-bottom: 2px;
      line-height: 1.1;
      color: #00ff90;
    }

    /* Image vanne */
    .vanne-img {
      display: block;
      width: 250px;
      margin-bottom: 2px;
    }

    /* Ligne FDC */
    .fdc {
      font-size: 1.1rem;
    }

    /* État FDC */
    .fdc-etat {
      padding: 2px 6px;
      border-radius: 4px;
      font-weight: bold;
      font-size: 1rem;
    }

    /* Inactif */
    .fdc-etat-off {
      background-color: #ff0000;
      color: white;
    }

    /* Actif */
    .fdc-etat-on {
      background-color: #0077ff;
      color: black;
    }
    /* Chronomêtre*/
    .chrono{
      color: #00ff90;
      font-family: 'Courier New', monospace;
      font-size: 1.2rem;
      margin-top: 5px;
    }
  </style>
</head>
<body>

  <h1>Contrôle électrovannes GCT</h1>

  <div class="commande">
    <button class="btn-marche" onclick="sendCommand('ACTION_1')">CYCLE COMPLET</button>
    <button class="btn-arret" onclick="sendCommand('ACTION_2')">OFF</button>
  </div>

  <div class="commande">
  <button class="btn-marche" onclick="sendCommand('ACTION_3')">ELECTROVANNES SELECTIVES</button>
  <button class="btn-arret" onclick="sendCommand('ACTION_4')">OFF</button>
  </div>

  <div class="commande">
    <button class="btn-marche" onclick="sendCommand('ACTION_5')">ELECTROVANNES PERMISSIVES A</button>
    <button class="btn-arret" onclick="sendCommand('ACTION_6')">OFF</button>
  </div>

  <div class="commande">
    <button class="btn-marche" onclick="sendCommand('ACTION_7')">ELECTROVANNES PERMISSIVES B</button>
    <button class="btn-arret" onclick="sendCommand('ACTION_8')">OFF</button>
  </div>

  <h2>État des FDC Ouverture</h2>

  <div class="vannes-container">

    <!-- VANNE 1 -->
    <div class="vanne">
      <div>
      <span class="vanne-label">Vanne 1</span>
      </div>

      <img src="/Symbole_vanne.png" class="vanne-img">

      <div class="fdc">
        <span class="FDC-label">FDC ouverture :</span>
        <span id="FDC1" class="fdc-etat-off">INACTIF</span>
      </div>

      <div class="chrono">
        <span>Temps :</span>
        <span id="chronoV1">00.00.000</span>
      </div>
    </div>

    <!-- VANNE 2 -->
    <div class="vanne">
      <div>
      <span class="vanne-label">Vanne 2</span>
      </div>

      <img src="/Symbole_vanne.png" class="vanne-img">

      <div class="fdc">
        <span class="FDC-label">FDC ouverture :</span>
        <span id="FDC2" class="fdc-etat-off">INACTIF</span>
      </div>

      <div class="chrono">
        <span>Temps :</span>
        <span id="chronoV2">00.00.000</span>
      </div>
    </div>
  </div>

  <script>
    const ESP32_A_IP = "192.168.1.2";
    const ESP32_B_IP = "192.168.1.3";
  </script>

  <script>
    function formatTime(ms) {
      const m = Math.floor(ms / 60000);
      const s = Math.floor((ms % 60000) / 1000);
      const msRest = ms % 1000;
      return (
        String(m).padStart(2, "0") + ":" +
        String(s).padStart(2, "0") + "." +
        String(msRest).padStart(3, "0")
      );
    }

    function updatechrono(id, value) {
      document.getElementById(id).textContent =
        formatTime(value);
    }
  </script>

  <script>
    function updateFDCouverture(id, value) {
      const el = document.getElementById(id);
      if (!el) return;
      if (value == 1) {
        el.textContent = "ACTIF";
        el.className = "fdc-etat-on";
      } else {
        el.textContent = "INACTIF";
        el.className = "fdc-etat-off";
      }
    }
  </script>

  <script>
    let socketA;
    let socketB;
    try {
      socketA = new WebSocket("ws://192.168.1.2:81/");
      socketB = new WebSocket("ws://192.168.1.3:81/");
    } catch (e) {
      console.warn("Connexion WebSocket impossible.");
    }
  </script>

  <script>
    if (socketA)
    socketA.onmessage = (event) => {
      const [type, value] = event.data.split(":")
      if (type == "FDC1") {
        updateFDCouverture ("FDC1" , value);
      }
      if (type == "FDC2") {
        updateFDCouverture ("FDC2" , value);
      }
      if (type == "Chrono_1") {
        updatechrono ("chronoV1" , value);
      }
      if (type == "Chrono_2") {
        updatechrono ("chronoV2" , value);
      }
    }
  </script>

  <script>
    if (socketB) {
      socketB.onopen =() => {
      }
    }
  </script>

  <script> 
    function surveillerSocket(socket, nom) {
      if (!socket) {
        console.error(`Le socket pour ${nom} n'est pas initialisé.`);
        return;
      }

      socket.onopen = () => {
        console.log(`${nom} est maintenant CONNECTÉ.`);
      };

      socket.onclose = () => {
        console.warn(`${nom} est DÉCONNECTÉ (ou n'a pas pu se connecter).`);
      };

      socket.onerror = (error) => {
        console.error(`Erreur sur ${nom} :`, error);
      };
    }

    surveillerSocket(socketA, "ESP32_A");
    surveillerSocket(socketB, "ESP32_B");
  </script>

 <script>
  function sendCommand(cmd) {
    const actionsA = ["ACTION_3", "ACTION_4", "ACTION_5", "ACTION_6"];
    const actionsB = ["ACTION_7", "ACTION_8"];
    const actionsAB = ["ACTION_1", "ACTION_2"];

    let sent = false;

    if (actionsAB.includes(cmd)) {
      if (socketA?.readyState !== WebSocket.OPEN || socketB?.readyState !== WebSocket.OPEN) {
        alert("Erreur : Au moins un des deux ESP32 n'est pas connecté.");
      }
      if (socketA?.readyState === WebSocket.OPEN) {
        socketA.send(cmd);
      }
      if (socketB?.readyState === WebSocket.OPEN) {
        socketB.send(cmd);
      }
      return;
    }
    
    if (actionsA.includes(cmd)) {
      if (socketA?.readyState == WebSocket.OPEN) {
        socketA.send(cmd);
      } else {
        alert("Erreur : l'ESP32 A n'est connecté pour cette action.");
      }
      return;
    }
  
    if (actionsB.includes(cmd)) {
      if (socketB?.readyState == WebSocket.OPEN) {
        socketB.send(cmd);
      } else {
        alert("Erreur : l'ESP32 B n'est connecté pour cette action.");
      }
      return;
    }
  }
</script>
</body>
</html>

)rawliteral";

/* ================= I/O ================= */
#define RELAY1_PIN 17 // pour l'esp32 A
#define RELAY2_PIN 18 // pour l'esp32 A
#define RELAY3_PIN 4  // pour l'esp32 B
#define FDC1_PIN   33 // pour l'esp32 A
#define FDC2_PIN   27 // pour l'esp32 A

/* ================= ETATS ================= */
bool relay1 = false;
bool relay2 = false;
bool relay3 = false;

bool chronoRunning1 = false;
bool chronoRunning2 = false;
unsigned long chronoStart = 0;
unsigned long chronoValue1 = 0;
unsigned long chronoValue2 = 0;

bool lastFDC1 = false;
bool lastFDC2 = false;

unsigned long tempsFinDeCourse = 0; // Stocke le moment où les FDC ont été touchés
bool enAttenteDeCoupure = false;    // Indique si on est dans la période de tempo

unsigned long time_loop = 0;

/* ================= WEBSOCKET EVENT ================= */

void Action_1(){
  //ACTIONS SUR LES 3 RELAIS + START CHRONO
  if (!chronoRunning1 && !chronoRunning2) {
    relay1 = relay2 = relay3 = true;
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    digitalWrite(RELAY3_PIN, HIGH);
    chronoStart = millis();
    chronoRunning1 = true;
    chronoRunning2 = true;
    Serial.println("3 sorties ON → chrono démarré");
  }
}

void Action_2(){
  //ACTION SUR LE REALIS DE L'ELECTRO SELECTIVE
  relay1 = relay2 = relay3 = false;
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  chronoRunning1 = false;
  chronoRunning2 = false;
  lastFDC1 = false;
  lastFDC2 = false;
  enAttenteDeCoupure = false;
  Serial.println("OFF général");
}

void Action_3(){
  //ACTION SUR LE REALIS DE L'ELECTRO SELECTIVE
  digitalWrite(RELAY1_PIN, HIGH);
  Serial.println("Relais éléctro selective → ON");
}

void Action_4(){
  digitalWrite(RELAY1_PIN, LOW);
  Serial.println("Relais éléctro selective → OFF");
}

void Action_5(){
  //ACTION SUR LE REALIS DE L'ELECTRO PERMISSIVE A
  digitalWrite(RELAY2_PIN, HIGH);
  Serial.println("Relais éléctro permissive A → ON");
}

void Action_6(){
  digitalWrite(RELAY2_PIN, LOW);
  Serial.println("Relais éléctro permissive A → OFF");
}

void Action_7(){
  //ACTION SUR LE REALIS DE L'ELECTRO PERMISSIVE B
  digitalWrite(RELAY3_PIN, HIGH);
  Serial.println("Relais éléctro permissive B → ON");
}

void Action_8(){
  digitalWrite(RELAY3_PIN, LOW);
  Serial.println("Relais éléctro permissive B → OFF");
}

void onwebsocketevent(uint8_t num,WStype_t type, uint8_t * payload,size_t length) {
  if (type != WStype_TEXT) return;

  String cmd = String((char*)payload).substring(0, length);
  Serial.println("Commande reçue : " + cmd);
    
  if (cmd == "ACTION_1"){
    Action_1();
  }
  if (cmd == "ACTION_2"){
    Action_2();
  }
  if (cmd == "ACTION_3"){
    Action_3();
  }
  if (cmd == "ACTION_4"){
    Action_4();
  }
  if (cmd == "ACTION_5"){
    Action_5();
  }
  if (cmd == "ACTION_6"){
    Action_6();
  }
  if (cmd == "ACTION_7"){
    Action_7();
  }
  if (cmd == "ACTION_8"){
    Action_8();
  }
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(9600);//Port serie de L'ESP 32

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(FDC1_PIN, INPUT_PULLUP);
  pinMode(FDC2_PIN, INPUT_PULLUP);

  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);

  WiFi.mode(WIFI_STA);
   if (!WiFi.config(local_ip,gateway,subnet)){
    Serial.println("Ya une couile avec le WIFI");
  }
  WiFi.begin(ssid);

  // Initialiser LittleFS
  if(!LittleFS.begin(true)){
    Serial.println("Erreur lors du montage de LittleFS");
    return;
  }
  
  // Route principale
  server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send(200,"text/html",htmlPage);
  });

  server.serveStatic("/", LittleFS, "/");

  server.begin();
  webSocket.begin();
  webSocket.onEvent(onwebsocketevent);
  while (WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print("x");
  }
}

void loop() {
  webSocket.loop();
  //Serial.println(WiFi.RSSI());
  webSocket.broadcastTXT("rssiA:"+String(WiFi.RSSI()));
  delay(500);

  // Lecture FDC
  bool fdc1 = (digitalRead(FDC1_PIN)==LOW);
  bool fdc2 = (digitalRead(FDC2_PIN)==LOW);

  if (fdc1 && !lastFDC1 && chronoRunning1) {
    chronoValue1 = millis() - chronoStart;
    webSocket.broadcastTXT("Chrono_1:" + String(chronoValue1));
    chronoRunning1 = false;
    lastFDC1 = fdc1;
    Serial.println("FDC reçu");
    Serial.print("Temps mesuré : ");
    Serial.print(chronoValue1);
    Serial.println(" ms");
    webSocket.broadcastTXT("FDC1:" + String(1));
  }
  if (fdc2 && !lastFDC2 && chronoRunning2) {
    chronoValue2 = millis() - chronoStart;
    webSocket.broadcastTXT("Chrono_2:" + String(chronoValue2));
    chronoRunning2 = false;
    lastFDC2 = fdc2;
    Serial.println("FDC reçu");
    Serial.print("Temps mesuré : ");
    Serial.print(chronoValue2);
    Serial.println(" ms");
    webSocket.broadcastTXT("FDC2:" + String(1));
  }

  // 1. Détection du moment où les deux FDC sont activés
  if (fdc1 && fdc2 && !enAttenteDeCoupure) {
    tempsFinDeCourse = millis(); // On lance le chrono
    enAttenteDeCoupure = true;   // On note qu'on attend
  }

  // 2. Vérification si le délai est écoulé (ex: 2000ms)
  if (enAttenteDeCoupure && (millis() - tempsFinDeCourse >= 5000)) {
    // Arrêt des relais après 2 secondes
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    relay1 = relay2 = relay3 = false;
    
    enAttenteDeCoupure = false; // Reset pour la prochaine fois
  }

  if (!fdc1 && lastFDC1) {
    webSocket.broadcastTXT("FDC1:0");
    lastFDC1 = false;
  }
  if (!fdc2 && lastFDC2) {
    webSocket.broadcastTXT("FDC2:0");
    lastFDC2 = false;
  }

  if (millis()- time_loop >= 1000){
    chronoValue1 = millis() - chronoStart;
    webSocket.broadcastTXT("Chrono_1:" + String(chronoValue1));
    Serial.println("etat fdc1: "+String(fdc1));
    Serial.println("etat lastFDC1: "+String(lastFDC1));
    Serial.println("etat chronoRunning1: "+String(chronoRunning1));
    chronoValue2 = millis() - chronoStart;
    webSocket.broadcastTXT("Chrono_2:" + String(chronoValue2));
    Serial.println("etat fdc2: "+String(fdc2));
    Serial.println("etat lastFDC2: "+String(lastFDC2));
    Serial.println("etat chronoRunning2: "+String(chronoRunning2));
  }
}