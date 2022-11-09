//esp12_priz_18.10.2018
// eklenecekler :
// 1- cihazın şifresinin değiştirilmesi isteği için kod yazılacak. restfull olarak kullanıcı şifreyi değiştirebilecek
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <MD5.h>
#include<string>
#include<EEPROM.h>
#include <SocketIoClient.h>
// socetio değişkenleri
SocketIoClient webSocket;
// ESP Erişim Noktanıza SSID ve Şifre
//benzersiz İD tanımlandı
String UUID = "277f5b81-cedb-4cc0-a518-9022fe5ce496";
//String mac = WiFi.macAddress();
String mac;
char ssid[50];
char password[50];
//Modem modunda tanımlanacak ssid ve password burda tutulacak
char eepromModemSsid[50] = "";
char eepromModemPassword[50] = "";
String connectingPassword;
String connectingSSID;
int modemConnectingCounter = 0;
int role = 5;
int deviceStatus = 0;
MD5Builder md5;
String readingEepromSsid;
String readingEepromPassword;
bool lastState = false;
int serverPort = 44444;
String modemIp = "0";
boolean webSocketCanConnected = true;
int lastMillis = 0;
bool wifiConnected = false;
String connectedWifiSsid = "";
ESP8266WebServer httpServer(serverPort); //44444 numaralı bağlantı noktasında sunucu
void myEepromPassword() {



}
void eepromClear() { // EEPROM Sıfırlanma ve verileri temizleme fonksiyonu
  for (int i = 0; i < 32; i++) {
    EEPROM.write(i, 255);
  }
  for (int j = 0; j < 94; j++) {
    EEPROM.write(32 + j, 255);
  }
}
void handleSetWifi() {
  eepromClear();

  String connectingSSID = httpServer.arg("SSID");
  String connectingPassword = httpServer.arg("password");

  connectingSSID.toCharArray(eepromModemSsid, 50);
  connectingPassword.toCharArray(eepromModemPassword, 50);

  for (int i = 0; i < connectingSSID.length(); i++) {
    EEPROM.write(i, connectingSSID[i]);
    delay(50);
    Serial.print("Yazdirilan: ");
    Serial.println(connectingSSID[i]);
  }
  for (int j = 0; j < connectingPassword.length(); j++) {
    EEPROM.write(32 + j, connectingPassword[j]);
    delay(50);
    Serial.print("Yazdirilan: ");
    Serial.println(connectingPassword[j]);
  }
  EEPROM.commit();
  httpServer.send(200, "text/html", "");  
  modemConnect();
  
}
void handleStatus() {
  String deviceStatus2 = (deviceStatus == 0 ? "0" : "1");
  httpServer.send(200, "application/json", "{\"status\":\"" + deviceStatus2 + "\"}");
  webSocketCanConnected = false;
  lastMillis = millis() + 10000;
}
void handleRoot() {
  Serial.println("koke girildi");
  String mac = WiFi.softAPmacAddress();
  httpServer.send(200, "application/json", "{\"mac\":\"" + mac + "\"}"); //Web sayfayı yolla
  //webSocketCanConnected = false;
  //lastMillis = millis() + 10000;
}
void handleRoleOn() {
  digitalWrite(role, HIGH); //role is connected in reverse
  deviceStatus = 1;
  httpServer.send(200, "application/json", "{\"status\":\"1\"}"); //Send ADC value only to client ajax request
  updateMyStatusToCloud();
  //webSocketCanConnected = false;
  //lastMillis = millis() + 10000;
}
void handleRoleOff() {
  digitalWrite(role, LOW); //role off
  deviceStatus = 0;
  httpServer.send(200, "application/json", "{\"status\":\"0\"}"); //Send ADC value only to client ajax request
  updateMyStatusToCloud();
  //webSocketCanConnected = false;
  //lastMillis = millis() + 10000;
}
void handleGetIp() {
  Serial.println("local ip istegi geldi");
  String content = "{\"modemIp\":\"" + modemIp + "\",\"connectedSsid\":\"" + connectedWifiSsid + "\"}";
  Serial.println(content);
  httpServer.send(200, "application/json", content);
  Serial.println("gönderme işlemi bitti");
  webSocketCanConnected = false;
  lastMillis = millis() + 10000;
}
void handleStartWebSocket(){
  webSocketCanConnected = true;
  httpServer.send(200, "application/json","{}");
  webSocketCanConnected = false;
  lastMillis = millis() + 10000;
}
void handleStopWebSocket(){
  webSocketCanConnected  = false;
  httpServer.send(200, "application/json","{}");
  webSocketCanConnected = false;
  lastMillis = millis() + 10000;
}
void connectEvent(const char * payload, size_t length) {
  Serial.println("Socket connected");
  String uuidJson = "{\"uuid\":\"" + UUID + "\"}";
  webSocket.emit("loginPriz", uuidJson.c_str());
}
void disconnectEvent(const char * payload, size_t length) {
  Serial.println("Socket disconnected");
  Serial.println(payload);
}
void toggleRole(const char * payload, size_t length) {
  deviceStatus = (deviceStatus == 1 ? 0 : 1);
  digitalWrite(role, deviceStatus);
  updateMyStatusToCloud();
}
void handleRoleOnFromWebSocket(const char * payload, size_t length) {
  digitalWrite(role, HIGH); //role is connected in reverse 
  deviceStatus = 1;
  Serial.println("actik kardeeş");
}
void handleRoleOffFromWebSocket(const char * payload, size_t length) {
  digitalWrite(role, LOW); //role off
  deviceStatus = 0;
  Serial.println("kapattik kardeeş");
}
void loginEvent(const char * payload, size_t length) {
  Serial.println(payload);
  if (String(payload) == "1") {
    updateMyStatusToCloud();
  }
}
void updateMyStatusToCloud() {
  String updateMyStatusJson = "{\"currentStatus\":" + String(deviceStatus) + "}";
  webSocket.emit("updateMyStatus", updateMyStatusJson.c_str());

}
void modemConnect() {
  WiFi.disconnect(true);
  connectedWifiSsid = "";
  WiFi.begin(eepromModemSsid, eepromModemPassword);
  //WiFi.config();
  wifiConnected = false;
  while (WiFi.status() != WL_CONNECTED) {
    modemConnectingCounter++;
    delay(500);
    Serial.print(".");
    if (modemConnectingCounter == 20) {
      Serial.println("Modeme Baglanamadi !");
      Serial.print("Wifi.status() == ");
      Serial.println(WiFi.status()); //Wi-Fi bağlantısının durumunu döndür.
      modemConnectingCounter = 0;
      break;
    }//bağlanmazsa çıkış
  }
  if (WiFi.status() == 3) {
    wifiConnected = true;
  }
  if (wifiConnected) {
    Serial.print("Modemden Alinan IP: ");
    Serial.println(WiFi.localIP());
    connectedWifiSsid = eepromModemSsid;
    webSocketStarting();
  }
  else {
    Serial.println("Wifi ya bağlanamadı");
  }
  modemIp = WiFi.localIP().toString();
}
void webSocketStarting() {
  webSocket.on("toggleRole", toggleRole);
  webSocket.on("handleRoleOn", handleRoleOnFromWebSocket);
  webSocket.on("handleRoleOff", handleRoleOffFromWebSocket);
  webSocket.on("connect", connectEvent);
  webSocket.on("login", loginEvent);
  webSocket.on("disconnected", disconnectEvent);
  webSocket.begin("192.168.1.108", 82);
}
String calcMd5(String s) {
  md5.begin();
  md5.add(s);
  md5.calculate();
  return md5.toString();
}
void startLocalHotspot() {
  WiFi.mode(WIFI_AP);           //Sadece erişim noktası
  mac = WiFi.softAPmacAddress();
  String temp = calcMd5(UUID + mac).substring(0, 16);
  Serial.print("temp : ");
  Serial.println(temp);
  String temp2 = calcMd5(temp + UUID).substring(0, 16);
  String temp3 = calcMd5(UUID).substring(0, 8);
  temp2.toCharArray(ssid, 50);
  temp3.toCharArray(password, 50);
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("password : ");
  Serial.println(password);
  WiFi.softAP(ssid, password);  // HOTspot'u kaldırma şifresini kaldırarak güvenliği devre dışı bırakır
  IPAddress myIP = WiFi.softAPIP(); // IP adresini al
  Serial.print("Local IP: ");
  Serial.println(myIP);
}
void firstStart() {
  for (int k = 0; k < 32; ++k) {
    char a = char(EEPROM.read(k));
    delay(50);
    if (a == 255) {
      break;
    }
    readingEepromSsid += a;
  }
  for (int l = 0; l < 96; ++l) {
    char b = char(EEPROM.read(32 + l));
    delay(50);
    if (b == 255) {
      break;
    }
    readingEepromPassword += b;
  }
  Serial.println(readingEepromSsid);
  Serial.println(readingEepromPassword);
  readingEepromSsid.toCharArray(eepromModemSsid, 50);
  readingEepromPassword.toCharArray(eepromModemPassword,50);

  if (EEPROM.read(0) != 255 && EEPROM.read(32) != 255 ) {
    
    modemConnect();
    Serial.println("*********************");
    Serial.println(String(WiFi.status()));
    Serial.println("*********************");
  }
  if (WiFi.status() != 3  || WiFi.status() == 4) {
    startLocalHotspot();
    Serial.println("*********************");
    Serial.println(String(WiFi.status()));
    Serial.println("*********************");
    /*
      WL_NO_SHIELD = 255,   ---> WiFi kalkanı bulunmadığında atanır ;
      WL_IDLE_STATUS = 0,   --->WiFi .begin () çağrıldığında atanan
                geçici bir durumdur ve deneme sayısı sona erene kadar (WL_CONNECT_FAILED ile sonuçlanan)
                veya bir bağlantı kuruluncaya kadar etkin kalır (WL_CONNECTED ile sonuçlanır);
      WL_NO_SSID_AVAIL = 1  ---> SSID bulunmadığında atanır;
      WL_SCAN_COMPLETED = 2 ---> tarama ağları tamamlandığında atanır;
      WL_CONNECTED = 3      ---> bir WiFi ağına bağlandığında atanır ;
      WL_CONNECT_FAILED = 4 ---> tüm girişimler için bağlantı başarısız olduğunda atanır;
      WL_CONNECTION_LOST = 5---> bağlantı kaybolduğunda atanır;
      WL_DISCONNECTED = 6   ---> bir ağ bağlantısı kesildiğinde atanan;
    */
  }

}
void httpServerStarting() {
  String preLink = calcMd5(mac + UUID); 
  Serial.print("preLink : ");
  Serial.println(preLink);
  httpServer.on("/", handleRoot);
  httpServer.on("/" + preLink + "/getstatus/", handleStatus);
  httpServer.on("/" + preLink + "/socket1On/", handleRoleOn);
  httpServer.on("/" + preLink + "/socket1Off/", handleRoleOff);
  httpServer.on("/" + preLink + "/setModem/", handleSetWifi);
  httpServer.on("/" + preLink + "/getIp/", handleGetIp);
  httpServer.on("/" + preLink + "/startWebSocket/", handleStartWebSocket);
  httpServer.on("/" + preLink + "/stopWebSocket/", handleStopWebSocket);
  httpServer.begin();                  // Sunucu başlat
  //Serial.println("HTTP sunucu basladi");
}
void setup() {
  
  pinMode(role, OUTPUT);
  digitalWrite(role,LOW);
  Serial.begin(115200);
  EEPROM.begin(512);
  Serial.print("WiFi.status()== ");
  Serial.println(WiFi.status()); //Wi-Fi bağlantısının durumunu döndür.
  firstStart();

  mac = WiFi.softAPmacAddress();
  Serial.print("MAC: ");
  Serial.println(mac);
  Serial.print("UUID: ");
  Serial.println(UUID);
  Serial.println(calcMd5(mac + UUID));
  httpServerStarting();
}

void loop(void) {
  httpServer.handleClient();          // Müşteri isteklerini ele al
  
  // if (digitalRead(resetPin) == LOW) {  ESP.reset();  }
  
  if(millis() >= lastMillis){
      webSocketCanConnected = true;
    }
  if(webSocketCanConnected){
    
  }
  //webSocket.loop();
}
