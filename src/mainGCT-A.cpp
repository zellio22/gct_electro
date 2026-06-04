#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include "LittleFS.h"

/* ================= WIFI ================= */
const char* ssid = "Wifi_PC";
const char* password = "gct123456";
IPAddress local_ip(192, 168, 1, 1);//espA
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

/* ================= WEBSOCKET ================= */
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


/* ================= I/O ================= */
#define RELAY1_PIN 17
#define RELAY2_PIN 18
#define RELAY3_PIN 4
#define FDC1_PIN   33
#define FDC2_PIN   27

/* ================= ETATS ================= */
bool relay1 = false;
bool relay2 = false;
bool relay3 = false;

bool chronoRunning1 = false;
bool chronoRunning2 = false;
unsigned long chronoStart  = 0;
unsigned long chronoValue1 = 0;
unsigned long chronoValue2 = 0;

bool lastFDC1 = false;
bool lastFDC2 = false;

unsigned long tempsFinDeCourse = 0;
bool enAttenteDeCoupure = false;

unsigned long time_loop = 0;

/* ================= ACTIONS ================= */

void Action_1() {
  // Démarre le cycle seulement si aucun chrono ne tourne
  if (!chronoRunning1 && !chronoRunning2) {
    relay1 = relay2 = relay3 = true;
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    digitalWrite(RELAY3_PIN, HIGH);
    chronoStart    = millis();
    chronoRunning1 = true;
    chronoRunning2 = true;
    Serial.println("3 sorties ON → chrono démarré");
  } else {
    Serial.println("Cycle ignoré : déjà en cours");
  }
}

void Action_2() {
  relay1 = relay2 = relay3 = false;
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  chronoRunning1      = false;
  chronoRunning2      = false;
  lastFDC1            = false;
  lastFDC2            = false;
  enAttenteDeCoupure  = false;
  Serial.println("OFF général");
}

void Action_3() {
  digitalWrite(RELAY1_PIN, HIGH);
  Serial.println("Relais électro sélective → ON");
}

void Action_4() {
  digitalWrite(RELAY1_PIN, LOW);
  Serial.println("Relais électro sélective → OFF");
}

void Action_5() {
  digitalWrite(RELAY2_PIN, HIGH);
  Serial.println("Relais électro permissive A → ON");
}

void Action_6() {
  digitalWrite(RELAY2_PIN, LOW);
  Serial.println("Relais électro permissive A → OFF");
}

void Action_7() {
  digitalWrite(RELAY3_PIN, HIGH);
  Serial.println("Relais électro permissive B → ON");
}

void Action_8() {
  digitalWrite(RELAY3_PIN, LOW);
  Serial.println("Relais électro permissive B → OFF");
}

/* ================= WEBSOCKET EVENT ================= */
void onwebsocketevent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type != WStype_TEXT) return;

  String cmd = String((char*)payload).substring(0, length);
  Serial.println("Commande reçue : " + cmd);

  if (cmd == "ACTION_1") Action_1();
  if (cmd == "ACTION_2") Action_2();
  if (cmd == "ACTION_3") Action_3();
  if (cmd == "ACTION_4") Action_4();
  if (cmd == "ACTION_5") Action_5();
  if (cmd == "ACTION_6") Action_6();
  if (cmd == "ACTION_7") Action_7();
  if (cmd == "ACTION_8") Action_8();
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(9600);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(FDC1_PIN, INPUT_PULLUP);
  pinMode(FDC2_PIN, INPUT_PULLUP);

  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);


  WiFi.softAP(ssid);
  WiFi.softAPConfig(local_ip, gateway, subnet);/// Normalement ok 
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  if (!LittleFS.begin(true)) {
    Serial.println("Erreur montage LittleFS");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(204); // 204 = No Content, le navigateur arrête de chercher
  });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(onwebsocketevent);

  Serial.println("Serveur WEB + WebSocket started.");
}

/* ================= LOOP ================= */
void loop() {
  webSocket.loop();
  
  // ---- Lecture FDC ----
  bool fdc1 = (digitalRead(FDC1_PIN) == LOW);
  bool fdc2 = (digitalRead(FDC2_PIN) == LOW);

  // ---- Détection front montant FDC1 ----
  if (fdc1 && !lastFDC1 && chronoRunning1) {
    chronoValue1   = millis() - chronoStart;
    chronoRunning1 = false;
    lastFDC1       = true;
    webSocket.broadcastTXT("Chrono_1:" + String(chronoValue1));
    webSocket.broadcastTXT("FDC1:1");
    Serial.print("FDC1 atteint - Temps : ");
    Serial.print(chronoValue1);
    Serial.println(" ms");
  }

  // ---- Détection front montant FDC2 ----
  if (fdc2 && !lastFDC2 && chronoRunning2) {
    chronoValue2   = millis() - chronoStart;
    chronoRunning2 = false;
    lastFDC2       = true;
    webSocket.broadcastTXT("Chrono_2:" + String(chronoValue2));
    webSocket.broadcastTXT("FDC2:1");
    Serial.print("FDC2 atteint - Temps : ");
    Serial.print(chronoValue2);
    Serial.println(" ms");
  }

  // ---- Retour à 0 des FDC (front descendant) ----
  // FDC1:0 envoyé UNIQUEMENT quand le FDC repasse inactif après avoir été actif
  if (!fdc1 && lastFDC1) {
    lastFDC1 = false;
    webSocket.broadcastTXT("FDC1:0");
  }
  if (!fdc2 && lastFDC2) {
    lastFDC2 = false;
    webSocket.broadcastTXT("FDC2:0");
  }

  // ---- Broadcast chrono toutes les secondes (seulement si en cours) ----
  if (millis() - time_loop >= 50) {
    time_loop = millis(); // ← mise à jour indispensable

    if (chronoRunning1) {
      chronoValue1 = millis() - chronoStart;
      webSocket.broadcastTXT("Chrono_1:" + String(chronoValue1));
    }
    if (chronoRunning2) {
      chronoValue2 = millis() - chronoStart;
      webSocket.broadcastTXT("Chrono_2:" + String(chronoValue2));
    }
/*
    Serial.println("--- État ---");
    Serial.println("FDC1: " + String(fdc1) + " | last: " + String(lastFDC1) + " | running: " + String(chronoRunning1));
    Serial.println("FDC2: " + String(fdc2) + " | last: " + String(lastFDC2) + " | running: " + String(chronoRunning2));
  */
  }
}
