#define BUILTIN_LED 2

#include <WiFi.h>
#include <PubSubClient.h>

#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"


// Update these with values suitable for your network.

const char* ssid = "LEO308_2G";
const char* password = "14393018";

const char* mqtt_server = "broker.emqx.io";

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String processor(const String& var)
{
  Serial.println(var);
  if(var == "STATE")
  {
    ledState = "MACARRAO";
    Serial.println("testano");
    return ledState;
  }
  return String();
}

//void setup_wifi(const char* ssid, const char* password) {
void setup_wifi(const String ssid, const String password) {


  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi(ssid, password);

  //client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);


  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  // Route to set GPIO to HIGH
  server.on("/retorno", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH); 
    AsyncWebParameter* l = request->getParam(0); //LOGIN
    AsyncWebParameter* s = request->getParam(1); //SENHA
    if (((l->value() == "hugo") && (s->value() == "1234"))  ||
       ((l->value() == "mikael") && (s->value() == "2025")))  request->send(SPIFFS, "/sucessologin.html", String(), false, processor); 
    else request->send(SPIFFS, "/falhalogin.html", String(), false, processor); 
    
    
    WiFi.disconnect();
    while (WiFi.status() != WL_DISCONNECTED) 
    {
      delay(500);
      Serial.print("d...");
    }
    Serial.println("Status:");
    Serial.println(WiFi.status());
    setup_wifi(l->value(), s->value());

  });
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });  

  // Start server
  server.begin();

}

void loop() {

/*
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
  }
*/
}