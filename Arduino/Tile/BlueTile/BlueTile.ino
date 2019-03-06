#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include <Adafruit_NeoPixel.h>
#include <MPU9250_RegisterMap.h>
#include <SparkFunMPU9250-DMP.h>
#include <imumaths.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEOPIXELPIN 2
#define BUZZERPIN 13
#define NUMPIXELS 24

// Update these with values suitable for your network.
//WiFi
const char *ssid = "IoT-WiFi";
const char *password = "40219917";
//MQTT
const char *mqtt_server = "10.42.0.1";
const int mqtt_port = 1883;
//Buzzer
const int frequency = 200; // A6
const int tonePlay = 80;   // half a second tone
//WiFi + MQTT
const char *clientID; //Filled with mac address, unused, but kept in case of future functionality requiring it.
String clientIDstr;   //String containing mac address, used in conjunction with message building
String IP;            //String containing IP address.
String HOSTNAME;      //String containing HOSTNAME.
String configID = "bluetile";
//Neopixel vars
byte individualLedColors[NUMPIXELS * 3];
int numberOfActiveLeds = NUMPIXELS;

//Communication Objects
WiFiClient espClient;
PubSubClient client(espClient);

//Arduino components
MPU9250_DMP imu;
Adafruit_NeoPixel ringLight = Adafruit_NeoPixel(NUMPIXELS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);
Adafruit_VL6180X sensor = Adafruit_VL6180X();

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting WiFi...");

  delay(1000);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //Buzzer setup--------------------
  pinMode(BUZZERPIN, OUTPUT);

  //NoePixel Setup-----------------------------------
  pixels.begin(); // This initializes the NeoPixel library.
  Serial.println("Loading NeoPixel... Success!");
  delay(500); // a second delay.

  if (!sensor.begin())
  {
    Serial.println("Failed to find sensor.");
    while (1)
      ;
  }
  Serial.println("Sensor found!");

  // Call imu.begin() to verify communication and initialize
  if (imu.begin() != INV_SUCCESS)
  {
    while (1)
    {
      SerialPort.println("Unable to communicate with MPU-9250");
      SerialPort.println("Check connections, and try again.");
      SerialPort.println();
      delay(7000);
    }
  }
  SerialPort.println("Begin 1.");
  int success = imu.dmpBegin(DMP_FEATURE_6X_LP_QUAT |                    // Enable 6-axis quat
                                 DMP_FEATURE_GYRO_CAL | DMP_FEATURE_TAP, // Use gyro calibration
                             10);                                        // Set DMP FIFO rate to 10 Hz
  // DMP_FEATURE_LP_QUAT can also be used. It uses the
  // accelerometer in low-power mode to estimate quat's.
  // DMP_FEATURE_LP_QUAT and 6X_LP_QUAT are mutually exclusive

  // dmpSetTap parameters, in order, are:
  // x threshold: 1-1600 (0 to disable)
  // y threshold: 1-1600 (0 to disable)
  // z threshold: 1-1600 (0 to disable)
  // (Threshold units are mg/ms)
  // taps: Minimum number of taps needed for interrupt (1-4)
  // tap time: milliseconds between valid taps
  // tap time multi: max milliseconds between multi-taps
  unsigned short xThresh = 0;     // Disable x-axis tap
  unsigned short yThresh = 0;     // Disable y-axis tap
  unsigned short zThresh = 20;    // Set z-axis tap thresh to 100 mg/ms
  unsigned char taps = 1;         // Set minimum taps to 1
  unsigned short tapTime = 300;   // Set tap time to 100ms
  unsigned short tapMulti = 1000; // Set multi-tap time to 1s
  imu.dmpSetTap(xThresh, yThresh, zThresh, taps, tapTime, tapMulti);

  SerialPort.print(success);
  SerialPort.println("Begin 2.");
  //Setting default color for ringlight leds
  for (int i = 0; i < NUMPIXELS; i++)
  {
    individualLedColors[(i * 3)] = 0;
    individualLedColors[(i * 3) + 1] = 150;
    individualLedColors[(i * 3) + 2] = 0;
  }
}

void setup_wifi()
{
  delay(500);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  IP = WiFi.localIP().toString();
  Serial.println("IP address: " + IP);
  clientIDstr = WiFi.macAddress();
  clientID = clientIDstr.c_str();
  Serial.println("MAC address: " + clientIDstr);
  HOSTNAME = WiFi.hostname();
  Serial.println("Hostname: " + HOSTNAME);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("New message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char *topicElement;
  topicElement = strtok(topic, "/");

  while (topicElement != NULL)
  {
    Serial.println(topicElement);
    if (strcmp(topicElement, "ping") == 0)
    {
      //Serial.println("Ping event space entered!");
      ping_event(payload);
    }
    else if (strcmp(topicElement, "action") == 0)
    {
      action_event(topicElement, payload);
    }
    else if (strcmp(topicElement, "get") == 0)
    {
      get_event(topicElement);
    }
    topicElement = strtok(NULL, "/");
  }
}

void get_event(char *topicElement)
{
  while (topicElement != NULL)
  {
    topicElement = strtok(NULL, "/");
  }
}

void action_event(char *topicElement, byte *payload)
{
  while (topicElement != NULL)
  {
    if (strcmp(topicElement, "ringlight") == 0)
    {
      topicElement = strtok(NULL, "/");
      if (strcmp(topicElement, "color") == 0)
      {
        for (int i = 0; i < NUMPIXELS; i++)
        {
          individualLedColors[(i * 3)] = payload[0];
          individualLedColors[(i * 3) + 1] = payload[1];
          individualLedColors[(i * 3) + 2] = payload[2];
        }
      }
      else if (strcmp(topicElement, "state"))
      {
        if (payload[0] == 1)
        {
          lightsOn();
        }
        else
        {
          lightsOff();
        }
      }
      else if (strcmp(topicElement, "all_colors"))
      {
        for (int i = 0; i < NUMPIXELS; i++)
        {
          individualLedColors[(i * 3)] = payload[(i * 3)];
          individualLedColors[(i * 3) + 1] = payload[(i * 3) + 1];
          individualLedColors[(i * 3) + 2] = payload[(i * 3) + 2];
        }
      }
      else if (strcmp(topicElement, "number_of_leds"))
      {
        numberOfActiveLeds = payload[0];
      }
    }
    else if (strcmp(topicElement, "toneplayer"))
    {
      topicElement = strtok(NULL, "/");
      if (strcmp(topicElement, "play"))
      {
        int frequency = 0;
        for (int i = 0; i < 4; i++)
        {
          frequency = payload[i] * pow(256, i);
        }
        tone(BUZZERPIN, frequency);
      }
      else if (strcmp(topicElement, "stop"))
      {
        noTone(BUZZERPIN);
      }
      else if (strcmp(topicElement, "frequency_duration"))
      {
        int frequency = 0;
        int duration = 0;
        for (int i = 0; i < 4; i++)
        {
          frequency = payload[i] * pow(256, i);
          duration = payload[4 + i] * pow(256, i);
        }
        tone(BUZZERPIN, frequency, duration);
      }
    }
    topicElement = strtok(NULL, "/");
  }
} //End of action_event

void ping_event(byte *payload)
{
  Serial.print("Ping event received & ");
  if ((char)payload[0] == '1')
  {
    client.publish(("unity/device/" + clientIDstr + "/ping").c_str(), "1");
    Serial.print("returned.\n");
  }
  else
  {
    Serial.print("disconnecting WiFi in 3 sec...");
    delay(3000);
    WiFi.disconnect(true);
  }
} //End of ping_event

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT server... ");
    // Attempt to connect
    if (client.connect(clientID))
    {
      Serial.println("Connected!");
      // Once connected, publish an announcement to unity: unity/connect/device-id
      client.publish(("unity/connect/" + clientIDstr + "/" + configID).c_str(), "1");
      // Then Subcribe to everything client-id/#
      client.subscribe((clientIDstr + "/#").c_str());
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("Retrying in 5 seconds...");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  lightsConnected(50);
}

bool firstTimeOff = false;
uint8_t sensorStatus = -1;
bool rangeCode()
{
  uint8_t range = sensor.readRange();
  sensorStatus = sensor.readRangeStatus();
  // Serial.print("Range: "); Serial.println(range);
  if (sensorStatus == VL6180X_ERROR_NONE)
  { // Range between 0 and approx. 160
    Serial.print("Range: ");
    Serial.println(range);
    client.publish(("unity/device/" + clientIDstr + "/event/timeofflight/"), char(range));

    /*int pixels = map(range, 15, 160, no_of_Pixels, 0);
    int red = map(range, 0, 160, 255, 0);
    int blue = map(range, 0, 160, 0, 255);
    // int green = map(range, 0, 160, 0, 255);
    pixelBar(&ringLight, pixels, ringLight.Color(red, 0, blue));*/
    //firstTimeOff = true;

    return true;
  }
  /*else
  {
    if (firstTimeOff)
    {
      pixelBar(&ringLight, 0, ringLight.Color(0, 0, 0));
      firstTimeOff = false;
    }
  }*/
  return false;
}

int rangeDelay = 300;
unsigned long rangeTime = -1;
void loop()
{
  if (rangeTime == -1)
  {
    rangeTime = millis();
  }
  else
  {
    if (millis() > (rangeTime + rangeDelay))
    {
      rangeCode();
      rangeTime = -1;
    }
  }

  if (sensorStatus == VL6180X_ERROR_NONE)
  {
    rangeDelay = 10;
    return;
  }
  else
  {
    rangeDelay = 300;
  }
  // Check for new data in the FIFO
  if (imu.fifoAvailable())
  {
    // SerialPort.println("loop2");
    // Use dmpUpdateFifo to update the ax, gx, mx, etc. values
    if (imu.dmpUpdateFifo() == INV_SUCCESS)
    {
      // computeEulerAngles can be used -- after updating the
      // quaternion values -- to estimate roll, pitch, and yaw
      imu.computeEulerAngles();
      printIMUData();
      if (imu.tapAvailable())
      {
        unsigned char tapDir = imu.getTapDir();
        unsigned char tapCnt = imu.getTapCount();
        if ((tapDir = TAP_Z_UP) || (tapDir = TAP_Z_DOWN))
        {
          client.publish(("unity/device/" + clientIDstr + "/event/imu/tapped").c_str(), "1");
          //      SerialPort.print("Tap Z");
        }
      }
    }
  }
}

bool first_reading = true;
float first_pitch = 0.0;
float first_roll = 0.0;

void printIMUData(void)
{
  // After calling dmpUpdateFifo() the ax, gx, mx, etc. values
  // are all updated.
  // Quaternion values are, by default, stored in Q30 long
  // format. calcQuat turns them into a float between -1 and 1
  float q0 = imu.calcQuat(imu.qw);
  float q1 = imu.calcQuat(imu.qx);
  float q2 = imu.calcQuat(imu.qy);
  float q3 = imu.calcQuat(imu.qz);

  Quaternion q(q0, q1, q2, q3);
  Vector<3> qEuler = q.toEuler();
  Vector<3> qEulerDeg(qEuler.x() * 180.0 / M_PI, qEuler.y() * 180.0 / M_PI, qEuler.z() * 180.0 / M_PI);

  float pitchAngle = -qEulerDeg.y();
  float rollAngle = -qEulerDeg.z();
  float yawAngle = qEulerDeg.x();

  char rotation[6];
  rotation[0] = (((rollAngle + 2) * 360) >> 256 ? 255 : ((rollAngle + 2) * 360));
  rotation[1] = (((rollAngle + 2) * 360) <= 256 ? 0 : ((rollAngle + 2) * 360) - 255);
  rotation[2] = (((pitchAngle + 2) * 360) >> 256 ? 255 : ((pitchAngle + 2) * 360));
  rotation[3] = (((pitchAngle + 2) * 360) <= 256 ? 0 : ((pitchAngle + 2) * 360) - 255);
  rotation[4] = (((yawAngle + 2) * 360) >> 256 ? 255 : ((yawAngle + 2) * 360));
  rotation[5] = (((yawAngle + 2) * 360) <= 256 ? 0 : ((yawAngle + 2) * 360) - 255);

  client.publish(("unity/device/" + clientIDstr + "/event/imu/rotation").c_str(), rotation);

  /*
   if (first_reading) {
    first_reading = false;
    first_pitch = pitchAngle;
    first_roll = rollAngle;
   } else {
    pitchAngle = pitchAngle - first_pitch;
    rollAngle = rollAngle - first_roll;
   }
*/

  // SerialPort.println(" R/P/Y: " + String(rollAngle) + ", "
  //           + String(pitchAngle) + ", " + String(yawAngle));
}

// Set the first noOfPixelsOn pixels on ringLightBar to color.
// Mustbe called pixelBar(&ringLight, n, ringLight.Color(r, g, b));
// DS 2018...
void pixelBar(Adafruit_NeoPixel *ringLightBar, int noOfPixelsOn, uint32_t color)
{
  int noOfPixelsTotal = ringLightBar->numPixels();
  // Serial.print("rgb: "); Serial.print(r); Serial.print(",");Serial.print(g); Serial.print(","); Serial.println(b);
  if ((noOfPixelsOn >= 0) && (noOfPixelsOn <= noOfPixelsTotal))
  {
    for (uint16_t i = 0; i < noOfPixelsOn; i++)
    {
      ringLightBar->setPixelColor(i, color);
      ringLightBar->show();
    }
    for (uint16_t i = noOfPixelsOn; i < noOfPixelsTotal; i++)
    {
      ringLightBar->setPixelColor(i, 0);
      ringLightBar->show();
    }
  }
}