#include <string.h>  // para strcmp

// Pinos
const int PIN_BOMBA   = 13; // relé da bomba
const int LED_BOMBA   = 12; // LED vermelho que indica bomba ligada
const int LED_SECO    = 5;  // LED vermelho do sensor (solo seco)
const int LED_MEDIO   = 4;  // LED amarelo do sensor (umidade média)
const int LED_UMIDO   = 3;  // LED verde do sensor (solo úmido)
const int PIN_SENSOR  = A0; // entrada analógica do sensor
const int PIN_BTN     = 6;  // botão físico (push button)

// Limites (%)
const float LIMITE_SECO  = 35.0;
const float LIMITE_MEDIO = 70.0;

bool  modoAuto     = false; // começa em Modo 1 (Manual)
float umidadeSuave = 0.0;
bool  bombaOn      = false;
bool  lastBtn      = HIGH; // estado anterior do botão (pull-up interno)

// ---------------------------------------------------

void setPump(bool on) {
  digitalWrite(PIN_BOMBA, on ? HIGH : LOW);
  digitalWrite(LED_BOMBA, on ? HIGH : LOW); // LED da bomba segue a bomba
  bombaOn = on;
}

void setNivelLeds(const char* nivel) {
  if (strcmp(nivel, "seco") == 0) {
    digitalWrite(LED_SECO,  HIGH);
    digitalWrite(LED_MEDIO, LOW);
    digitalWrite(LED_UMIDO, LOW);
  }
  else if (strcmp(nivel, "medio") == 0) {
    digitalWrite(LED_SECO,  LOW);
    digitalWrite(LED_MEDIO, HIGH);
    digitalWrite(LED_UMIDO, LOW);
  }
  else { // "umido"
    digitalWrite(LED_SECO,  LOW);
    digitalWrite(LED_MEDIO, LOW);
    digitalWrite(LED_UMIDO, HIGH);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_BOMBA,  OUTPUT);
  pinMode(LED_BOMBA,  OUTPUT);
  pinMode(LED_SECO,   OUTPUT);
  pinMode(LED_MEDIO,  OUTPUT);
  pinMode(LED_UMIDO,  OUTPUT);
  pinMode(PIN_BTN,    INPUT_PULLUP); // botão com resistor de pull-up interno

  setPump(false);
  setNivelLeds("umido"); // inicializa verde (úmido)
  delay(1000);
}

void loop() {
  // —— 1) Comandos Serial ——
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'A') {
      modoAuto = true;  // Modo 2 (sensor)
    } else if (c == 'M') {
      modoAuto = false; // Modo 1 (manual)
    } else if (!modoAuto) {
      // No Modo 1 (manual) aceita '1'/'0'
      if (c == '1') setPump(true);
      if (c == '0') setPump(false);
    }
  }

  // —— 2) Leitura do botão físico (apenas no Modo 1) ——
  bool btnState = digitalRead(PIN_BTN);
  if (!modoAuto && lastBtn == HIGH && btnState == LOW) {
    setPump(!bombaOn);   // alterna o estado da bomba
    delay(250);          // debounce simples
  }
  lastBtn = btnState;

  // —— 3) Leitura do sensor e cálculo da umidade (%) ——
  int leitura = analogRead(PIN_SENSOR);
  float umidadePercent = map(leitura, 1023, 0, 0, 100);
  umidadeSuave += (umidadePercent - umidadeSuave) * 0.03;

  // Classifica nível
  const char* nivel;
  if (umidadeSuave <= LIMITE_SECO)        nivel = "seco";
  else if (umidadeSuave <= LIMITE_MEDIO)  nivel = "medio";
  else                                    nivel = "umido";

  // LEDs de nível SEMPRE refletem a umidade atual
  setNivelLeds(nivel);

  // —— 4) Se Modo 2 (sensor), controla a bomba automaticamente ——
  if (modoAuto) {
    if (strcmp(nivel, "seco") == 0) setPump(true);
    else                            setPump(false);
  }

  // —— 5) Telemetria Serial para o Raspberry Pi ——
  Serial.print("UM:"); Serial.print(umidadeSuave, 1);
  Serial.print(";MD:"); Serial.print(modoAuto ? 'A' : 'M');
  Serial.print(";ST:"); Serial.print(nivel);
  Serial.print(";P:");  Serial.println(bombaOn ? 1 : 0);

  delay(500);
}
