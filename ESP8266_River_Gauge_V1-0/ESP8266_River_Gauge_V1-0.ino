#include <SoftwareSerial.h>           //for communication with gsm module
#include <NewPing.h>                  //for communication with sonar sensor

#define TRIGGER_PIN  14               // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     12               // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 600              // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define SENSOR_HEIGHT 300             // Hight of the sensor above the river bed

//Create software serial object to communicate with SIM800L
SoftwareSerial SIM800L(13, 15); //SIM800LL Tx & Rx is connected to Arduino #3 & #2
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

int sleepTime = 900E6;                //time in seconds*E6 between updates 
int resetSleepTime = 60E6;            //time in seconds*E6 after an upload failure (prevents significant battery drain)
const String APN  = "simbase";        //sim APN
String apiKey = "HAJQOQIJ6SIAXCU5";   //thingspeak api key
String levelField = "field1";
String batteryField = "field2";


int modulebattery;                    //battery level to send
int waterlevel;                       //water level to send
int tSuccess = 0;                     //changes to zero on successful data transmission to thingspeak
int dtrPin = 2;
int resetPin = 4;


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);       //led indicator light, low is on
  pinMode(dtrPin,OUTPUT);                  //dtr pin
  pinMode(resetPin,OUTPUT);      
  digitalWrite(resetPin,HIGH);
  digitalWrite(dtrPin,HIGH);
  
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  //Begin serial communication with Arduino and SIM800LL
  SIM800L.begin(9600);

  Serial.println("Initializing...");

  digitalWrite(dtrPin,LOW);      //dtr pin low
  delay(100);
  sendSerial("AT+CSCLK=0"); //wake up
  sendSerial("AT+CFUN=1");  //enable full functionality
  
  
  for(int i=0;i<10;i++){
    waterlevel=waterlevel+(SENSOR_HEIGHT-(sonar.ping_cm()));
    delay(100);   
  }
  waterlevel = waterlevel/10;

  
  Serial.println(waterlevel);
  digitalWrite(LED_BUILTIN, LOW);
  checkBattery();
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  waitForConnection();
  
  uploadData(waterlevel, modulebattery);

  goToSleep();  
}

//#####################################################################################

void loop()
{
  Serial.println("entered loop, error with sleep");
  goToSleep();
}


//#####################################################################################

void checkBattery(){
  Serial.println("Send ->: AT+CBC");
  while (SIM800L.available()){
    SIM800L.read();
  }

  int timeout = 0;
  
  SIM800L.println("AT+CBC");
  while(SIM800L.available()<30){ //wait for 30 characters to come in before moving on or else restart everything
    Serial.print(".");
    timeout = timeout + 1;
    if(timeout>=100){
      Serial.println();
      Serial.println("Timeout, Sleeping for 5 mins to reset");
      goToSleep();
    }
    delay(200);
  }
  Serial.println("Command Sent");

  char byteRead;
  char delim = ',';
  char C1;
  char C2;
  char C3;
  while(SIM800L.available())
  {
    byteRead = SIM800L.read();
    if(byteRead == delim){
      C1=SIM800L.read();
      if(C1 != delim){
        C2=SIM800L.read();
        if(C2 != delim){
          C3=SIM800L.read();
        }
        
      }
    }

  }
  Serial.println(C1);
  Serial.println(C2);
  Serial.println(C3);
  
  if(C1==','){
    modulebattery = 0;
  }else if(C2==','){
    modulebattery = C1-'0';
  }else if(C3==','){
    modulebattery = 10*(C1 - '0') + (C2-'0');
  }else{
    modulebattery = 100;
  }
  Serial.print("Battery Level: ");
  Serial.println(modulebattery);
}

//#####################################################################################

void waitForConnection(){
  Serial.println("Checking for Connection");
  Serial.println("Send ->: AT+CREG?");
  while (SIM800L.available()){
    SIM800L.read();
  }
  int simConnected =0;

  while(simConnected == 0){
    Serial.println("Checking");
    int timeout = 0;
  
    SIM800L.println("AT+CREG?");
    while(SIM800L.available()<5){ //wait for 30 characters to come in before moving on or else restart everything
      Serial.print(".");
      timeout = timeout + 1;
      if(timeout>=100){
        Serial.println();
        Serial.println("Timeout, Sleeping for 5 mins to reset");
        goToSleep();
      }
      delay(200);
    }
    Serial.println("Command Sent");

    char byteRead;
    while(SIM800L.available())
    {
      byteRead = SIM800L.read();
      Serial.print(byteRead);
      if(byteRead == '5' || byteRead == '1'){
        simConnected = 1;
      }
    }
    delay(2000);
  }
  Serial.println("Connection Successful");
}

//######################################################################################

void goToSleep(){
  Serial.println("#########################################################################");
  
  if(tSuccess==0){
    Serial.println("ERROR UPLOADING, Resetting sim card");
    sleepTime=resetSleepTime;
    digitalWrite(resetPin,LOW);
    delay(150);
    digitalWrite(resetPin,HIGH);
    delay(5000);
     
  }else{
    Serial.println("SUCCESSFUL TRANSMISSION, Sleeping for 15");
  }

  
  Serial.println("Going into Sleep");
  sendSerial("AT+CFUN=0");  //Need to send twice "+CPIN: NOT READY"
  sendSerial("AT+CFUN=0");
  digitalWrite(dtrPin,HIGH);
  sendSerial("AT+CSCLK=1");
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  ESP.deepSleep(sleepTime);
}

//#####################################################################################

void uploadData(int level, int battery){
  String url = "http://api.thingspeak.com/update?api_key="+apiKey+"&"+levelField+"="+level+"&"+batteryField+"="+battery;
  Serial.println(url);
 
  sendSerial("AT+CGATT=1");                     //attach to gprs service
  sendSerial("AT+SAPBR=3,1,Contype,\"GPRS\"");  //set gprs
  sendSerial("AT+SAPBR=3,1,APN,\"simbase\"");   //set apn
  sendSerial("AT+SAPBR=1,1");                   //enable gprs
  sendSerial("AT+HTTPINIT");                //enable http mode
  sendSerial("AT+HTTPPARA=CID,\"1\"");          //set http bearer profile
  sendSerial("AT+HTTPPARA=URL,\""+url+"\"");    //set url
  sendSerial("AT+HTTPACTION=0");                //start gttp get
  sendSerial("AT+HTTPTERM");                    //terminate sesion
  sendSerial("AT+SAPBR=0,1");                   //disable gprs
}

//#####################################################################################

void sendSerial(String command)
{
  int timeout = 0;
  char byteRead;
  Serial.println("Send ->: " + command);
  SIM800L.println(command);
  
  while(SIM800L.available()<2){    //think this might slow things down for everything to send through fully
    Serial.print(".");
    delay(200);

    timeout = timeout + 1;
    if(timeout>=240){
      Serial.println();
      Serial.println("Timeout, Sleeping for 5 mins to reset");
      goToSleep();
    }
  }

  
  long wtimer = millis();
  while (wtimer + 3000 > millis()) {        //min 3 seconds for reading, not sure why this is neccessary but i cant get it working without it
    while (SIM800L.available()) {
      byteRead = SIM800L.read();
      Serial.write(byteRead);
      if(byteRead == '2'){
        delay(10);
        byteRead = SIM800L.read();
        Serial.write(byteRead);
        if(byteRead == '0'){
          delay(10);
          byteRead = SIM800L.read();
          Serial.write(byteRead);
          if(byteRead == '0'){
            delay(10);
            byteRead = SIM800L.read();
            Serial.write(byteRead);
            if(byteRead == ','){
              tSuccess=1;
            }
          }
        }
      }
      //Done while
    }
  }
  Serial.println();
}
