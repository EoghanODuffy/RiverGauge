#include <SoftwareSerial.h>           //for communication with gsm module
#include <NewPing.h>                  //for communication with sonar sensor

#define TRIGGER_PIN  14               // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     12               // Arduino pin tied to echo pin on the ultrasonic sensor.
#define SENSOR_HEIGHT 400             // Hight of the sensor above the river bed
#define NUM_SAMPLES 15                // Number of samples to take

//Create software serial object to communicate with SIM800L
SoftwareSerial SIM800L(13, 15); //SIM800LL Tx & Rx is connected to Arduino #3 & #2
NewPing sonar(TRIGGER_PIN, ECHO_PIN, SENSOR_HEIGHT);     //Trigger pin, echopin, max distance - set to sensor height so no negative values are returned


//CONFIG DATA
int sleepTime=900E6;                   //Default sleep time with good battery connection
int resetSleepTime = 300E6;            //time in seconds*E6 after an upload failure (prevents significant battery drain)
const String APN  = "SIMAPN";        //sim APN
const String Pass = "WEBSERVERPASSWORD"; //Password for upload server
int gaugeid = 1;                      //ID for gauge upload


//SET UP VARIABLES
int modulebattery;                    //battery level to send
int waterlevel;                       //water level to send
int tSuccess = 0;                     //changes to 1 on successful data transmission to web server
int pwrPin = 5;                       //connected to sim power mosfet. High is on
int resetPin = 4;                     //Reset pin. Bring to gnd to reset
int readings[NUM_SAMPLES];            //Sensor readings


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);       //led indicator light, low is on
  pinMode(pwrPin,OUTPUT);                  //pwr pin
  pinMode(resetPin,OUTPUT);      
  digitalWrite(resetPin,HIGH);
  digitalWrite(pwrPin,HIGH);
  
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  //Begin serial communication with Arduino and SIM800LL
  SIM800L.begin(9600);

  Serial.println("Initializing...");
  delay(1500);    //Short delay so the SIM800L can start recieving commands
  Serial.println("Waiting for sim and sensor ready");
  delay(1500);
  Serial.println("Reading Gauge Data");
  Serial.print("Sensor Height: ");
  Serial.println(SENSOR_HEIGHT);
  Serial.print("Reading(");
  
  //Measure data from sensor and get average
  /*
  for(int i=0;i<10;i++){
    waterlevel=waterlevel+(SENSOR_HEIGHT-(sonar.ping_cm()));
    Serial.print("#");
    delay(500);
  }
  Serial.println(")");
  waterlevel = waterlevel/10;
  Serial.print("Water Level = ");
  Serial.println(waterlevel);
  */
  //Measure Sensor and get median
  for (int i = 0; i < NUM_SAMPLES; i++) {
    readings[i] = SENSOR_HEIGHT - sonar.ping_cm();
    Serial.print("#");
    delay(300);
  }
  Serial.println(")");

  //Sort the readings gotten
  for (int i = 0; i < NUM_SAMPLES - 1; i++) {
    for (int j = i + 1; j < NUM_SAMPLES; j++) {
      if (readings[j] < readings[i]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }
  //Calculate median
  if (NUM_SAMPLES % 2 == 0) {
    // Even number of samples → average of two middle values
    waterlevel = (readings[NUM_SAMPLES/2 - 1] + readings[NUM_SAMPLES/2]) / 2;
  } else {
    // Odd number of samples → middle value
    waterlevel = readings[NUM_SAMPLES/2];
  }
  Serial.print("Water Level = ");
  Serial.println(waterlevel);



  
  sendSerial("AT","OK", 10);  //quick comms check
  digitalWrite(LED_BUILTIN, LOW);
  checkBattery();
  digitalWrite(LED_BUILTIN, HIGH);
  waitForConnection();
  uploadData(gaugeid, waterlevel, modulebattery);
  goToSleep();  
}

//#####################################################################################
//MAIN LOOP - THIS SHOULD NEVER HAPPEN. IF THIS OCCURS THEN A MAJOR ERROR HAS OCCURED
void loop()
{
  Serial.println("entered loop, error with sleep");
  goToSleep();
  ESP.deepSleep(900E6);
}


//#####################################################################################

void checkBattery(){                  //This could definetly be made prettier but it works and is resonably quick
  Serial.println("Send ->: AT+CBC");
  while (SIM800L.available()){        //clear anything in serial buffer
    SIM800L.read();
  }

  int timeout = 0;                    //initilize timeout counter
  
  SIM800L.println("AT+CBC");
  while(SIM800L.available()<30){      //wait for 30 characters to come in before moving on or else restart everything
    Serial.print(".");
    timeout = timeout + 1;
    if(timeout>=200){                 //Timeout if response takes longer than 40 seconds
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
  while(SIM800L.available())          //read response
  {
    byteRead = SIM800L.read();        //read data
    if(byteRead == delim){            //if battery data string comes through then set C1,C2,C3 to corrisponding chars
      C1=SIM800L.read();
      if(C1 != delim){
        C2=SIM800L.read();
        if(C2 != delim){
          C3=SIM800L.read();
        }
        
      }
    }

  }
  Serial.println(C1);     //reports wrong values unless these are printed. I dont know why. Please dont delete
  Serial.println(C2);
  Serial.println(C3);
  
  if(C1==','){                    //convert individual chars to int
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
//This waits for a connection to be established before sending data
//In most cases this takes under 10 seconds but can take longer in remote areas
//
void waitForConnection(){
  Serial.println("Checking for Connection");
  Serial.println("Send ->: AT+CGREG?");
  while (SIM800L.available()){
    SIM800L.read();
  }
  int simConnected =0;
  int attempts = 0;

  while(simConnected == 0){
    Serial.println("Checking");
    int timeout = 0;
    attempts = attempts + 1;
  
    SIM800L.println("AT+CGREG?");               //Checks for connection
    while(SIM800L.available()<5){               //wait for 5 characters to come in before moving on or else restart everything
      Serial.print(".");
      timeout = timeout + 1;
      if(timeout>=200){                         //if taking too long to respond, timeout. Set at 40 seconds (Delay*200)
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
      if(byteRead == '5' || byteRead == '1'){       //5 for roaming, 1 for local
        simConnected = 1;
      }
    }
    if (attempts>15){           //Give up after 15 tries
      goToSleep();
    }
    delay(2000);
  }
  Serial.println("Connection Successful");
}



//SLEEP SETTINGS
//######################################################################################

void goToSleep(){
  int newTime = sleepTime;
  Serial.println("#########################################################################");
  SIM800L.println("AT+CPOWD=1");
  delay(100);
  
  if(tSuccess==0){
    Serial.println("ERROR UPLOADING, Resetting sim card");
    newTime=resetSleepTime;
  }else{
    Serial.print("SUCCESSFUL TRANSMISSION, Sleeping for ");   //Choose a sleep time depending on battery level
    if(modulebattery<15){
      newTime=sleepTime * 8;
    }else if(modulebattery<25){
      newTime=sleepTime * 4;
    }else if(modulebattery<35){
      newTime=sleepTime * 2;
    }else{
      newTime=sleepTime;
    }
    Serial.println(newTime);
  }
  
  Serial.println("Going into Sleep");
  digitalWrite(pwrPin,LOW);
  ESP.deepSleep(newTime);
}


//UPLOAD DATA - AT commands to send the data to the server
//#####################################################################################

void uploadData(int gauge, int level, int battery){
  String url = "http://rivergauge.net/upload.php?id="+String(gauge)+"&l="+String(level)+"&b="+String(battery)+"&pw="+Pass;     //Compose link to send
  Serial.println(url);
  sendSerial("AT+CPIN?","OK", 10);
  sendSerial("AT+CSQ","OK", 10);
  sendSerial("AT+CGATT=1","OK", 10);
  sendSerial("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"","OK", 10);  //set gprs
  sendSerial("AT+SAPBR=3,1,\"APN\",\""+APN+"\"","OK", 10);   //set apn
  sendSerial("AT+SAPBR=1,1","OK", 10);
  sendSerial("AT+SAPBR=2,1","OK", 10);
  sendSerial("AT+HTTPINIT","OK", 10);                    //enable http mode
  sendSerial("AT+HTTPPARA=\"CID\",1","OK", 10);          //set http bearer profile
  sendSerial("AT+HTTPPARA=\"UA\",\"SIM800\"","OK", 10);
  sendSerial("AT+HTTPPARA=\"URL\",\""+url+"\"","URL", 10);    //set url
  sendSerial("AT+HTTPACTION=0","200", 30);                //start gttp get
  sendSerial("AT+HTTPTERM","OK", 10);                    //terminate sesion
  sendSerial("AT+SAPBR=0,1","OK", 10);                   //disable gprs
}


//COMMUNICATION WITH THE SIM800L
//#####################################################################################

void sendSerial(String command, String response, int maxAttempts)
{ 
  int timeout = 0;
  Serial.println("Send ->: " + command);
  digitalWrite(LED_BUILTIN, LOW); 
  SIM800L.println(command);
  
  while(SIM800L.available()<2){    //wait for serial to start coming in
    Serial.print(".");
    delay(200);

    timeout = timeout + 1;          
    if(timeout>=200){               //If sim800l is taking too long to reply. Reset everything and start over
      Serial.println();
      Serial.println("Timeout, Sleeping and resetting");
      goToSleep();
    }
  }
  
  digitalWrite(LED_BUILTIN, HIGH);
  String SerialResponse = "";
  char byteRead;
  bool ResponseFound = false;
  int attempts = 0;
  while (ResponseFound == false){ //loop until desired responce come through
    while (SIM800L.available()){  //get all incoming serial and convert to string
      byteRead = SIM800L.read();
      SerialResponse.concat(byteRead);
      delay(1);
    }
    
    Serial.println(SerialResponse);
    if(SerialResponse.indexOf(response) >= 0){ //is the response in the string
        ResponseFound = true;
        Serial.println("Response found");
        if(response == "200"){
          tSuccess=1;
        }
    }else if(SerialResponse.indexOf("ERROR") >= 0){ //is error found in the string returned
      Serial.println("SIM ERROR, Continuing Anyway");
      ResponseFound = true;
    }else if(SerialResponse.indexOf("601") >= 0){
      Serial.println("Transmit Error, Resetting");
      goToSleep();          
    }else {                       //if not try again
      attempts = attempts + 1;
      if (attempts == maxAttempts){             //if limit reached reset everything
        Serial.println("Timeout, Sleeping and resetting");
        goToSleep();
      }
      Serial.println("Waiting for response");
      delay(1000);
    }
  }
}

