// Estação Metereológica - Versao 2S/2024

// Credenciais do Blynk 
#define BLYNK_TEMPLATE_ID      "TMPL2Wp8TZBUN"
#define BLYNK_TEMPLATE_NAME    "INSTRUMENTAÇÃO ELETRÔNICA"
#define BLYNK_AUTH_TOKEN       "73vm3Df-Si17tug1PY_zlNRPjilNxL8j"
#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;

// Inclui bibliotecas
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <DHT.h>
#include <AS5600.h>
#include <SimpleTimer.h>

//Configuracao do Wi-Fi
#define WIFI_SSID         "2G_CLAROVIRTUA401"    
#define WIFI_PASS         "21010712"

//Define do pino do ESP para o sensor DHT11 (Umidade)
#define DHT_DATA_PIN 32
#define DHTTYPE DHT11
DHT dht(DHT_DATA_PIN, DHTTYPE);

// Define o pino do encoder (Rotacao)
#define EncPIN 35

// Sensor de pressao e temperatura (BMP280)
Adafruit_BMP280 bmp;

// Sensor de luminosidade (BH1750)
BH1750 lightMeter;

// Sensor de direcao do vento (Efeito Hall AS5600)
AS5600 as5600; 

// Define os objetos timer para acionar as interrupções
SimpleTimer TimerLeituras; // Define o intervalo entre leituras
SimpleTimer TimerEncoder; // Define intervalo para ler contador

// Define a variável para armazenar o intervalo de amostragem (em milisegundos)
int IntLeituras = 500; //300000; // Faz uma leitura dos sensores a cada 5 min
int IntEncoder = 500; // Faz a totalizacao dos pulsos a cada 0,5 segundos

// Define as variáveis para armazenar os valores lidos
int Umidade = 0;
int Luminosidade = 0;
int Pressao = 0;
float Temperatura = 0.0;
int Velocidade = 0;
int Rotacao = 0;
float Direcao = 0;

float lastTemperatura = 0.0;
int lastUmidade = 0;
int lastPressao = 0;
int lastRotacao = 0;
int lastLuminosidade = 0;
float lastDirecao = 0.0;
int lastTime = 0;

// Variável para ler o status do encoder
bool EncSt = LOW;
bool EncAnt = LOW;

// Variável para armazenar a contagem de pulsos
int Cont = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Configura o pino do encoder como entrada
  pinMode(EncPIN, INPUT);

  // Configura a interrupção para ser acionada a cada intervalo de amostragem
  TimerEncoder.setInterval(IntEncoder, ContaPulsos);  

  wifiConnect();

  // Inicia DHT11
  dht.begin();

  // Inicia BH1750
  lightMeter.begin();

  // Inicia BMP280
  bmp.begin(0x76);

  // Inicia AS5600
  as5600.begin(4);  //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  // default, just be explicit.

  // Mensagem de conexão do Blynk
  Blynk.begin(auth,WIFI_SSID,WIFI_PASS);
}

void loop() {
  int actualTime = millis();

  int tempo = millis() - lastTime;
   
  if  (tempo >= IntLeituras) {
    lastTime = millis();
    RunLeituras();
    if (mudouTemperatura()) {
    writeTemperatura();
    lastTemperatura = Temperatura;  // Atualiza o valor anterior
  }

  // Verifica e envia os dados de umidade
  if (mudouUmidade()) {
    writeUmidade();
    lastUmidade = Umidade;  // Atualiza o valor anterior
  }

  // Verifica e envia os dados de pressão
  if (mudouPressao()) {
    writePressao();
    lastPressao = Pressao;  // Atualiza o valor anterior
  }

  // Verifica e envia os dados de luminosidade
  if (mudouLuminosidade()) {
    writeLuminosidade();
    lastLuminosidade = Luminosidade;  // Atualiza o valor anterior
  }

  // Verifica e envia os dados de direção
  if (mudouDirecao()) {
    writeRotacaoDirecao();
    lastDirecao = Direcao;  // Atualiza o valor anterior
  }

  // Verifica e envia os dados de rotação
  if (mudouRotacao()) {
    writeRotacaoDirecao();
    lastRotacao = Rotacao;  // Atualiza o valor anterior
  }

    
  }
  // Executa as tarefas programadas pelo timer
  TimerEncoder.run();
  Blynk.run();  

  // Realiza a contagem de pulsos 
  EncSt = digitalRead(EncPIN);
  if (EncSt == HIGH && EncAnt == LOW) {
    Cont++;
    EncAnt = HIGH;
  }
  if (EncSt == LOW && EncAnt == HIGH) {
    EncAnt = LOW;
  }
}

void RunLeituras() {

  Umidade = dht.readHumidity();
  Luminosidade = lightMeter.readLightLevel();
  Pressao = bmp.readPressure()/100;
  Temperatura = bmp.readTemperature();

  // Conversao para graus e ajuste do zero
  Direcao = abs((as5600.rawAngle()*360.0/4095.0));

  Serial.print("Temperatura: ");
  Serial.print(Temperatura);
  Serial.println(" ºC"); 
  Serial.print("Umidade: ");
  Serial.print(Umidade);
  Serial.println(" % " );
  Serial.print("Pressão: ");
  Serial.print(Pressao);
  Serial.println(" hPa " );
  Serial.print("Luminosidade: ");
  Serial.print(Luminosidade);
  Serial.println(" lux  "); 
  Serial.print("Rotacao: ");
  Serial.print(Rotacao);
  Serial.println(" rpm  "); 
  Serial.print("Direcao: ");
  Serial.print(Direcao);
  Serial.println(" graus  "); 
}  

// Rotina para integralizar os pulsos e converter para rpm
void ContaPulsos() {
  Rotacao = ((Cont * 6) ) * 1000.00 / IntEncoder;
  Cont = 0;    
}

void wifiConnect(){
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startTime = millis();
  
  // Tenta conectar ao Wi-Fi com um limite de tempo de 1 minuto
  while (WiFi.status() != WL_CONNECTED) {
    // Verifica se já se passaram 60 segundos
    if (millis() - startTime >= 60000) {
      Serial.println("\n[WIFI] Timeout! Falha ao conectar.");
      WiFi.disconnect();
      return; // Cancela a tentativa de conexão
    }
    Serial.print(".");
    delay(5000); // Aguarda 5 segundos antes de tentar novamente
  }
  
  Serial.println("\n[WIFI] Conectado com sucesso!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

}

void writeRotacaoDirecao(){
  Blynk.virtualWrite(V4, Rotacao);
  Blynk.virtualWrite(V5, Direcao);
}


void writeTemperatura(){
  Blynk.virtualWrite(V0, Temperatura);
}

void writeUmidade(){
  Blynk.virtualWrite(V1, Umidade);
}

void writeLuminosidade(){
 Blynk.virtualWrite(V3, Luminosidade);
}

void writePressao(){
 Blynk.virtualWrite(V2, Pressao);
}

bool mudouTemperatura() {
  return abs(Temperatura - lastTemperatura) > 0.5;  // mudança mínima de 0.5°C
}

bool mudouUmidade() {
  return abs(Umidade - lastUmidade) > 2;  // mudança mínima de 2%
}

bool mudouPressao() {
  return abs(Pressao - lastPressao) > 1;  //mudança mínima de 1 hPa
}

bool mudouLuminosidade() {
  return abs(Luminosidade - lastLuminosidade) > 10;  //mudança mínima de 10 lux
}

bool mudouDirecao() {
  return abs(Direcao - lastDirecao) > 1.0;  // mudança mínima de 1 grau
}


bool mudouRotacao() {
  return abs(Rotacao - lastRotacao) > 10;  // mudança mínima de 10 rpm
}