#include <ArduinoJson.h>
#include <Wire.h>
#include <Zumo32U4.h>
#include <Zumo32U4Motors.h>
#include <Zumo32U4Buzzer.h>

#define DELAY_SEND_DATA 2000; //delay between sending battery info over serial
int BTY_SHUTDOWN = 1; //at which battery percentage the zumo will stop
int BTY_EMERGENCY_MODE = 10; //at which battery percentage the zumo's speed will be reduced
#define SPEED_REDUCTION 0.25; //what the speed will be multiplied by

Zumo32U4Motors motors;
Zumo32U4Buzzer buzzer;

/* define the top forward/reverse speed as well as turn speed (0-400) */
const int16_t _topSpeed = 400;
const int16_t _turnSpeed = 200;
int16_t topSpeed = 400;
int16_t turnSpeed = 200;
int16_t speed;
int16_t turn;
float station_bty_prct;

int state = 0;

int16_t leftSpeed;
int16_t rightSpeed;
int16_t deltaSpeed;
int16_t deltaTurn;
int16_t lastSpeed = 0;
int16_t lastTurn = 0;

int8_t swBattery_max = 50;
float swBattery_kWh = 50;
float swBattery_percent = 100;
float temp_used_kWh;

unsigned long ms;
unsigned long lastms;
unsigned long last_buzz;

int batt_message[3] = {0,0,0};

void calculate_motor() {
  /* adjust speed and turn once per incoming data to get correct max values */
  if((speed == lastSpeed)&&(turn == lastTurn)) {
      } else {
        /* stop the gearbox from destroying itself during sudden shifts in motor values */
        deltaSpeed = speed - lastSpeed;
        if(deltaSpeed > _topSpeed/2) {
          deltaSpeed = _topSpeed/2;
        } else if(deltaSpeed < -_topSpeed/2) {
          deltaSpeed = -_topSpeed/2;
        }

        deltaTurn = turn - lastTurn;
        if(deltaTurn > _turnSpeed/2) {
          deltaTurn = _turnSpeed/2;
        } else if(deltaTurn < -_turnSpeed/2) {
          deltaTurn = -_turnSpeed/2;
        }

        speed = lastSpeed + deltaSpeed;
        turn = lastTurn + deltaTurn;

        if(speed > _topSpeed) {
          speed = _topSpeed;
        } else if(speed < -_topSpeed) {
          speed = -_topSpeed;
        }

        if(turn > _turnSpeed) {
          turn = _turnSpeed;
        } else if(turn < -_turnSpeed) {
          turn = -_turnSpeed;
        }
      }

      lastSpeed = speed;
      lastTurn = turn;
    }

    /* calculate correct rotation values for the left and right motor */
    if((speed > 0)&&(turn < 0)) {
      rightSpeed = speed;
      leftSpeed = speed + turn;
    } else if((speed < 0)&&(turn < 0)) {
      leftSpeed = speed - turn;
      rightSpeed = speed;
    } else if((speed > 0)&&(turn > 0)) {
      leftSpeed = speed;
      rightSpeed = speed - turn;
    } else if((speed < 0)&&(turn > 0)) {
      leftSpeed = speed;
      rightSpeed = speed + turn;
    } else if((speed == 0)&&(turn != 0)) {
      leftSpeed = turn;
      rightSpeed = 0-turn;
    }else {
      leftSpeed = speed;
      rightSpeed = speed;
    }  
}

void get_serial() /* get the data from esp32 serial communication */
{
  if (Serial1.available()) 
  {
    /* define a json document and saves data from the esp32 serial communication */
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, Serial1);

    if (error)
    { 
      /* prints debug information to the serial */
      Serial.print("deserializeJson() in get_serial() returned ");
      Serial.println(error.c_str());
    } else {
      speed = doc["speed"];
      turn = doc["turn"];

      speed *= topSpeed/100;
      turn *= turnSpeed/100;       

    /* reset the serial from the esp32 */
    while (Serial1.available() > 0) {
      Serial1.read();
    }
  }  
}

void controller() /* code to run in controller mode */
{      
  calculate_motor();
  /* sets the motor speed to the calculated values*/
  motors.setSpeeds(leftSpeed,rightSpeed);
}

void website() /* code to run in website mode */
{
  calculate_motor();
  /* sets the motor speed to the calculated values*/
  motors.setSpeeds(leftSpeed,rightSpeed);
}

void line() /* code to run in line following mode */
{
  //put code here
}

void charge() /* code to run when zumo needs charging */
{
  //put code here
}

void charging() /* code to run when zumo is charging */
{
  //put code here
}

void check_battery() {

  if(swBattery_percent <= BTY_SHUTDOWN) { /* battery is at a point where it will shut down to protect itself */

    if(batt_message[0] == 0) {  /* send a message to serial when battery status has changed */
      Serial.println("battery is empty, shutting down");
      batt_message[0] = 1;
      batt_message[1] = 0;
      batt_message[2] = 0;
    }
    topSpeed = 0;
    turnSpeed = 0;

  } else if(swBattery_percent <= BTY_EMERGENCY_MODE) { /* play a note each second if battery is below 10% and reduces speed */
    
    if(batt_message[1] == 0) {  /* send a message to serial when battery status has changed */
      Serial.println("emergency mode, speed is reduced");
      batt_message[0] = 0;
      batt_message[1] = 1;
      batt_message[2] = 0; 
    }
    
    topSpeed = _topSpeed * SPEED_REDUCTION;
    turnSpeed = _turnSpeed * SPEED_REDUCTION;

    if(ms - last_buzz >= 1000 && state != 4) {
      buzzer.playFrequency(445, 100, 10);
      last_buzz = millis();
    }
      
  } else if(swBattery_percent <= 100) { /* battery is fully operational */
  
    if(batt_message[2] == 0) {  /* send a message to serial when battery status has changed */
        Serial.println("battery is fully operating");
        batt_message[0] = 0;
        batt_message[1] = 0;
        batt_message[2] = 1; 
      }

    topSpeed = _topSpeed;
    turnSpeed = _turnSpeed;
  }

}

void setup() {
  /* initialize the debug serial port */
  Serial.begin(115200);

  /* initialize the esp32 wired serial communication */
  Serial1.begin(115200);
}

void loop() {
  /* calculate used battery since last loop */
  ms = millis();

  /*temp_used_kWh = ((sqrt((abs(leftSpeed) + abs(rightSpeed))/800))/4) * (ms - lastms);
  swBattery_kWh -= temp_used_kWh;

  swBattery_percent = (swBattery_kWh/swBattery_max)*100;*/
    
  /* get data from esp32/server */
  get_serial();

  /* check if the battery level is at an acceptable range */
  //check_battery();
  
  /*  */
  switch(state) {
    case 0:
      controller();
      break;
    case 1:
      website();
      break;
    case 2:
      line();
      break;
    case 3:
      charge();
      break;
    case 4:
      charging();
      break;
  }
  lastms = ms;

  delay(10); //coffee break

}
