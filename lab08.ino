/*
Lab08
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
#include <TimeLib.h>

// Valores para la conexion WiFi
const char* ssid              = "GOMEZ 2,4G";
const char* password          = "holathiago123456789";
const int pinLDR              = A0; //Pin A0 que obtiene el valor analogico del sensor
//Como se usa un led RGB con 4 pines se establecen en variables los pines D5, D6, D7 donde R,G,B respectivamente
const int redpin              = 13; 
const int greenpin            = 12 ;
const int bluepin             = 14;
//Nombre del topico al cual publicar
const String topicPublish     = "mobile/mensajes";
const String topicsubscribe   = "sensor/command";
//Se establecen los valores para un rango de 30 segundos
unsigned long tiempoPrevio    = 0; // Variable para almacenar el tiempo previo
unsigned long intervalo       = 30000;//30000 milisegundos (30 segundos)
const long utcOffsetInSeconds = -18000; // UTC offset para Colombia (UTC-5, teniendo en cuenta horario de verano)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org",utcOffsetInSeconds); 
//MQTT broker ip, EndPoint obtenido del Lab08
const char * AWS_endpoint     = "a3brjmyw4c304l-ats.iot.us-east-2.amazonaws.com";

//CallBack, se imprime en el Monitor Serie el mensaje recivido por el servicio Cloud
void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Mensaje recibido : [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println(topic);
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


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // Lee el valor analógico del sensor LDR
  int valorLDR = analogRead(pinLDR);
  client.loop();

  // Obtenemos la fecha y hora actual del NTP 
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  
  //Concatenamos la fecha y hora actual en Strings
  String currentDate = String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear);
  String timeNow =  String(timeClient.getHours())+":"+String(timeClient.getMinutes())+":"+String(timeClient.getSeconds());

  //De acuerdo al valor analogico del sensor LDR establecemos el nivel de luminosidad del LED   
  analogWrite(redpin,valorLDR);
  analogWrite(greenpin,valorLDR);
  analogWrite(bluepin,valorLDR);
   
  //Creamos el objeto JSON para establecer los siguiente valores :
  StaticJsonDocument<128> jsonDoc;

  jsonDoc["Timestamp"] = currentDate+", "+timeNow; //Fecha y hora actual del NTP 
  jsonDoc["Value"] = valorLDR;
  jsonDoc["Unit"] = "lux";
  jsonDoc["Notes"] = "TEST";
  

  String jsonString;
  serializeJson(jsonDoc, jsonString);
   if (millis() - tiempoPrevio >= intervalo) {
      tiempoPrevio = millis();
      //Publicamos el mensaje del sensor en el servicio Cloud(AWS), con el topico llamado "mobile/mensajes"
      client.publish(topicPublish.c_str(), jsonString.c_str());
      //Imprimimos el mensaje en el monitor serie
      Serial.println(jsonString.c_str());

   }


    //Recibimos el mensaje enviado por el servicio cloud (AWS, con el topico llamado "sensor/command"
    char recivedMsg = client.subscribe(topicsubscribe.c_str(),1);
    //Imprimimos el mensaje en el monitor serie
    Serial.println(recivedMsg);
 
}
