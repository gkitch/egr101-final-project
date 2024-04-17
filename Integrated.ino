#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <RTClib.h>
#include <SparkFun_TB6612.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // Display
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // Temp
RTC_DS1307 rtc; // Clock

int sunExp = 0;
int maxTemp = 80;
int mode = 0;

int count = 0;
int sun[7];
int temps[7];

int buttonUp = 7;
int buttonDown = 6;
int next = 5;

#define AIN1 9
#define AIN2 10
#define PWMA 11
#define STBY 8

const int offsetA = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY);

void setup() {
  // Begin Serial Monitor
  Serial.begin(9600);

  // Initalize Temperature Sensor
  mlx.begin();
  getTempTest(); // test

  // Initalize Clock Module
  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // set current date + time
  getTime();

  // Initalize Motors
  motorTest();

  // ETC
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(next, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("Climate Control!");
  delay(5000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hours of Sun: 0");
  lcd.setCursor(0, 1);
  lcd.print("Day 1");
  delay(1000);
}

void loop() {
  // Mode 0 - Hours of sun
  if (mode == 0) {
    lcd.setCursor(14, 0);
    if (digitalRead(buttonDown) == HIGH) {
      delay(200);
      sunExp--;
      lcd.print(sunExp);
      if (sunExp < 10) {
        lcd.setCursor(15, 0);
        lcd.print(" ");
      }
    }
    if (digitalRead(buttonUp) == HIGH) {
      delay(200);
      sunExp++;
      lcd.print(sunExp);
      if (sunExp < 10) {
        lcd.setCursor(15, 0);
        lcd.print(" ");
      }
    }
  }

  // Mode 1 - Temperature
  if (mode == 1) {
    lcd.setCursor(10, 0);
    lcd.print(maxTemp);
    if (digitalRead(buttonDown) == HIGH) {
      delay(200);
      maxTemp--;
      lcd.print(maxTemp);
      lcd.setCursor(12, 0);
      lcd.print((char)223);
      lcd.print("F");
    }
    if (digitalRead(buttonUp) == HIGH) {
      delay(200);
      maxTemp++;
      lcd.print(maxTemp);
      lcd.setCursor(12, 0);
      lcd.print((char)223);
      lcd.print("F");
    }
  }

  // Mode Change
  if (digitalRead(next) == HIGH) {
    delay(400);
    if (mode == 0) {
      sun[count] = sunExp;
      sunExp = 0;
      mode = 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Max Temp: ");
      lcd.setCursor(10, 0);
      lcd.setCursor(12, 0);
      lcd.print((char)223);
      lcd.print("F");
      lcd.setCursor(0, 1);
      lcd.print("Day " + String(count + 1));
    }
    else if (mode == 1) {
      temps[count] = maxTemp;
      maxTemp = 80;
      count++;
      //Serial.println(count);
      mode = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Hours of Sun: 0");
      lcd.setCursor(0, 1);
      lcd.print("Day " + String(count + 1));
    }
  }
  // Print
  if (count == (sizeof(sun) / sizeof(sun[0]))) {
    for (int i = 0; i < (sizeof(sun) / sizeof(sun[0])); i++) {
      Serial.print("Day " + String(i + 1) + ": Sun Exposure - ");
      Serial.print(sun[i]);
      Serial.print(" hrs");
      Serial.print(", Max Temp - ");
      Serial.print(temps[i]);
      Serial.print("˚F");
      Serial.println();
    }
    count++;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Data set!");
    mode = 2;
  }
  if (count > (sizeof(sun) / sizeof(sun[0]))) {
    int dayNum = 0;
    int state = 0;
    DateTime now = rtc.now();
    int currentDay = now.day();
    float timeSun = 0;

    while (dayNum < (sizeof(sun) / sizeof(sun[0]))) {
      // Check Temperature
      if (getTemp() > temps[dayNum]) {
        if (state == 0) {
          Serial.println("Too hot - closing");
        }
        else if (state == 1) {
          Serial.println("Too hot - keeping closed");
        }
      }
      if (timeSun >= sun[dayNum]) {
        if (state == 0) {
          Serial.println("Sun time reached - closing");
        }
        else if (state == 1) {
          Serial.println("Sun time reached - keeping closed");
        }
      }
      // Temperature Checks, Shade Open, Sun Limit Not Reached - Leave Open
      else if (getTemp() < temps[dayNum] && state == 0 && timeSun < sun[dayNum]) {
        Serial.println("Temp Checks, Max Sun Not Reached - leaving open");
        timeSun += 0.2;
      }
      // Temperature Checks, Shade Open, Sun Limit Reached
      else if (getTemp() < temps[dayNum] && state == 0 && timeSun >= sun[dayNum]) {
        Serial.println("Temp Checks, Max Sun Reached - closing");
      }
      // Temperature Checks, Shade Closed, Sun Limit Not Reached
      else if (getTemp() < temps[dayNum] && state == 0 && timeSun >= sun[dayNum]) {
        Serial.println("Temp Checks, Max Sun NOT Reached - opening");
        timeSun += 0.2;
      }
      Serial.println(timeSun);
      delay(5000);
    }
  }
}

void getTempTest() {
  Serial.print("Ambient Temperature: ");
  Serial.println(String(mlx.readAmbientTempF()) + " ˚F");
  delay(1000);
}

float getTemp() {
  return (mlx.readAmbientTempF());
}

float toHours(int hrs, int mins) {
  float n = hrs + (mins / 60.0);
  return n;
}

void getTime() {
  Serial.println("\nTime Test:");
  DateTime now = rtc.now();
  Serial.print(now.month());
  Serial.print(" / ");
  Serial.print(now.day());
  Serial.print(" / ");
  Serial.println(now.year());
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  delay(1000);
}

void motorTest() {
  Serial.print("\nMotor Test: ");
  Serial.print("Forward");
  motor1.drive(255, 1000);
  delay(1000);
  motor1.drive(0, 1000);
  delay(1000);
  Serial.print(", Backward");
  motor1.drive(-255, 1000);
  delay(1000);
  motor1.drive(0, 1000);
  delay(1000);
  Serial.println("\n");
}
