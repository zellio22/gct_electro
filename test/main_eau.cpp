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
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>EAU Terminal</title>
  <style>
    @media (orientation: portrait) {
      .terminal-box {
        width: 57rem;
        height: 10px;
      }
      #terminal {
        font-size: 2rem;
      }
      .button-row {
        flex-direction: row;
        gap: 0.5rem;
      }
      .button-row button {
        display: flex;
        text-align: center;
      }
    }

    body {
      background-color: #121212;
      color: #00ff90;
      font-family: 'Courier New', monospace;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      height: 100vh;
    }

    h2 {
      text-align: center;
      margin: 0.25rem 0;
      padding: 0.25rem 0;
      color: #00ff90;
      font-size: 1rem;
    }

    .terminal-box {
      flex: 1;
      background-color: #1e1e1e;
      border: 1px solid #00ff90;
      margin: 1rem;
      padding: 1rem;
      border-radius: 6px;
      display: flex;
      flex-direction: column;
    }

    #terminal {
      flex: 1;
      background-color: #000000;
      color: #00ff90;
      padding: 1rem;
      resize: none;
      border: none;
      font-size: 1rem;
      line-height: 1.4;
      overflow-y: auto;
    }

    .input-section {
      display: flex;
      padding: 1rem;
      background-color: #1e1e1e;
      gap: 0.5rem;
      border-top: 1px solid #00ff90;
      height: 45px;
    }

    input[type="text"] {
      flex: 1;
      padding: 0.5rem;
      font-size: 1rem;
      background-color: #000;
      color: #00ff90;
      border: 1px solid #00ff90;
      border-radius: 4px;
    }

    button {
      padding: 0.5rem 1rem;
      font-size: 1rem;
      background-color: #00ff90;
      color: #000;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      transition: background 0.2s;
      height: 45px;
    }

    button:hover {
      background-color: #00c170;
    }

    .button-row {
      display: flex;
      justify-content: space-around;
      padding: 1rem;
      background-color: #1e1e1e;
      border-top: 1px solid #00ff90;
    }

    .button-row button {
      flex: 1;
      margin: 0 0.5rem;
    }

    #dashboard {
      display: none;
      padding: 1rem;
      background-color: #1e1e1e;
      border-top: 1px solid #00ff90;
    }

    #dashboard label {
      margin-bottom: 0.5rem;
      font-size: 0.9rem;
    }

    #toggleBtn {
      position: absolute;
      top: 10px;
      left: 10px;
      padding: 5px 10px;
      font-size: 0.8rem;
      height: auto;
      z-index: 10;
    }

    /* Progress bars */
    .progress-bar {
      position: relative;
      width: 100%;
      height: 20px;
      background-color: #000;
      border: 1px solid #00ff90;
      border-radius: 4px;
      overflow: hidden;
    }
    .empty {
      position: absolute;
      top: 0;
      right: 0;
      height: 100%;
      background-color: #000000; /* Couleur vide */
      width: 100%;
    }
    .fill {
      position: absolute;
      top: 0;
      left: 0;
      height: 100%;
      background-color: #00ff90; /* Couleur remplie */
      width: 0%;
      transition: width 0.2s ease;
    }
    
    .cursor {
      position: absolute;
      top: -3px;
      width: 10px;
      height: 26px;
      background-color: #aaff90;
      border-radius: 3px;
      transition: left 0.1s ease-out;
      z-index: 10; /* >>> met le curseur au-dessus du remplissage */
}


  </style>
</head>
<body>
  <button onclick="toggleDashboard()" id="toggleBtn">Afficher le Dashboard</button>
  <h2>Terminal EAU</h2>

  <div class="terminal-box" id="terminalSection">
    <textarea id="terminal" readonly></textarea>
  </div>

  <div class="input-section" id="inputSection">
    <input type="text" id="input" placeholder="Enter command">
    <button onclick="sendMessage()">Send</button>
  </div>

  <div class="button-row" id="commandButtons">
    <button onclick="insertCommand('CLEAR EVENT LED')">Clear event led</button>
    <button onclick="insertCommand('aq ft 4000')">aq ft 4000</button>
    <button onclick="insertCommand('Alarm off')">Alarm off</button>
    <button onclick="insertCommand('Alarm 1')">Alarm 1</button>
    <button onclick="insertCommand('\\\\\\')">\\\</button>
  </div>

  <div id="dashboard">
    <div style="display: flex; flex-direction: column; gap: 1.5rem;">
      <label>Slider :
        <input type="range" id="rangeSlider" min="0" max="100" value="50" oninput="updateProgress(this.value)">
      </label>

      <div class="progress-bar" id="bar1">
        <div class="empty"></div>
        <div class="fill" id="fill1"></div>
        <div class="cursor" id="cursor1"></div>
      </div>

      <div class="progress-bar" id="bar2">
        <div class="empty"></div>
        <div class="fill" id="fill2"></div>
        <div class="cursor" id="cursor2"></div>
      </div>

      <div class="progress-bar" id="bar3">
        <div class="empty"></div>
        <div class="fill" id="fill3"></div>
        <div class="cursor" id="cursor3"></div>
      </div>

      <div class="progress-bar" id="bar4">
        <div class="empty"></div>
        <div class="fill" id="fill4"></div>
        <div class="cursor" id="cursor4"></div>
      </div>
    </div>
  </div>

  <script>
    let socket;
    try {
      socket = new WebSocket("ws://" + location.hostname + ":81/");
    } catch (e) {
      console.warn("WebSocket not connected (ESP32 non disponible).");
    }

    const term = document.getElementById("terminal");
    const inputField = document.getElementById("input");


/*split 
const chaine = "bar:1:75.00:red";
const resultat = chaine.split(":");
const premierElement = resultat[0];
console.log(premierElement); // Affichera "bar"

*/

    if (socket) {
      socket.onmessage = function(event) {
        const split_data = event.data.split(":");///bar:1:75.00:red ///
        const type_msg = split_data[0];
        
        //console.log(split_data);
        //console.log(type_msg);

        if (type_msg === "ser"){
            term.scrollTop = term.scrollHeight;
            const data_msg = split_data[1];
            console.log("Serial: "+data_msg);
            term.value += data_msg;
        }
        if (type_msg === "bat"){
            //console.log("Bat: "+split_data[1]+" V");
        }
        if (type_msg === "bar"){
            
            let num_bar = Number(split_data[1]);//bar:1:75.00:red
            let prc_bar = Number(split_data[2]);//bar:1:75.00:red
            let col_bar = split_data[3];//bar:1:75.00:red
            updateBar(num_bar,prc_bar,col_bar);
            //console.log("Num_Bar: "+num_bar+" Couleur: "+col_bar+" Value: "+prc_bar+" %");
        }
      };
    }

    function sendMessage() {
      const msg = inputField.value.trim();
      if (msg === "") return;
      term.value += "> " + msg + "\n";
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(msg);
        console.log("Simulated send:", msg);
      } else {
        console.log("Simulated send:", msg);
      }
      term.scrollTop = term.scrollHeight;
      inputField.value = "";
    }

    function insertCommand(cmd) {
      inputField.value = cmd;
      inputField.focus();
    }

    function toggleDashboard() {
      const dashboard = document.getElementById("dashboard");
      const terminal = document.getElementById("terminalSection");
      const input = document.getElementById("inputSection");
      const buttons = document.getElementById("commandButtons");
      const toggleBtn = document.getElementById("toggleBtn");

      const isDashboardVisible = dashboard.style.display === "block";

      dashboard.style.display = isDashboardVisible ? "none" : "block";
      terminal.style.display = isDashboardVisible ? "flex" : "none";
      input.style.display = isDashboardVisible ? "flex" : "none";
      buttons.style.display = isDashboardVisible ? "flex" : "none";

      toggleBtn.textContent = isDashboardVisible ? "Afficher Electro" : "Afficher terminal";
    }

    function updateProgress(val) {
      const bars = [1, 2, 3, 4];
      bars.forEach(i => {
        const cursor = document.getElementById(`cursor${i}`);
        const bar = document.getElementById(`bar${i}`);
        
        const width = bar.clientWidth;
        const position = (val / 100) * width;
        cursor.style.left = `${position - 5}px`; 
        
      }); 
    }
    function updateBar(bar,val,col){
      const fill = document.getElementById(`fill${bar}`);
      if (col === "red"){
        fill.style.backgroundColor = "red";
      }
      if (col === "blue"){
        fill.style.backgroundColor = "blue";
      }
      
      fill.style.width = val + "%"; 
    }
  </script>
</body>
</html>






)rawliteral";

IPAddress local_ip(192, 168, 1, 1);        // IP de l'ESP32
IPAddress gateway(192, 168, 1, 1);         // La passerelle peut être la même que l'IP
IPAddress subnet(255, 255, 255, 0);        // Masque de sous-réseau



void setup() {

  
  Serial.begin(115200);
  MySerial.begin(19200, SERIAL_8N1, 18, 17); // RX=39, TX=40sendMessage()
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);/// Normalement ok 
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  Serial.println("Web server + WebSocket started.");

  // def E/S
  pinMode(15,OUTPUT);
  pinMode(39,OUTPUT);
  pinMode(37,OUTPUT);
  pinMode(35,OUTPUT);

  //teste

  digitalWrite(15,true);
  digitalWrite(1,true);
  digitalWrite(2,false);
  digitalWrite(4,true);

}

static String serialBuffer = "";
static String serialusbBuffer = "";

unsigned long lastloop_led = millis();
unsigned long lastloop_read = millis();

int color=0;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_TEXT:
      MySerial.write(payload,length); // envoyer au port série
      MySerial.write('\r'); // envoyer au port série

      Serial.write(payload,length);//pour le port USB
      Serial.write('\r');
      break;
    default:
      break;
  }
}

void loop() {
  webSocket.loop();
  

  while (MySerial.available()) {
    char c = MySerial.read();
    webSocket.broadcastTXT("ser:"+String(c))+"::";
    
    
    /*
    serialBuffer += c; fait un bufer de serial 
    if (c == '\r') { // on envoie ligne par ligne
      webSocket.broadcastTXT("ser:"+serialBuffer+"::");
      serialBuffer = "";// vide le buffer 
    }
    */
  }

  while (Serial.available()) {
    char c = Serial.read();
    webSocket.broadcastTXT("ser:"+String(c))+"::";

    /*
    if (c == '\r') { // on envoie ligne par ligne ou \n je c pas 
      webSocket.broadcastTXT("ser:"+serialusbBuffer)+"::";
      Serial.println(serialBuffer);
      serialusbBuffer = "";
    }
    */
  }

  if(millis()>=lastloop_read + 50){
    lastloop_read=millis();

    uint16_t raw = analogRead(14);
    float vin_adc= (raw / 8191.0)*2.7;
    float vbat = vin_adc*2;

    float hall1 = ((analogRead(11)/8191.0)*100.0-64)*2;//
    float hall2 = ((analogRead(13)/8191.0)*100.0-64)*2;
    float hall3 = ((analogRead(2)-140)/8191.0*100.0-64)*2;
    float hall4 = ((analogRead(3)-240)/8191.0*100.0-64)*2;

    
    if (hall1 <0){
      webSocket.broadcastTXT("bar:1:"+String(fabs(hall1))+":red");
    }
    else{
      webSocket.broadcastTXT("bar:1:"+String(hall1)+":blue");
    }
    if (hall2 <0){
      webSocket.broadcastTXT("bar:2:"+String(fabs(hall2))+":red");
    }
    else{
      webSocket.broadcastTXT("bar:2:"+String(hall2)+":blue");
    }
    if (hall3 <0){
      webSocket.broadcastTXT("bar:3:"+String(fabs(hall3))+":red");
    }
    else{
      webSocket.broadcastTXT("bar:3:"+String(hall3)+":blue");
    }
    if (hall4 <0){
      webSocket.broadcastTXT("bar:4:"+String(fabs(hall4))+":red");
    }
    else{
      webSocket.broadcastTXT("bar:4:"+String(hall4)+":blue");
    }

    webSocket.broadcastTXT("bat:"+String(vbat)+"::\r");

  }  

}
/*
char c = Serial.read();
webSocket.broadcastTXT(String(c));/// pour envoyer part chart
*/ 

/* pour envoyer par ligne 
void loop() {
  webSocket.loop();

  while (Serial.available()) {
    char c = Serial.read();
    serialBuffer += c;

    if (c == '\n') { // on envoie ligne par ligne
      webSocket.broadcastTXT(serialBuffer);
      serialBuffer = "";
    }
  }
}
*/