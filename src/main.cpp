
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

//PRO SERVO
Servo myservo;
int pos = 0;    
int servoPin = 18;

int flagAntesDepoisLogin = 1;    //ANTES DO LOGIN (AP) E DEPOIS

// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

//MQTT ADICOES
const char* mqtt_server = "broker.emqx.io";   
WiFiClient espClient;
PubSubClient clientMQTT(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;


String meuLogin(String texto)
{
  int indexInicialLogin = texto.indexOf("login=")+6; //+6 é o tamanho de 'login='
  if (texto.indexOf("login=") == -1) return "nenhum";
  int indexEComercial = texto.indexOf("&senha");
  int indexInicialsenha = indexEComercial + 7; //+7 é o tamanho de '&senha='
  int indexFinalizador = texto.indexOf("&finalizador");
  return texto.substring(indexInicialLogin, indexEComercial);
}
String minhaSenha(String texto)
{
  int indexInicialLogin = texto.indexOf("login=")+6; //+6 é o tamanho de 'login='
  int indexEComercial = texto.indexOf("&senha");
  int indexInicialsenha = indexEComercial + 7; //+7 é o tamanho de '&senha='
  int indexFinalizador = texto.indexOf("&finalizador");
  return texto.substring(indexInicialsenha, indexFinalizador);
}

void setup_wifi(const String ssid, const String password) 
{
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
  
  flagAntesDepoisLogin = 2; //COMECA O MQTT
}

void servo180Graus()
{
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  } 
}
void servo0Graus()
{
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  }
}
//CALLBACK DO MQTT
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 0 was received as first character
  if ((char)payload[0] == '0') {
    if (pos >= 180) servo0Graus(); //FOR ACABA LEVANDO pos PARA 181
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else if ((char)payload[0] == '1') {
    if (pos <= 0) servo180Graus(); //FOR ACABA LEVANDO pos PARA -1
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  } else {
    Serial.println("Não recebi nem 0 nem 1");
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (clientMQTT.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");   //TIREI... NAO SEI SE PRECISA SE INSCREVER
      // ... and resubscribe
      clientMQTT.subscribe("terroso/mensagens");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  //PRO SERVO
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object

  //MQTT ADICOES
  clientMQTT.setServer(mqtt_server, 1883);
  clientMQTT.setCallback(callback);

  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

void loop(){
  if (flagAntesDepoisLogin == 1)
  {
    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {                             // If a new client connects,
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              
              if (meuLogin(header) != "nenhum")
              {
                Serial.println("teste LEOOOO");
                Serial.println("login:");
                Serial.println(    meuLogin(header).length()  );
                Serial.println("senha:");
                Serial.println(    minhaSenha(header).length()   );
              }

              // turns the GPIOs on and off
              if (header.indexOf("GET /26/on") >= 0) {
                Serial.println("GPIO 26 on");
                output26State = "on";
                digitalWrite(output26, HIGH);
              } else if (header.indexOf("GET /26/off") >= 0) {
                Serial.println("GPIO 26 off");
                output26State = "off";
                digitalWrite(output26, LOW);
              } else if (header.indexOf("GET /27/on") >= 0) {
                Serial.println("GPIO 27 on");
                output27State = "on";
                digitalWrite(output27, HIGH);
              } else if (header.indexOf("GET /27/off") >= 0) {
                Serial.println("GPIO 27 off");
                output27State = "off";
                digitalWrite(output27, LOW);
              }
              
              client.println("<!DOCTYPE html><html>");
              client.println("<html><body><fieldset style='display: inline-block'>");
              client.println("<legend>Login</legend>");
              client.println("<form action='' method='get' target='_blank'>");
              client.println("<label for='login'>Login:</label>");
              client.println("<input type='text' id='login' name='login'><br><br>");
              client.println("<label for='senha'>Senha:</label>");
              client.println("<input type='password' id='senha' name='senha'><br><br>");
              client.println("<input type='text' id='finalizador' name='finalizador' style='display:none'><br><br>");
              client.println("<input type='submit' value='LOGIN'>");
              client.println("</form></fieldset></body></html>");
              // The HTTP response ends with another blank line
              client.println();

              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }

      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");

      if (meuLogin(header) != "nenhum")
      {
        setup_wifi(meuLogin(header), minhaSenha(header));
      }

      // Clear the header variable
      header = "";
    }

  }

  else if (flagAntesDepoisLogin == 2)
  {
    //MQTT ADICOES
    if (!clientMQTT.connected()) {
      reconnect();
    }
    clientMQTT.loop();
  }
  
}