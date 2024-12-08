#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>  // Biblioteca para armazenamento não volátil
#include <WiFi.h>
#include <PubSubClient.h>

Preferences preferences;

// Funções para o Wi-Fi
String ssid;
String password;
String comando;
String ID1;
String ID2;
String ID3;
String ID4;
String ID5;
int auxId;  // Para manter o controle de onde salvar o próximo ID

// Função para conectar ao Wi-Fi
void connectToWiFi() {
  Serial.println();
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  int timeout = 10;  // Timeout de 10 segundos
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Wi-Fi conectado");
    Serial.println("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha na conexão. Verifique o SSID e a senha.");
  }
}

void enterNewCredentials() {
  String newSSID, newPassword;

  // Solicita o SSID
  Serial.println("Digite o SSID do Wi-Fi:");
  while (newSSID.length() == 0) {
    while (Serial.available() == 0) {
      // Aguarda a entrada pelo serial
    }
    newSSID = Serial.readStringUntil('\n');
    newSSID.trim();  // Remove espaços extras
  }

  // Solicita a senha
  Serial.println("Digite a senha do Wi-Fi:");
  while (newPassword.length() == 0) {
    while (Serial.available() == 0) {
      // Aguarda a entrada pelo serial
    }
    newPassword = Serial.readStringUntil('\n');
    newPassword.trim();  // Remove espaços extras
  }

  // Salva as credenciais na memória flash
  preferences.putString("ssid", newSSID);
  preferences.putString("password", newPassword);

  // Atualiza as credenciais e tenta conectar
  ssid = newSSID;
  password = newPassword;
  connectToWiFi();
}

WiFiClient wifiClient;  // Cliente seguro para SSL/TLS
PubSubClient mqttClient(wifiClient);

// Configuração do HiveMQ
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;  // Porta Padrão
//const char* mqtt_user = "";
//const char* mqtt_password = "";

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  Serial.print("Mensagem: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

void setupMqtt() {
  if (WiFi.status() == WL_CONNECTED) { // Verifica se está conectado ao Wi-Fi
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    // Conectando ao broker MQTT
    Serial.print("Conectando ao MQTT...");
    while (!mqttClient.connected()) {
      if (mqttClient.connect("ESP32Client")) {
        Serial.println("Conectado!");
      } else {
        Serial.print("Falha, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" Tentando novamente em 5 segundos...");
        delay(5000);
      }
    }
  } else {
    Serial.println("Erro: Wi-Fi não conectado. Não é possível conectar ao MQTT.");
  }
}


void publishToMqtt(const String& topic, const String& payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic.c_str(), payload.c_str());
    Serial.println("Mensagem publicada!");
  } else {
    Serial.println("Erro: Não conectado ao servidor MQTT.");
  }
}

// Definiremos o id que sera liberado o acesso
#define ID "76 18 82 C1 9B C7 D9"

// Define alguns pinos do esp32 que serão conectados aos módulos e componentes
#define LedVerde 32
#define LedVermelho 25
#define tranca 33
#define buzzer 4
#define SS_PIN 5
#define RST_PIN 15

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Define os pinos de controle do módulo de leitura de cartões RFID
LiquidCrystal_I2C lcd(0x27, 16, 2); // Define informações do LCD como o endereço I2C (0x27) e tamanho do mesmo

void setup() {
  SPI.begin(); // Inicia a comunicação SPI que será usada para comunicação com o módulo RFID

  // Inicializando o objeto Preferences
  preferences.begin("wifi-creds", false);  // Nome do namespace para as preferências

  
  
  // Recuperando o SSID e a senha salvos
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  // Recuperando ID's salvos
  ID1 = preferences.getString("id1", "");
  ID2 = preferences.getString("id2", "");
  ID3 = preferences.getString("id3", "");
  ID4 = preferences.getString("id4", "");
  ID5 = preferences.getString("id5", "");
  auxId = preferences.getInt("auxId", 0);

  Serial.begin(115200); // Inicia a comunicação serial com o computador na velocidade de 115200 baud rate

  // Verifica se o SSID e a senha estão salvos
  if (ssid.length() > 0 && password.length() > 0) {
    Serial.println("Credenciais encontradas na memória:");
    Serial.print("SSID: ");
    Serial.println(ssid);
    connectToWiFi();
  } else {
    Serial.println("Nenhuma credencial encontrada. Insira os dados pelo Monitor Serial.");
    enterNewCredentials();
  }
  //setupMqtt();

  lcd.init(); // Inicia o LCD
  lcd.backlight();
  mfrc522.PCD_Init(); // Inicia o módulo RFID

  Serial.println("RFID + ESP32");
  Serial.println("Passe alguma tag RFID para verificar o id da mesma.");

  // Define alguns pinos como saída
  pinMode(LedVerde, OUTPUT);
  pinMode(LedVermelho, OUTPUT);
  pinMode(tranca, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {
  // Verifica se há um comando vindo do Serial
  if (Serial.available() > 0) {
    comando = Serial.readString();
    comando.trim();
    if (comando == "novo") {
      enterNewCredentials();
    }
  }

  lcd.home();               // Posiciona o cursor no início do LCD
  lcd.print("Aguardando");  // Exibe "Aguardando" no LCD
  lcd.setCursor(0, 1);      // Move o cursor para a segunda linha
  lcd.print("Leitura RFID"); // Exibe "Leitura RFID"

  // Verifica se um novo cartão está presente
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return; // Se não houver cartão, retorna e continua aguardando
  }

  // Tenta ler o cartão
  if (!mfrc522.PICC_ReadCardSerial()) {
    return; // Se a leitura falhar, retorna e continua aguardando
  }

  // Um cartão foi detectado e lido
  String conteudo = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();

  Serial.println("ID da tag: " + conteudo);

  // Verifica a conexão ao Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    connectToWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Falha ao reconectar ao Wi-Fi. Aguardando...");
      delay(5000);
      return; // Sai do loop para aguardar outra tentativa
    }
  }

  // Verifica e reconecta ao servidor MQTT
  if (!mqttClient.connected()) {
    Serial.println("MQTT desconectado. Tentando reconectar...");
    setupMqtt();
    if (!mqttClient.connected()) {
      Serial.println("Falha ao reconectar ao MQTT. Aguardando...");
      delay(1000);
      return; // Sai do loop para aguardar outra tentativa
    }
  }

  // Publica o ID do cartão no servidor MQTT
  publishToMqtt("esp/id443", conteudo.substring(1));

// Salva o próximo ID de cartão lido
if (conteudo.substring(1) == "80 B7 24 14") {
    Serial.println("Salvando o próximo cartão lido...");
    
    // Aguarda o próximo cartão
    while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        // Espera até que um novo cartão seja detectado e lido
        delay(100);
    }

    // Após detectar e ler um novo cartão, processa o conteúdo do ID
    String novoConteudo = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        novoConteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        novoConteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    novoConteudo.toUpperCase();

    // Verifica se auxId é 0 para resetar os IDs
    if (auxId == 0) {
        preferences.putString("id1", "");
        preferences.putString("id2", "");
        preferences.putString("id3", "");
        preferences.putString("id4", "");
        preferences.putString("id5", "");
        Serial.println("Memória dos IDs foi zerada.");
    }

    // Salva o novo ID de acordo com o valor atual de auxId
    if (auxId == 0) preferences.putString("id1", novoConteudo);
    else if (auxId == 1) preferences.putString("id2", novoConteudo);
    else if (auxId == 2) preferences.putString("id3", novoConteudo);
    else if (auxId == 3) preferences.putString("id4", novoConteudo);
    else if (auxId == 4) preferences.putString("id5", novoConteudo);

    // Incrementa e salva auxId na faixa de 0 a 4
    auxId = (auxId + 1) % 5;
    preferences.putInt("auxId", auxId);

    // Exibe o novo valor de auxId no Monitor Serial
    Serial.println("Novo ID salvo: " + novoConteudo);
    Serial.println("Novo valor de auxId: " + String(auxId));
}
  // Lógica de controle de acesso
  if (conteudo == ID || conteudo == ID1 || conteudo == ID2 || conteudo == ID3 || conteudo == ID4 || conteudo == ID5) {
    digitalWrite(LedVerde, HIGH);
    lcd.clear();
    lcd.print("Acesso Liberado");
    digitalWrite(tranca, HIGH);
    delay(5000);
    digitalWrite(tranca, LOW);
    digitalWrite(LedVerde, LOW);
  } else {
    digitalWrite(LedVermelho, HIGH);
    lcd.clear();
    lcd.print("Acesso Negado");
    delay(5000);
    digitalWrite(LedVermelho, LOW);
  }
  Serial.println(ID1);
  Serial.println(ID2);
  Serial.println(ID3);
  Serial.println(ID4);
  Serial.println(ID5);
}
