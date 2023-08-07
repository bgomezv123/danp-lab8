/*
  Proyecto IoT
Equipo:
--Gomez Velaco Brian Jospeh
--Jacobo Castillo Andrew Pold 
--Chuctaya Ruiz Diego Moises
--Ramos Ticona Gilbert
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>





// Valores para la conexion WiFi
const char* ssid = "GOMEZ 2,4G";
const char* password = "holathiago123456789";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
//AWS este valor se optiene del portal de AWS

const int pinLDR = A0;
bool onOff = true;
int MODE_LED = 1;
int R=50;
int G=50;
int B=50;
 //MQTT broker ip (EndPoint obtenido del portal AWS)
const char * AWS_endpoint = "a3brjmyw4c304l-ats.iot.us-east-2.amazonaws.com";

//CallBack, se imprime en el Monitor Serie el mensaje recivido por el servicio Cloud

void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println(topic);
 /**
  payload[length] = '\0';

 
  String jsonString = String((char*)payload);
  DynamicJsonDocument doc(512);
  deserializeJson(doc, jsonString);
  int value1 = doc["on_off"];
  int value2 = doc["mode_led"];
  int r = doc["r"];
  int g = doc["g"];
  int b = doc["b"];
 
 // Si el valor del payload es 1 se enciende el led, de lo contrario se apaga
  if(strcmp(topic,"sensor/command")==0){
    if (value1 == 1) {
      onOff = true;
    } else if(value1 == 0) {
      onOff = false;  
    } 

    if(value2 == 1){
      MODE_LED = 1;
    }else if(value2 == 2){
      MODE_LED = 2;
      R=r;
      G=g;
      B=b;
    }

  
  }
  **/
 
}

WiFiClientSecure espClient;
//MQTT port 8883 - standard
PubSubClient client(AWS_endpoint, 8883, callback, espClient); 
long lastMsg = 0;
char msg[50];
int value = 0;
void setup_wifi() {
  delay(10);
  // Inicializamos Wifi
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}
void reconnect() {
  // Reconeccion si es necesario
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("SensorLab")) {
      Serial.println("connected");

      client.publish("mobile/mensajes", "Hola, somos Brian, Andrew , Diego y Gilbert!!!!!!!! ");
      
      client.subscribe("sensor/command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  // inicializamos el led del esp8266
  
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
  // Cargamos los certificados
  File cert = SPIFFS.open("/cert.der", "r"); //verifica
  if (!cert) {
    Serial.println("Failed to open cert file");
  } else
    Serial.println("Success to open cert file");
  delay(1000);
  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");
  // Cargamos llave privada
  File private_key = SPIFFS.open("/private.der", "r"); 
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  } else
    Serial.println("Success to open private cert file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
  // Cargamos CA 
  File ca = SPIFFS.open("/ca.der", "r"); 
  if (!ca) {
    Serial.println("Failed to open ca ");
  } else
    Serial.println("Success to open ca");
  delay(1000);
  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
}

unsigned long tiempoPrevio = 0; // Variable para almacenar el tiempo previo
unsigned long intervalo = 1000; 

int redpin = 13; // select the pin for the red LED
int greenpin = 12 ;// select the pin for the green LED
int bluepin = 14;


const int pinD7 = 13;
const String topicPublish = "mobile/mensajes";
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // Lee el valor analógico del sensor LDR
  int valorLDR = analogRead(pinLDR);
  client.loop();
  //De acuerdo al valor analogico del sensor LDR establecemos el nivel de luminosidad del LED

  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  
  
  String currentDate = String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear);
  String timeNow =  String(timeClient.getHours())+":"+String(timeClient.getMinutes())+":"+String(timeClient.getSeconds());

  if(MODE_LED == 1){
     if(onOff){
      analogWrite(redpin,valorLDR);
      analogWrite(greenpin,valorLDR);
      analogWrite(bluepin,valorLDR);
     }else{
      analogWrite(redpin,0);
      analogWrite(greenpin,0);
      analogWrite(bluepin,0);
     }
  }else if(MODE_LED ==2){
      analogWrite(redpin,R);
      analogWrite(greenpin,G);
      analogWrite(bluepin,B);
  }


  StaticJsonDocument<128> jsonDoc;
  // Obtenemos la fecha y hora actual
  jsonDoc["Timestamp"] = currentDate+", "+timeNow;
  jsonDoc["Value"] = valorLDR;
  jsonDoc["Unit"] = "lux";
  jsonDoc["Notes"] = "TEST";
  


  String jsonString;
  serializeJson(jsonDoc, jsonString);

   if (millis() - tiempoPrevio >= intervalo) {
      tiempoPrevio = millis();
      //Publicamos el mensaje del sensor en el servicio Cloud(AWS), con el topico llamado "outTopic"
      client.publish(topicPublish.c_str(), jsonString.c_str());
      //Imprimimos el mensaje en el monitor serie
      Serial.println(jsonString.c_str());
   }


   //client.print(String("GET ") + "fkcmxu52vb7naznafl2ciivoei0ipuea.lambda-url.us-east-1.on.aws" + " HTTP/1.1\r\n" +
   //            "Host: " + AWS_endpoint + "\r\n" +
   //            "Connection: close\r\n\r\n");


  //Recibimos el mensaje enviado por el servicio cloud (AWS, con el topico llamado "on_off"
  char recivedMsg = client.subscribe("sensor/command",1);
  Serial.println(recivedMsg);

}
