#include <LiquidCrystal.h>  // includes the LiquidCrystal Library
#include <Wire.h>
#include <string.h>

#define maxDisplayLength 20
#define maxDisplayDetails 4
#define className "F305"

#define guideThresholdMinute 2   //minutes
#define guideThresholdSecond 10  //seconds
#define guideDelayTime 5000     //milliseconds

//Define pins for LCD control
const int RS = 2, EN = 3, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);  // Creates an LCD object. Parameters: (rs, enable, d4, d5, d6, d7)


// Char array to hold details to be displayed
char details[maxDisplayDetails][maxDisplayLength];

// Char array to hold visitor's guide to be displayed
char visitorGuide[maxDisplayDetails][maxDisplayLength];

// Char array to hold details of current time and day
char currentDay[maxDisplayLength];

char scheduleEnd[maxDisplayLength];

char visitorTimeStamp[maxDisplayLength];

// Flag to know if data arrived
bool isDataReceived = false;

// Counter for the number of data recieved
int receiveCount = 0;
int visitorReceiveCount = 0;

// flag that indicates a completely received data
bool receiveDetailsComplete = false;
bool receiveGuideComplete = false;

// Flag that indicates details arrived
bool scheduleDetailArrived = false;
bool visitorGuideArrived = false;

bool scheduleFree = true;
bool guideOn = false;

long int guideTimeStart;
long int guideTimeEnd;

bool schedulePaused = false;
bool scheduleActivated = false;
//
void setup() {
  // Open serial communications and wait for port to open:
  setupI2C();
  lcd.begin(maxDisplayLength, maxDisplayDetails);  // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display
  int displayColCursor = (maxDisplayLength - 14) / 2;
  int displayRowCursor = (maxDisplayDetails / 2) - 1;
  lcd.setCursor(displayColCursor, displayRowCursor);
  lcd.print("IoT Class Sign");  // Prints string "Distance" on the LCD
  delay(2000);

  lcd.clear();
}

void setupI2C() {
  Wire.begin(8);                /* join i2c bus with address 8 */
  Wire.onReceive(receiveEvent); /* register receive event */
  Wire.onRequest(requestEvent); /* register request event */
  Serial.begin(9600);           /* start serial for debug */
}

int currentHour = 0, currentMinute = 0, currentSecond = 0, endTrigHour = 0, endTrigMinute = 0;
int visitorHourStamp = 0, visitorMinuteStamp = 0, visitorSecondStamp = 0;

void loop() {  // run over and over

  // Continually display the current day and time
  lcd.setCursor(1, 0);
  lcd.print(currentDay);
  SplitCurrentTime(currentDay, &currentHour, &currentMinute, &currentSecond);

  // If the class is free, display the name of the class and a free notification
  if ((scheduleFree == true) && (guideOn == false)) {
    lcd.setCursor(8, 1);
    lcd.print(className);
    lcd.setCursor(5, 3);
    lcd.print("Class Free");
  }



  // If a schedule has arrived, check the current time against the end time of that schedule.
  // If they are equal, clear the display and declare a free schedule
  if (scheduleDetailArrived == true) {
    if (currentHour == endTrigHour) {
      if (currentMinute == endTrigMinute) {
        lcd.clear();
        lcd.setCursor(8, 1);
        lcd.print(className);
        lcd.setCursor(4, 3);
        lcd.print("Class Ended");
        scheduleFree = true;
        scheduleDetailArrived = false;
        scheduleActivated = false;
        Serial.println("Schedule ended");
        delay(2000);
        lcd.clear();
      }
    }
  }

  // If a visitor guide has arrived, check the current time against the timestamp of that guide.
  // If within time frame of 1 minute, display the guide
  if (visitorGuideArrived == true) {
    if ((millis() - guideTimeStart) >= guideDelayTime) {
      lcd.clear();
      // lcd.setCursor(8, 1);
      // lcd.print(className);
      // lcd.setCursor(4, 3);
      // lcd.print("Class Ended");
      // // scheduleFree = true;
      visitorGuideArrived = false;
      guideOn = false;
      schedulePaused = false;
      // Serial.println("Schedule ended");
      // delay(2000);
      // lcd.clear();
    }
  }

  // When a schedule is received completely, clear the screen, extract the end time of that schedule and display the details
  if ((receiveDetailsComplete == true || scheduleActivated == true) && (schedulePaused == false)) {
    scheduleActivated = true;
    scheduleFree = false;
    // Serial.println("Completed....");

    //Clear the display
    if (receiveDetailsComplete == true) {
      lcd.clear();
    }
    // delay(500);

    strcpy(scheduleEnd, details[3]);
    SplitScheduleTime(scheduleEnd, &endTrigHour, &endTrigMinute);
    // Serial.print(endTrigHour);
    // Serial.print("-");
    // Serial.println(endTrigMinute);


    //Display the details, one on each row
    for (int i = 1; i < maxDisplayDetails; i++) {
      int cursorCol = (maxDisplayLength - strlen(details[i])) / 2;
      // Serial.println(details[i]);
      lcd.setCursor(cursorCol, i);
      lcd.print(details[i]);
    }

    // Deactivate flag that indicates a completely received data
    receiveDetailsComplete = false;
    scheduleDetailArrived = true;
  }

  // When a guide is received completely, clear the screen, extract the timestamp of that schedule and display the guide
  if (receiveGuideComplete == true) {

    // Get time stamp and compare with current time
    // If within 5 minutes, display, if not, ignore.
    
    strcpy(visitorTimeStamp, visitorGuide[3]);
    // Serial.println(visitorGuide[3]);
    splitTimeStamp(visitorTimeStamp, &visitorHourStamp, &visitorMinuteStamp, &visitorSecondStamp);
    Serial.print(visitorHourStamp);
    Serial.print(":");
    Serial.print(visitorMinuteStamp);
    Serial.print(":");
    Serial.println(visitorSecondStamp);

    if ((visitorHourStamp == currentHour) && ((currentMinute - visitorMinuteStamp) <= guideThresholdMinute)) {
      guideOn = true;

      //Clear the display
      lcd.clear();
      // delay(500);

      schedulePaused = true;
      guideTimeStart = millis();
      //Display the guide, one on each row
      int cursorCol = (maxDisplayLength - strlen(visitorGuide[1])) / 2;
      Serial.println(visitorGuide[1]);
      lcd.setCursor(cursorCol, 1);
      lcd.print(visitorGuide[1]);

      if (strcmp(visitorGuide[2], "Right") == 0) {
        Serial.print("Kindly follow arrow  ");
        // Serial.println(visitorGuide[2]);
        Serial.println("------------>>>>");
        lcd.setCursor(0, 2);
        lcd.print("Kindly follow arrow");
        lcd.setCursor(2, 3);
        lcd.print("------------>>>>");
      } else if (strcmp(visitorGuide[2], "Left") == 0) {
        Serial.print("Kindly follow arrow  ");
        // Serial.println(visitorGuide[2]);
        Serial.println("<<<<------------");
        lcd.setCursor(0, 2);
        lcd.print("Kindly follow arrow");
        lcd.setCursor(2, 3);
        lcd.print("<<<<------------");
      }
    }


    // Deactivate flag that indicates a completely received data
    receiveGuideComplete = false;
    visitorGuideArrived = true;
  }
}




// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  isDataReceived = true;
  // lcd.clear();
  // char dataReceived[howMany + 1];
  char dataReceived[maxDisplayLength];
  int i = 0, j = 0;
  char flagVal;
  while (0 < Wire.available()) {
    char c = Wire.read(); /* receive byte as a character */
    // Serial.print(c);
    if (j == 0) {
      flagVal = c;
      j++;
    } else {
      dataReceived[i] = c;
      i++;
    }
    // i++; /* print the character */
  }
  dataReceived[i] = '\0';
  // if (receivedCount == 0 && dataReceived[0] == '1'){
  //   isScheduleDetail = false;
  // }
  if (flagVal == '1') {
    // Copy received data into details array
    strcpy(details[receiveCount], dataReceived);

    if (receiveCount == 0) {
      strcpy(currentDay, dataReceived);
    }
    if (visitorReceiveCount == 0) {
      strcpy(currentDay, dataReceived);
    }

    // Increment the counter
    receiveCount++;
  } else if (flagVal == '0') {
    // Copy received data into date array
    strcpy(currentDay, dataReceived);
  } else if (flagVal == '2') {
    // Copy visitor's guide data into visitor guide array
    // Copy received data into details array
    strcpy(visitorGuide[visitorReceiveCount], dataReceived);
    visitorReceiveCount++;
  }

  if (receiveCount == maxDisplayDetails) {
    // courseDuration = dataForDisplay;
    receiveCount = 0;
    receiveDetailsComplete = true;
  }
  if (visitorReceiveCount == maxDisplayDetails) {
    // courseDuration = dataForDisplay;
    visitorReceiveCount = 0;
    receiveGuideComplete = true;
  }
}

// function that executes whenever data is requested from master
void requestEvent() {
  Wire.write("Hello NodeMCU"); /*send string on request */
}


//Split String value of the time into its integer values
void SplitCurrentTime(char* timeStr, int* hourVal, int* minuteVal, int* secondVal) {
  char* v;
  int val[3];
  char* p;
  int i = 0;
  p = strtok(timeStr, ",");
  while (p) {
    p = strtok(NULL, ":");
    v = p;
    sscanf(v, "%d", &val[i]);
    // p = strtok(NULL, ".");
    i++;
  }
  *hourVal = val[0];
  *minuteVal = val[1];
  *secondVal = val[2];
}

//Split String value of the time into its integer values
void SplitScheduleTime(char* timeStr, int* hourVal, int* minuteVal) {
  char* v;
  int val[2];
  char* p;
  char* q;
  int i = 0;
  q = strtok(timeStr, " - ");
  q = strtok(NULL, " - ");

  p = strtok(q, ":");

  while (p) {
    v = p;
    sscanf(v, "%d", &val[i]);
    p = strtok(NULL, ".");
    i++;
  }
  *hourVal = val[0];
  *minuteVal = val[1];
}

//Split String value of the time into its integer values
void splitTimeStamp(char* timeStr, int* hourStamp, int* minuteStamp, int* secondStamp) {
  char* v;
  int val[3];
  char* p;
  char* q;
  int i = 0;

  p = strtok(timeStr, ":");

  while (p) {
    v = p;
    sscanf(v, "%d", &val[i]);
    p = strtok(NULL, ":");
    i++;
  }
  *hourStamp = val[0];
  *minuteStamp = val[1];
  *secondStamp = val[2];
}