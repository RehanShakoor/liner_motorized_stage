//Including nesessary libraries
#include <AccelStepper.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

const char keypad_rows_count = 4; //number of rows
const char keypad_columns_count = 4; //number of columns

const char lcd_rows_count = 4; //number of rows
const char lcd_columns_count = 20; //number of columns

char keyMap[keypad_rows_count][keypad_columns_count] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char rowPins[keypad_rows_count] = {2, 3, 4, 5}; //row pin numbers
char colPins[keypad_columns_count] = {6, 7, 8, 9}; //column pin numbers

//creating object for keypad
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, keypad_rows_count, keypad_columns_count); 

//creating object for LCD
LiquidCrystal_I2C lcd(0x27, lcd_columns_count, lcd_rows_count);

String inputString = ""; //stores input from keypad as astring
unsigned int inputNumber = 0; //stores (integers <- string)

// Define pin connections
const int dirPin = A0;
const int stepPin = A1;

// Define motor interface type
#define motorInterfaceType 1
#define M0 A6
#define M1 A3
#define M2 A2

//lead screw and motor characteristics
const char  pitch_mm = 2;
const float stepAngle_degree = 1.8;
float oneStep_um = (pitch_mm / (360 / stepAngle_degree)) * 1000;

/*input read variables*/
char dir1; unsigned char step_mode1;
unsigned char  centimeter1; unsigned char millimeter1; unsigned int micrometer1;
unsigned char time_sec1;

char dir2; unsigned char step_mode2;
unsigned char  centimeter2; unsigned char millimeter2; unsigned int micrometer2;
unsigned char time_sec2;

//stage characteristics
const char stageTravel_mm = 6;

//creating object of class AccelStepper for motor control
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);

unsigned int readNumberFromKeypad(unsigned char column_number, unsigned char row_number);

void moveStage(char dir, unsigned char stepping, unsigned char  cm, unsigned char mm, unsigned int um, unsigned char time_sec);

void mode1(void);
void mode2(void);
void mode3(void);
void mode4(void);

void main_menu();

void print_space(unsigned char column, unsigned char row, unsigned char size);
void read_write_lcd(unsigned char variable_type);

void setup() {
  // set the maximum speed, acceleration factor,
  // initial speed and the target position
  Serial.begin(9600);

  myStepper.setMaxSpeed(10000); //important, if not written then 1 step per sec is set to all  
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);

  lcd.begin(20,4,LCD_5x8DOTS);
  lcd.backlight();
  lcd.clear();
  lcd.cursor();
  lcd.blink();
}

void loop() 
{
  main_menu();
}

void moveStage(char dir, unsigned char stepping, unsigned char  cm, unsigned char mm, unsigned int um, unsigned char time_sec)
{
  lcd.noCursor();
  lcd.clear();

  if(dir == 0)
  {
    lcd.setCursor(2,0);
    lcd.print("Motor Running (F)");
  }
  else if(dir == 1)
  {
    lcd.setCursor(2,0);
    lcd.print("Motor Running (B)");
  }
  else
  {
    //do nothing
  }
  
  long steps = 0;

  oneStep_um = oneStep_um / stepping;  //updating one step movement according to microstepping chosen

  if(stepping == 1 || stepping == 2)
  {
    if(stepping == 1) //changing M0 M1 M2 voltage level for microstepping
    {
      digitalWrite(M0, LOW);
      digitalWrite(M1, LOW);
      digitalWrite(M2, LOW);
    }
    else //changing M0 M1 M2 voltage level for microstepping
    {
      digitalWrite(M0, HIGH);
      digitalWrite(M1, LOW);
      digitalWrite(M2, LOW);
    }

    unsigned char a = um / oneStep_um, b = um % (int)oneStep_um; //finding a and b

    if(b > (int)oneStep_um / 2) //checking b and updating a
    {
      a = a + 1;
    }

    um = a * oneStep_um; //updating um

    steps = (um + mm*1000 + cm*10000) / oneStep_um; //calculating total steps required
  }
  
  if(dir == 0) //forward
  {
    myStepper.moveTo(myStepper.currentPosition() - 1 * steps);
  }
  else if(dir == 1) //backward
  {
    myStepper.moveTo(myStepper.currentPosition() + 1 * steps);
  }
  
  myStepper.setSpeed((float) steps / time_sec);

  Serial.print(steps);
  Serial.print(" ");
  Serial.print(time_sec);
  Serial.print(" ");  
  Serial.println((float) steps / time_sec);

  lcd.setCursor(0,1);
  lcd.print("Steps   :");
  lcd.setCursor(0,2);
  lcd.print("um      :");
  lcd.setCursor(0,3);
  lcd.print("mm      :");

  long time = millis();
  long abs_distanceToGo = abs(myStepper.distanceToGo());
  long abs_steps = abs(steps);

  long temp1 = abs_steps - abs_distanceToGo;

  while(abs_distanceToGo != 0)
  {
    myStepper.runSpeedToPosition();

    abs_distanceToGo = abs(myStepper.distanceToGo());
    temp1 = abs_steps - abs_distanceToGo;

    lcd.setCursor(9,1);
    lcd.print(temp1);
    lcd.setCursor(9,2);
    lcd.print(temp1 * oneStep_um);
    lcd.setCursor(9,3);
    lcd.print(temp1 * oneStep_um / 1000.0);
  } 
  
  if(dir == 0)
  {
    lcd.setCursor(2,0);
    lcd.print("Motor Stopped (F)");
  }
  else if(dir == 1)
  {
    lcd.setCursor(2,0);
    lcd.print("Motor Stopped (B)");
  }
  else
  {
    //do nothing
  }

  oneStep_um = oneStep_um * stepping;  //updating one step movement according to microstepping chosen

  lcd.cursor();
  lcd.blink();

  delay(2000);
}

unsigned int readNumberFromKeypad(unsigned char column_number, unsigned char row_number)
{
  char key = 0;
  readAgain : inputString = "";  //clearing inputString variable

  print_space(column_number, row_number, lcd_columns_count - 1, row_number); //clearing integer diaplay area

  key = keypad.waitForKey(); //reading key (will wait untill a key is pressed)

  if(!(key >= '0' && key <= '9')) //check if non numeric key is pressed
  {
    lcd.setCursor(column_number, row_number);
    lcd.print(key);
    delay(500);
    lcd.setCursor(column_number, row_number);
    lcd.print("NA");
    delay(500);
    print_space(column_number, row_number, lcd_columns_count - 1, row_number);
    goto readAgain;
  }

  lcd.setCursor(column_number, row_number);
  lcd.print(key);

  inputString += key; //appending entered character to inputString
  key = keypad.waitForKey();

  while(key != '*') //read untill '*' is not pressed
  {
    if(key >= '0' && key <= '9')
    {
      lcd.print(key);
      inputString += key; //appending entered character to inputString
    }
    else if(key == '#') //cancel is pressed, re-read for same input variable
    {
      print_space(column_number, row_number, lcd_columns_count - 1, row_number);
      goto readAgain;
    }
    else
    {
      print_space(column_number, row_number, lcd_columns_count - 1, row_number);
      lcd.print("NA");
      delay(500);
      print_space(column_number, row_number, lcd_columns_count - 1, row_number);
      goto readAgain;
    }
      
    key = keypad.waitForKey();
  }

  if(inputString.length() > 0)
  {
    inputNumber = inputString.toInt();  //converting string to integer
    return inputNumber;
  }
}

void mode1(void)
{
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Mode 1 (page1)");
  
  read_write_lcd(1);
  moveStage(dir1, step_mode1, centimeter1, millimeter1, micrometer1, time_sec1);
} 

void mode2(void)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 2  (1st motion)");

  read_write_lcd(1);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 2  (2nd motion)");

  read_write_lcd(2);

  moveStage(dir1, step_mode1, centimeter1, millimeter1, micrometer1, time_sec1);
  moveStage(dir2, step_mode2, centimeter2, millimeter2, micrometer2, time_sec2);
}

void mode3(void)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 3  (1st motion)");

  read_write_lcd(1);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 3  (2nd motion)");

  read_write_lcd(2);

  while(1)
  {
    moveStage(dir1, step_mode1, centimeter1, millimeter1, micrometer1, time_sec1);
    moveStage(dir2, step_mode2, centimeter2, millimeter2, micrometer2, time_sec2);
  }
}

void mode4(void)
{
  lcd.cursor();
  lcd.noCursor();
  
  while(1)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Mode4 Vibration Test");
    lcd.setCursor(3,1);
    lcd.print("Low vibration");
    lcd.setCursor(0,2);
    lcd.print("1/16 step resolution");
    lcd.setCursor(0,3);
    lcd.print("Moving 1 cm backward");
    
    //16 microstepping
    digitalWrite(M0, HIGH);
    digitalWrite(M1, HIGH);
    digitalWrite(M2, HIGH);
  
    myStepper.moveTo(myStepper.currentPosition() + 16000);
    myStepper.setSpeed(3200);
  
    while(myStepper.distanceToGo() != 0)
    {
      myStepper.runSpeedToPosition();
    }
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Mode4 Vibration Test");
    lcd.setCursor(3,1);
    lcd.print("High vibration");
    lcd.setCursor(0,2);
    lcd.print("Full step resolution");
    lcd.setCursor(0,3);
    lcd.print("Moving 1 cm  Forward");
  
    //no microstepping
    digitalWrite(M0, LOW);
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
  
    myStepper.moveTo(myStepper.currentPosition() - 1000);
    myStepper.setSpeed(200);
  
    while(myStepper.distanceToGo() != 0)
    {
      myStepper.runSpeedToPosition();
    }
  }

  lcd.cursor();
  lcd.blink();
}

void read_write_lcd(unsigned char variable_type)
{
  print_space(0, 1, lcd_columns_count - 1, lcd_rows_count - 1);

  if(variable_type == 1)
  {
    lcd.setCursor(0,1);
    lcd.print("Direction [0/1]:");
    lcd.setCursor(0,2);
    lcd.print("Stepping  [1/2]:");
    lcd.setCursor(0,3);
    lcd.print("Travel Time (s):");

    dir1 = (char) readNumberFromKeypad(16,1);
    while(!(dir1 == 0 || dir1 == 1))
    {
      dir1 = (char) readNumberFromKeypad(16,1);
    }

    step_mode1 = (unsigned char) readNumberFromKeypad(16,2);
    while(!(step_mode1 == 1 || step_mode1 == 2))
    {
      step_mode1 = (unsigned char) readNumberFromKeypad(16,2);
    }

    time_sec1 = (unsigned char) readNumberFromKeypad(16,3);

    print_space(0, 1, lcd_columns_count - 1, lcd_rows_count - 1);
    
    lcd.setCursor(0,1);
    lcd.print("cm  [0-6]:");
    lcd.setCursor(0,2);
    lcd.print("mm  [0-9]:");
    lcd.setCursor(0,3);
    lcd.print("um[0-999]:");
    
    centimeter1 = (unsigned char) readNumberFromKeypad(10,1);
    while(!(centimeter1 >= 0 && centimeter1 <= 6))
    {
      centimeter1 = (unsigned char) readNumberFromKeypad(10,1);
    }

    millimeter1 = (unsigned char) readNumberFromKeypad(10,2);
    while(!(millimeter1 >= 0 && millimeter1 <= 9))
    {
      millimeter1 = (unsigned char) readNumberFromKeypad(10,2);
    }

    micrometer1 = (unsigned char) readNumberFromKeypad(10,3);
    while(!(micrometer1 >= 0 && micrometer1 <= 999))
    {
      micrometer1 = (unsigned char) readNumberFromKeypad(10,3);
    }

  }
  else if(variable_type == 2)
  {
    lcd.setCursor(0,1);
    lcd.print("Direction [0/1]:");
    lcd.setCursor(0,2);
    lcd.print("Stepping  [1/2]:");
    lcd.setCursor(0,3);
    lcd.print("Travel Time (s):");

    dir2 = (char) readNumberFromKeypad(16,1);
    while(!(dir2 == 0 || dir2 == 1))
    {
      dir2 = (char) readNumberFromKeypad(16,1);
    }

    step_mode2 = (unsigned char) readNumberFromKeypad(16,2);
    while(!(step_mode2 == 1 || step_mode2 == 2))
    {
      step_mode2 = (unsigned char) readNumberFromKeypad(16,2);
    }

    time_sec2 = (unsigned char) readNumberFromKeypad(16,3);

    print_space(0, 1, lcd_columns_count - 1, lcd_rows_count - 1);
    
    lcd.setCursor(0,1);
    lcd.print("cm  [0-6]:");
    lcd.setCursor(0,2);
    lcd.print("mm  [0-9]:");
    lcd.setCursor(0,3);
    lcd.print("um[0-999]:");
    
    centimeter2 = (unsigned char) readNumberFromKeypad(10,1);
    while(!(centimeter2 >= 0 && centimeter2 <= 6))
    {
      centimeter2 = (unsigned char) readNumberFromKeypad(10,1);
    }

    millimeter2 = (unsigned char) readNumberFromKeypad(10,2);
    while(!(millimeter2 >= 0 && millimeter2 <= 9))
    {
      millimeter2 = (unsigned char) readNumberFromKeypad(10,2);
    }

    micrometer2 = (unsigned char) readNumberFromKeypad(10,3);
    while(!(micrometer2 >= 0 && micrometer2 <= 999))
    {
      micrometer2 = (unsigned char) readNumberFromKeypad(10,3);
    }
  }
  else
  {
    //do nothing
  }
}

void print_space(unsigned char c1, unsigned char r1, unsigned char c2, unsigned char r2)
{
  unsigned char r = r1, c = c1;

  lcd.noCursor();
  lcd.setCursor(c1, r1);

  while(!(c == c2 + 1 && r == r2))
  {
    if(c >= lcd_columns_count)
    {
      c = 0; r++;
    }

    if(r >= lcd_rows_count)
    {
      r = 0; c = 0;
    }

    lcd.setCursor(c, r);
    lcd.print(" "); c++;
  }
  
  lcd.setCursor(c1, r1);
  lcd.cursor();
  lcd.blink();
}

void main_menu()
{
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Select Mode");
  lcd.setCursor(0,1);
  lcd.print("Mode 1");
  lcd.setCursor(14,1);
  lcd.print("Mode 2");
  lcd.setCursor(0,2);
  lcd.print("Mode 3");
  lcd.setCursor(14,2);
  lcd.print("Mode 4");
  lcd.setCursor(3,3);
  lcd.print("Enter mode :");

  unsigned char choice = (unsigned char) readNumberFromKeypad(15, 3);

  while(!(choice >= 1 && choice <= 4))
  {
    choice = (unsigned char) readNumberFromKeypad(15, 3);
  }

  if(choice == 1)
  {
    mode1();
  }
  else if(choice == 2)
  {
    mode2();
  }
  else if(choice == 3)
  {
    mode3(); 
  }
  else if(choice == 4)
  {
    mode4();
  }
  else
  {
    //do nothing
  }
}
