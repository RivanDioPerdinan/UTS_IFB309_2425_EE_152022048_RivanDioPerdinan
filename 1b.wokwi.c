#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

const char* ssid = "Wokwi-GUEST";
const char* pass = "";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "dio";

const char* topic_suhu = "152022048_Suhu";
const char* topic_kelembaban = "152022048_Kelembapan";
const char* topic_relay = "152022048_Relay";
const char* topic_buzzer = "152022048_Buzzer";

#define DHTPIN 4   
#define DHTTYPE DHT22    

#define LED_RED_PIN 12    
#define LED_YELLOW_PIN 13 
#define LED_GREEN_PIN 5   
#define BUZZER_PIN 16    
#define RELAY_PIN 17         

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nTerhubung ke WiFi");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Terhubung ke MQTT");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  dht.begin();

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
}

void controlDevices(float temperature) {
  bool relayState = LOW;

  if (temperature > 35) {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Buzzer aktif: Suhu terlalu tinggi!");
    client.publish(topic_buzzer, "BUZZER AKTIF!");
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
    relayState = HIGH;
  } else if (temperature >= 30) {
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    client.publish(topic_buzzer, "BUZZER NONAKTIF");
  } else {
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    client.publish(topic_buzzer, "BUZZER NONAKTIF");
  }

  // Update relay state (pump status)
  digitalWrite(RELAY_PIN, relayState);

  // Print and publish the pump (relay) status
  if (relayState == HIGH) {
    Serial.println("Pompa: ON");
    client.publish(topic_relay, "ON");
  } else {
    Serial.println("Pompa: OFF");
    client.publish(topic_relay, "OFF");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembaban)) {
    Serial.println("Gagal membaca sensor!");
    return;
  }

  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print(" C\t");
  Serial.print("Kelembaban: ");
  Serial.print(kelembaban);
  Serial.println(" %");

  client.publish(topic_suhu, String(suhu).c_str());
  client.publish(topic_kelembaban, String(kelembaban).c_str());

  controlDevices(suhu);

  delay(2000);
}
