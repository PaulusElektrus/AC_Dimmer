/* Dimmer Library: https://github.com/RobotDynOfficial/RBDDimmer */
#include <RBDdimmer.h>

/* Pinout */
#define VENTILATOR_RELAIS     5
#define HEATER_RELAIS         4
#define ZEROCROSS_PIN         2
#define DIMMER_PIN            3

/* Thresholds */
#define SMALL_POWER_THRESHOLD -15.0 /* W */
#define BIG_POWER_THRESHOLD   50.0 /* W */
#define VENTILATOR_POWER      -40.0 /* W */
#define HEATER_POWER          -30.0 /* W */
#define INSTANT_OFF           250.0 /* W */
#define MAX_HEATER_SET        98 /* % */
/* Timing */
#define DIMMER_DELAY          100 /* msec */
#define HEATER_UPDATE_TIME    500 /* msec */
#define REPORT_TIME           1000 /* msec */
/* Debug Mode */
#undef  DEBUG_MODE

enum heaterStates
{   
  OFF_STATE, 
  VENTILATOR,
  HEATER,
  ERROR 
};

/* Global Variables */
heaterStates  state = OFF_STATE;
float         powerMeasured = 0.0;
int           powerSet      = 1;

bool          ventilatorState = OFF;
bool          heaterState     = OFF;

/* Dimmer */
dimmerLamp dimmer(DIMMER_PIN);

void setup() 
{
  Serial.begin(115200);
  setupPinout();
}

void setupPinout()
{
  pinMode(VENTILATOR_RELAIS, OUTPUT);
  pinMode(HEATER_RELAIS, OUTPUT);
  dimmer.begin(NORMAL_MODE, OFF);
  ventilatorOff();
}

void loop() {
  getData();
  setHeater();
  #ifdef DEBUG_MODE
  serialReport();
  #endif
}

void getData() {
  if (Serial.available()) 
  {
    powerMeasured = Serial.parseFloat();
  }
}

void setHeater()
{
  static unsigned long lastUpdate = millis();

  if (millis() - lastUpdate >= HEATER_UPDATE_TIME)
  {
    if (powerMeasured > INSTANT_OFF)
    {
      /* Check for high consumption - fast Off feature */
      ventilatorOff();
    }

    switch (state)
    {
    case OFF_STATE:
      if (powerMeasured < VENTILATOR_POWER)
      {
        ventilatorOn();
      }
      break;
    
    case  VENTILATOR:
      if (powerMeasured > SMALL_POWER_THRESHOLD)
      {
        ventilatorOff();
      }
      else if (powerMeasured < HEATER_POWER)
      {
        heaterOn();
      }
      break;

    case HEATER:
      if (powerMeasured > SMALL_POWER_THRESHOLD)
      {
        if (powerSet <= 1)
        {
          heaterOff();
          state = VENTILATOR;
          break;
        }
        if (powerMeasured > BIG_POWER_THRESHOLD)
        {
          if (powerSet <= 5)
          {
            ventilatorOff();
            break;
          }
          powerSet -= 5;
          dimmer.setPower(powerSet);
          break;
        }
        powerSet--;
        dimmer.setPower(powerSet);
      }
      else if (powerMeasured < HEATER_POWER)
      {
        if (powerSet < MAX_HEATER_SET)
        {
          powerSet++;
          dimmer.setPower(powerSet);
        }
      }
      break;

    case ERROR:

    default:
      ventilatorOff();
      break;
    }
    lastUpdate = millis();
  }
}

void serialReport()
{
  static unsigned long lastReport = millis();

  if (millis() - lastReport >= REPORT_TIME)
  {
    String output;
    output += "\nActual Power: ";
    output += powerMeasured;
    output += "\nVentilator State: ";
    output += ventilatorState;
    output += "\nHeater State: ";
    output += heaterState;
    output += "\nPower Set: ";
    output += powerSet;
    output += " %\n";
    Serial.println(output);
    lastReport = millis();
  }
}

void ventilatorOn()
{
  digitalWrite(VENTILATOR_RELAIS, LOW);
  ventilatorState = ON;
  state = VENTILATOR;
}

void ventilatorOff()
{
  heaterOff();
  digitalWrite(VENTILATOR_RELAIS, HIGH);
  ventilatorState = OFF;
  state = OFF_STATE;
}

void heaterOn()
{
  ventilatorOn();
  digitalWrite(HEATER_RELAIS, LOW);
  delay(DIMMER_DELAY);
  powerSet = 1;
  dimmer.setPower(powerSet);
  dimmer.setState(ON);
  heaterState = ON;
  state = HEATER;
}

void heaterOff()
{
  dimmer.setState(OFF);
  delay(DIMMER_DELAY);
  powerSet = 1;
  dimmer.setPower(powerSet);
  digitalWrite(HEATER_RELAIS, HIGH);
  heaterState = OFF;
}
