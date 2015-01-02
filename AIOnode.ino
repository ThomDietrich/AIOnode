
// Ethernet Library
#include <SPI.h>
#include <Ethernet.h>

// Dallas Temperature Control Library
// http://milesburton.com/Main_Page?title=Dallas_Temperature_Control_Library
#include <OneWire.h>
#include <DallasTemperature.h>

// operate remote radio controlled devices with 433MHz AM transmitter
// http://code.google.com/p/rc-switch
#include <RCSwitch.h>

/***************************************************************************************************
 *  Ethernet Server Settings: MAXC, static fallback IP
 **************************************************************************************************/
byte mac[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
IPAddress ip(192, 168, 23, 234);
IPAddress gateway(192, 168, 23, 100);
IPAddress subnet(255, 255, 255, 0);
int webserverPort = 80;

#define BUFFERSIZE 128
EthernetServer server(webserverPort);

/***************************************************************************************************
 *  Ethernet Client
 **************************************************************************************************/
byte loggingServer[] = {10, 21, 7, 109};
int logPort = 3333;
EthernetClient ethClient;
unsigned long lastClientExec = 0;
unsigned long interval = 300 * 1000;


/***************************************************************************************************
 *  OneWire
 **************************************************************************************************/
// Setup oneWire instance to communicate with OneWire devices
OneWire oneWireBus1(5);
OneWire oneWireBus2(6);
OneWire oneWireBus3(7);
DallasTemperature sensor1(&oneWireBus1);
DallasTemperature sensor2(&oneWireBus2);
DallasTemperature sensor3(&oneWireBus3);
float temp1;
float temp2;
float temp3;

/***************************************************************************************************
 *  HX2262
 **************************************************************************************************/
RCSwitch hx2262Switch = RCSwitch();
#define HX2262_PIN 4

/***************************************************************************************************
 *  SETUP
 **************************************************************************************************/
void setup() {
  Serial.begin(9600);
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for Leonardo only
  //}
  Serial.println("GoGoGo!");

  Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {
    Ethernet.begin(mac, ip, gateway, subnet);
  }
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // Start up the Dallas Temperature Control Library
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();

  hx2262Switch.enableTransmit(HX2262_PIN);
}

/***************************************************************************************************
 *  LOOP
 **************************************************************************************************/
void loop() {

  /***************************************************************************************************
   *  HTTP Server
   **************************************************************************************************/

  char buffer[BUFFERSIZE];
  int bufindex = 0;  

  EthernetClient client = server.available();
  if (client) {
    Serial.println("(new client)");

    memset(buffer, 0, sizeof(buffer));

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);

        if (c != '\r' && c != '\n' && bufindex < BUFFERSIZE){
          buffer[bufindex] = c;
          bufindex++;
          continue;
        }

        // all lines after the request can be ignored
        Serial.print("bytes in memory before flush: ");
        Serial.println(client.available());
        client.flush();
        Serial.print("bytes in memory after flush: "); 
        Serial.println(client.available());

        Serial.println(buffer);
        String requestString = String(buffer);

        /***************************************************************************************************
         * parse request
         **************************************************************************************************/
        // HTTP-Methode (GET, POST, PUT, ...)
        String operation = requestString.substring(0, requestString.indexOf(' '));
        Serial.print("operation: "); Serial.println(operation);
        // url
        requestString = requestString.substring(requestString.indexOf('/'), requestString.indexOf(' ', requestString.indexOf('/')));
        requestString.toLowerCase();
        // back to chararray
        requestString.toCharArray(buffer, BUFFERSIZE);
        char *command = strtok(buffer,"/");
        char *parameter = strtok(NULL,"/");

        if(command != NULL){
         if(parameter != NULL){
         Serial.print("command: "); Serial.println(command);
         Serial.print("parameter: "); Serial.println(parameter);
         }
         else {
         Serial.print("command: "); Serial.println(command);
         Serial.println("no parameters");
         }
         }

        /***************************************************************************************************
         * DS18B20 Temperature Command
         **************************************************************************************************/
        if (strcmp(command, "temperature") == 0) {
          Serial.println("Temperatur Dallas");
          temp1 = readTemperatureCelsius(sensor1);
          temp2 = readTemperatureCelsius(sensor2);
          temp3 = readTemperatureCelsius(sensor3);
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Server: arduino");
          client.println("Connnection: close");
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();

          client.print(temp1, 1); client.print(";");
          client.print(temp2, 1); client.print(";");
          client.print(temp3, 1); client.print(";");

          client.println();
          break;
        }

        /***************************************************************************************************
         * HX2262 SwitchOn/SwitchOff
         **************************************************************************************************/
        else if ((strcmp(command, "switchon") == 0) || (strcmp(command, "switchoff") == 0)) {
          Serial.println("HX2262 SwitchOn-SwitchOff");
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Server: arduino");
          client.println("Connnection: close");
          client.println();

          char group[6];
          strncpy(group, parameter, 5);
          group[5] = '\0';
          //Serial.println(group);

          char device[6];
          strncpy(device, parameter+5, 5);
          device[5] = '\0';
          //Serial.println(device);

          if (strcmp(command, "switchon") == 0) {          
            hx2262Switch.switchOn(group, device);
            client.print("HX2262 address ");
            client.print(parameter);
            client.println(" on");
          }
          if (strcmp(command, "switchoff") == 0) {
            hx2262Switch.switchOff(group, device);
            client.print("HX2262 address ");
            client.print(parameter);
            client.println(" off");
          }
          client.println();
          break;
        }

        /***************************************************************************************************
         * Template
         **************************************************************************************************/
        else if (strcmp(command, "something_else") == 0) {
          Serial.println("...");
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Server: arduino");
          client.println("Connnection: close");
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();

          // magic here

          client.println();
          break;
        }

        /***************************************************************************************************
         * no command present - help text
         **************************************************************************************************/
        else {
          Serial.println("Error, no or unknown command");
          //client.println("HTTP/1.1 404 Not Found");
          //client.println("Content-Type: text/html");
          //client.println();
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json;charset=utf-8");
          client.println("Server: Arduino");
          client.println("Connnection: close");
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("Error!\r\n\r\nno or unknown command\r\nhttp://<ip>/temperature\r\nhttp://<ip>/SwitchOn/0111010000\r\nhttp://<ip>/SwitchOff/0111010000");
          client.println();
          break;
        }


        /***************************************************************************************************
         **************************************************************************************************/
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("(client disonnected)");
  }
  /***************************************************************************************************
   *  Ethernet Client
   **************************************************************************************************/  
  if (millis() - lastClientExec > interval) {
    lastClientExec = millis();
    Serial.println("Send");
    if (ethClient.connect(loggingServer, logPort)) {
      Serial.println("connected");
      temp1 = readTemperatureCelsius(sensor1);
      temp2 = readTemperatureCelsius(sensor2);
      temp3 = readTemperatureCelsius(sensor3);
      
      ethClient.print("{\"host\":\""); ethClient.print(Ethernet.localIP());
      ethClient.print("\",\"logsource\":\"arduino-logger");
      ethClient.print("\",\"millis\":\""); ethClient.print(lastClientExec);
      ethClient.print("\",\"message\":\""); ethClient.print(temp1, 1); ethClient.print(";"); ethClient.print(temp2, 1); ethClient.print(";"); ethClient.print(temp3, 1);
      ethClient.print("\",\"temperature_1\":\""); ethClient.print(temp1, 1);
      ethClient.print("\",\"temperature_2\":\""); ethClient.print(temp2, 1);
      ethClient.print("\",\"temperature_3\":\""); ethClient.print(temp3, 1);
      ethClient.print("\"}");
      ethClient.println();
      delay(1);
      ethClient.stop();
    } else {
      Serial.println("connection failed");
    }
  }
  //Behandelt Überlauf
  if (millis() < lastClientExec) lastClientExec = 0;
  
} //LOOP

/***************************************************************************************************
 *  DS18B20 Temperature Sensor
 **************************************************************************************************/
float readTemperatureCelsius(DallasTemperature oneWireSensor) {
  oneWireSensor.requestTemperatures();
  float temperature = oneWireSensor.getTempCByIndex(0);
  //temperature = (float)(((int)temperature * 10.0) / 10.0);
  Serial.print("Celsius Temperature for device is: ");
  Serial.println(temperature,3);  //zero is first sensor if we had multiple on bus
  return temperature;
}




