#include <WiFi.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>
#include <FastLED.h>

/* ===================== CONFIGURAÇÃO DOS LEDS (IGUAL AO SNAKE) ===================== */
#define MATRIX_W 16
#define MATRIX_H 16
#define NUM_LEDS 256

#define LED_PIN 13
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
#define BRIGHTNESS 80

CRGB leds[NUM_LEDS];

/* ===================== CONFIGURAÇÃO DE REDE ===================== */
const char* ssid = "The-net";
const char* password = "12345678";

const char* cloudflare_host = "scanning-harley-peoples-washington.trycloudflare.com"; 
const int server_port = 443;

SocketIOclient socketIO;

/* ===================== VARIÁVEIS DO PONG ===================== */
#define PADDLE_HEIGHT 3

float p1_y = 6;
float p2_y = 6;

float ball_x = 8;
float ball_y = 8;
float ball_dx = 0.5;
float ball_dy = 0.3;

unsigned long lastGameUpdate = 0;
#define GAME_SPEED 50 

/* ===================== MAPEAMENTO SERPENTINO ===================== */
uint16_t getIndex(uint8_t x, uint8_t y) {
  if (x >= MATRIX_W || y >= MATRIX_H) return 0;
  if (y % 2 == 0) return y * MATRIX_W + x;
  return y * MATRIX_W + (MATRIX_W - 1 - x);
}

/* ===================== EVENTOS DO SOCKET.IO ===================== */
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_CONNECT:
      Serial.println("[ESP32] Conectado ao Servidor Cloudflare!");
      break;

    case sIOtype_EVENT: {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        String eventName = doc[0];
        
        if (eventName == "update_simulator" || eventName == "update_game") {
          JsonObject data = doc[1];
          String player = data["player"];
          float beta = data["beta"] | 0.0;

          float mappedY = map(constrain(beta, -45, 45), -45, 45, 13, 0);

          if (player == "Player 1") p1_y = mappedY;
          if (player == "Player 2") p2_y = mappedY;
        }
      }
      break;
    }

    default:
      break;
  }
}

/* ===================== FÍSICA DO JOGO ===================== */
void resetBall() {
  ball_x = 8;
  ball_y = 8;
  ball_dx = (random(0, 2) == 0 ? 0.5 : -0.5);
  ball_dy = (random(-3, 4) / 10.0);
}

void updatePong() {
  ball_x += ball_dx;
  ball_y += ball_dy;

  if (ball_y <= 0 || ball_y >= MATRIX_H - 1) {
    ball_dy *= -1;
    ball_y = constrain(ball_y, 0, MATRIX_H - 1);
  }

  if (ball_x <= 1) {
    if (ball_y >= p1_y && ball_y <= p1_y + PADDLE_HEIGHT) {
      ball_dx *= -1;
      ball_x = 1;
    } else if (ball_x <= 0) {
      resetBall();
    }
  }

  if (ball_x >= MATRIX_W - 2) {
    if (ball_y >= p2_y && ball_y <= p2_y + PADDLE_HEIGHT) {
      ball_dx *= -1;
      ball_x = MATRIX_W - 2;
    } else if (ball_x >= MATRIX_W - 1) {
      resetBall();
    }
  }
}

/* ===================== RENDERIZAÇÃO ===================== */
void drawPong() {
  FastLED.clear();

  // Raquete Player 1 (Azul)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    int y = (int)p1_y + i;
    if (y >= 0 && y < MATRIX_H) leds[getIndex(0, y)] = CRGB::Blue;
  }

  // Raquete Player 2 (Vermelha)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    int y = (int)p2_y + i;
    if (y >= 0 && y < MATRIX_H) leds[getIndex(MATRIX_W - 1, y)] = CRGB::Red;
  }

  // Bola (Branca)
  leds[getIndex((uint8_t)ball_x, (uint8_t)ball_y)] = CRGB::White;

  FastLED.show();
}

/* ===================== SETUP E LOOP ===================== */
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");

  socketIO.beginSSL(cloudflare_host, server_port, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);

  randomSeed(analogRead(0));
  resetBall();
}

void loop() {
  socketIO.loop();

  if (millis() - lastGameUpdate > GAME_SPEED) {
    lastGameUpdate = millis();
    updatePong();
    drawPong();
  }
}