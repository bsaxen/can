//=============================================
// File.......: graphHttpClient.c
// Date.......: 2020-10-23
// Author.....: Benny Saxen
// Description: 
//=============================================

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#ifndef STASSID
//#define STASSID "bridge"
//#define STAPSK  "1234"
#define STASSID "bridge"
#define STAPSK  "fghjfghjf"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "rdf.simuino.com";
const uint16_t port = 80;
String url = "/can_server.php";
String label = "httpClient";
int period = 30; // Sec

ESP8266WiFiMulti WiFiMulti;
int counter = 0;
String mac;

//=============================================
void setup() {
//=============================================
  Serial.begin(9600);

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  mac = WiFi.macAddress();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("MAC address: ");
  Serial.println(mac);
  mac.replace(":","-");

  delay(500);
}
int start = 0;
int end = 0;
char msg[256];
int ix = 0;
int msgLen = 0;

void executeMsg(String m,int n)
{
  Serial.println("------>");
  Serial.println(m);
  Serial.println("<------");
    counter += 1;
  if (counter > 999999)
  {
    counter = 1;
  }
  
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print(':');
  Serial.println(port);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(10000);
    return;
  }

  String url = "/can_server.php";
  url += "?id=";
  url += mac;
  
  url += "&data=";
  url += String(m);

  Serial.println(url);
  // This will send the request to the server
   client.print(String("GET ") + url + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
     if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
     }
     delay(5);
  }
  Serial.println("receiving from remote server");
  while (client.available()) {
    char ch = static_cast<char>(client.read());
    Serial.print(ch);
    delay(5);
  }
  Serial.println("closing connection");
  client.stop();
}

//=============================================
void loop() {
//=============================================

  if (Serial.available()) {
    char rec = Serial.read();
    //Serial.println(rec);

    if (rec == ':' || rec == '*')
    { 
      if (rec == ':')
      {
        start = 1;
        ix = 0;
      }

      if (rec == '*')
      {
        start = 0;
        msgLen = ix;
        executeMsg(String(msg),msgLen);
      }
    }

    if (start == 1 && rec != ':')
    {
      Serial.println(rec);
      msg[ix] = rec;
      ix += 1;
    }

  }

}
//=============================================
// End of File
//=============================================
