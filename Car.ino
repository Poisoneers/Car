#include <Arduino_FreeRTOS.h>
#include <IRremote.hpp>
#include <semphr.h>

//  my definitions
const int irPIN = 10;
const int mSpeedA = 6;
const int mSpeedB = 9;
const int mD1 = 12;
const int mD2 = 13;
const int mD3 = 4;
const int mD4 = 5;
const int trig = 2;
const int echo = 8;

volatile unsigned int distance;
int motorSP = 100; //def speed.
volatile unsigned long irCommand = 0; // this will just store the last command

SemaphoreHandle_t xSemaphore;

void setupHC() { //this sets up the sensor for measuring the distance.
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
}

unsigned int getD() { //our simple function that will make sure that the pulse
  digitalWrite(trig, LOW); // is on high for at least 10 us
  delayMicroseconds(5);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10); //here it is.
  digitalWrite(trig, LOW);
  long length = pulseIn(echo, HIGH); //measures our pulse length.
  // to calculate the distance: we know velocity of sound is 340, the sensor will go
  //to the object and then back, so to get the distance only to the object
  //we divide by 2, then you multiply it with the velocity of sound which is 340
  //but since we want it in cm, i divided it by 1000 -> 0.034
  unsigned int distance = (length / 2) * 0.034;
  return distance;
}

void printD(unsigned int distance) { //simple print function
  Serial.print("The distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

void stopM() { //function for stopping the motors, incase it is < 20 cm
  analogWrite(mSpeedB, 0);
  analogWrite(mSpeedA, 0);
  digitalWrite(mD1, LOW);
  digitalWrite(mD2, LOW);
  digitalWrite(mD3, LOW);
  digitalWrite(mD4, LOW);
  Serial.println("Stopping");
}

void measureD(void *pvParam) { //will constantly measure the distance
  for (;;) { //runs forever
    unsigned int currentDistance = getD(); //calls the func for measuring
    //below it will try to access the semaphore
    if (xSemaphoreTake(xSemaphore, (TickType_t) 10) == pdTRUE) {  
      distance = currentDistance; //updates the distance
      printD(distance); //prints it
      if (distance <= 20) { //stops it 
        stopM();
      }
      xSemaphoreGive(xSemaphore); //gives it back
    }
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(mSpeedB, OUTPUT);
  pinMode(mSpeedA, OUTPUT);
  pinMode(mD1, OUTPUT);
  pinMode(mD2, OUTPUT);
  pinMode(mD3, OUTPUT);
  pinMode(mD4, OUTPUT);
  setupHC(); //calls the function.

  xSemaphore = xSemaphoreCreateMutex(); //creates our mutex sem
  if (xSemaphore == NULL) {
    Serial.println("Failed");
    while (1);
  }

  xTaskCreate(measureD, "Measure Distance", 128, NULL, 1, NULL); //creates our task

  IrReceiver.begin(irPIN, 0); // initialization

  vTaskStartScheduler(); //executes the tasks, starting from prio
}

void loop() { //it will start with checking for the signal
  if (IrReceiver.decode()) { 
    if (xSemaphoreTake(xSemaphore, (TickType_t) 10) == pdTRUE) {
      irCommand = IrReceiver.decodedIRData.decodedRawData; //stores the cmd

      switch (irCommand) { //switch statement for diff cases, depending on cmd
        case 0xE619FF00UL: // forward
          if (distance > 20) {
            digitalWrite(mD1, HIGH);
            digitalWrite(mD2, LOW);
            digitalWrite(mD3, HIGH);
            digitalWrite(mD4, LOW);
            analogWrite(mSpeedB, motorSP);
            analogWrite(mSpeedA, motorSP);
            Serial.println("Forward");
          } else {
            Serial.println("Something close, stopping");
          }
          break;
        case 0xE31CFF00UL: // back
          digitalWrite(mD1, LOW);
          digitalWrite(mD2, HIGH);
          digitalWrite(mD3, LOW);
          digitalWrite(mD4, HIGH);
          analogWrite(mSpeedB, motorSP);
          analogWrite(mSpeedA, motorSP);
          Serial.println("Backing");
          break;
        case 0xF30CFF00UL: // left
          digitalWrite(mD1, LOW);
          digitalWrite(mD2, HIGH);
          digitalWrite(mD3, HIGH);
          digitalWrite(mD4, LOW);
          analogWrite(mSpeedB, motorSP);
          analogWrite(mSpeedA, motorSP);
          Serial.println("Turning left");
          break;
        case 0xA15EFF00UL: // right
          digitalWrite(mD1, HIGH);
          digitalWrite(mD2, LOW);
          digitalWrite(mD3, LOW);
          digitalWrite(mD4, HIGH);
          analogWrite(mSpeedB, motorSP);
          analogWrite(mSpeedA, motorSP);
          Serial.println("Turning right");
          break;
        case 0xBB44FF00UL: // speed 30% (a)
          motorSP = 30;
          Serial.println("Speed: A, 30%");
          break;
        case 0xBF40FF00UL: // speed 60% (b)
          motorSP = 60;
          Serial.println("Speed: B, 60%");
          break;
        case 0xBC43FF00UL: // speed 100% (c)
          motorSP = 100;
          Serial.println("Speed: C, 100%");
          break;
        default:
          Serial.println("Command unknown");
          break;
      }
      IrReceiver.resume(); //ready for next cmd
      xSemaphoreGive(xSemaphore); //gives it back
    }
  }
}



