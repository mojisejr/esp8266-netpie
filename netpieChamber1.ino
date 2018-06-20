#include <ESP8266WiFi.h>
#include <MicroGear.h>
#include <SHT21.h>
#include <DHT.h>
#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>

const char* ssid = "n/a";
const char* pwd = "n/a";

#define DHTPIN D5 
#define FANPIN D8
#define HDPIN D7
#define BOARD "ESP8266"
#define APPID "CHAMBER1"
#define KEY "5QqAPKDLqPxJvmY"
#define SECRET "dMq7CWHtgQ98N51YBYQJWpEuN"

#define ALIAS "pieChamber1"
WiFiClient client;
int sv_fan1_pos = 5;
int sv_fan1_open = 120;
int sv_fan1_close = 5;
int timer = 0;
char str[200];
SHT21 SHT21;
DHT dht(DHTPIN, DHT22);
float canopy_temp;
float canopy_humid;
float air_temp;
float air_humid;
float VPD;
float temp_diff;

MicroGear microgear(client);
//state update
String m;
const char delimeter = '#';
String state = "1,0,0";
String vpdstate = "1";
String fanstate = "0";
String humidstate = "0";
float readBattLv(){
  int batt_value = analogRead(A0);
  float volt = 3.637*(float)batt_value/1023.00;
  float volt_pct = (100*volt)/3.637;
  return volt;
}
void _oled(char x, char y, String str){
  oled.setTextXY(x, y);
  oled.putString(str);
}
void VPD_function(){
  if(VPD < 0.4){
    FAN(HIGH);
    Humidifier(LOW);
    fanstate = "1";
    humidstate = "0";
    state = stateupdate(vpdstate, fanstate, humidstate);
    microgear.chat(BOARD,state);
  }else if(VPD > 0.4 && VPD < 0.8){
    FAN(LOW);
    Humidifier(LOW);
    fanstate = "0";
    humidstate = "0";
    state = stateupdate(vpdstate, fanstate, humidstate);
    microgear.chat(BOARD,state);
  }else if(VPD > 0.8){
    FAN(LOW);
    Humidifier(HIGH);
    fanstate = "0";
    humidstate = "1";
    state = stateupdate(vpdstate, fanstate, humidstate);
    microgear.chat(BOARD,state);
  }
}
void FAN(int pinValue){
  digitalWrite(FANPIN, pinValue);
}
void Humidifier(int pinValue){
  digitalWrite(HDPIN, pinValue);
}
String stateupdate(String v, String f, String h){
  return v + "," + f + "," + h;
}
void onMsghandler(char *topic, uint8_t *msg, unsigned int msglen){
  Serial.println("Incoming message -->");
  msg[msglen] = '\0';
  Serial.println((char*)msg);
  if(*(char*)msg == '?'){
    state = stateupdate(vpdstate, fanstate, humidstate);
    Serial.println("Initial State: " + state);
    microgear.chat(BOARD, state);
  }else{
    m = (char*)msg;
    if(m.length() > 0){
      if(m.indexOf(delimeter) != -1){
        String code = m.substring(0, m.indexOf(delimeter));
        String logic = m.substring(m.indexOf(delimeter)+1);
        if(code == "v"){
          if(logic == "1"){
            Serial.println("msg --> VPD is ON");
          }else if(logic == "0"){
            Serial.println("msg --> VPD is OFF");
          }
          vpdstate = logic;
        }
        if(code == "f" && vpdstate == "0"){
          if(logic ==  "1"){
            Serial.println("msg --> FAN is manually ON");
            FAN(HIGH);
            fanstate = logic;
          }else if(logic == "0"){
            Serial.println("msg --> FAN is manually OFF");
            FAN(LOW);
            fanstate = logic;
          }
        }else if(code == "f" && vpdstate =="1"){
          Serial.println("msg--> FAN State not change manually in VPD mode");
        }
        if(code == "h" && vpdstate == "0"){
          if(logic == "1"){
            Serial.println("msg --> Humidifier is manually ON");
            Humidifier(HIGH);
            humidstate = logic;
          }else if(logic == "0"){
            Serial.println("msg --> Humidifier is manually OFF");
            Humidifier(LOW);
            humidstate = logic;
          }
        }else if(code == "h" && vpdstate =="1"){
          Serial.println("msg--> FAN State not change manually in VPD mode");
        }
        state = stateupdate(vpdstate,fanstate,humidstate);
        microgear.chat(BOARD, state);
        Serial.println("msg--> Board Status is updated");
      }
    }
  }
}
void onConnected(char *attribute, uint8_t *msg, unsigned int msglen){
  Serial.println("connected to NETPIE...");
  microgear.setAlias(ALIAS);
}
void setup() {
  microgear.on(MESSAGE, onMsghandler);
  microgear.on(CONNECTED, onConnected);
  Wire.begin();
  SHT21.begin();
  Serial.begin(115200);
  oled.init();
  oled.clearDisplay();
  pinMode(FANPIN, OUTPUT);
  pinMode(HDPIN, OUTPUT);
  Serial.println("Starting...");
  // put your setup code here, to run once:
  if(WiFi.begin(ssid, pwd)){
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
    }
  }
  
  Serial.println("WiFi Connected...");
  Serial.println("IP Address: ");
  Serial.println(String(WiFi.localIP()));
  _oled(1,1, "WiFi Connected...");
  _oled(2,1, "IP Address: ");
  _oled(3,1, String(WiFi.localIP()));
  microgear.init(KEY,SECRET, ALIAS);
  microgear.connect(APPID);
  oled.clearDisplay();
}

void loop() {
  if(microgear.connected()){
    Serial.println("...");
    microgear.loop();

    canopy_temp = SHT21.getTemperature();
    canopy_humid = SHT21.getHumidity();
    air_temp = dht.readTemperature();
    air_humid = dht.readHumidity();
    
    if(timer >= 1000){
      if(!isnan(air_temp) || !isnan(air_humid)){
        Serial.println("OLED Displaying");
        _oled(1,1, "c-temp:" + String(canopy_temp) + " C");
        _oled(2,1, "c-RH:  " + String(canopy_humid) + " %");
        _oled(3,1, "a-temp:" + String(air_temp) + " C");
        _oled(4,1, "a-RH:  " + String(air_humid) + " %");
        _oled(5,1, "VPD:   " + String(VPD) + " kPa");
        _oled(6,1, "F/H:   " + fanstate + "/" + humidstate);
        float VP_sat = 610.7 * pow(10,(7.5*canopy_temp)/(237.3+canopy_temp))/1000;
        float VP_air = (610.7 * pow(10,(7.5*air_temp)/(237.3+air_temp))/1000)*air_humid/100;
        VPD = VP_sat - VP_air;
        temp_diff = canopy_temp - air_temp;
        Serial.print("Sending -->");
        microgear.publish("/canopy_temp", canopy_temp,2);
        microgear.publish("/canopy_humid", canopy_humid, 2);
        microgear.publish("/air_temp", air_temp, 2);
        microgear.publish("/air_humid", air_humid, 2);
        microgear.publish("/vpd",VPD,2);
        microgear.publish("/temp_diff", temp_diff,2);
        //microgear.publish("/batt", readBattLv(),2);
        microgear.writeFeed("chamber1ctemp","{temp:" + String(canopy_temp) +",air_temp:" + String(air_temp) + ",canopy_humid:" + String(canopy_humid) + ",air_humid:" + 
        String(air_humid) + ",vpd:" + String(VPD) +",temp_diff:" + String(temp_diff) + "}");
      }
      if(vpdstate == "1"){
        VPD_function();
      }
      timer = 0;
    }else timer += 100;
  }else{
    Serial.println("connection lost, reconnect...");
    if(timer >= 5000){
      microgear.connect(APPID);
      timer = 0;
    }else timer += 100;
  }
  delay(100);
}

