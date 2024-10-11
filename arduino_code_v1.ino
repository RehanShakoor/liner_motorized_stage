//Including nesessary libraries
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

//defining pin connections for motor
const unsigned char dirPin = A0;
const unsigned char stepPin = A1;
const unsigned char MS1 = A6;
const unsigned char MS2 = A3;
const unsigned char MS3 = A2;

//defining system constants
const unsigned int lead = 1; //in mm
const unsigned int stepsPerRevolution = 200; //for no micro-stepping
const unsigned int distancePerStep = 2 * 1000 / stepsPerRevolution; //in micrometer

/*input read variables for motor control */
unsigned char dir;
unsigned char rounds;
unsigned char microStepping;
unsigned long int distance;

//stage characteristics
const char stageTravel_mm = 6;

//function to read a number from keypad
unsigned int readNumberFromKeypad(unsigned char column_number, unsigned char row_number);

//function to select micro-stepping
void selectMicroStepping(unsigned char n);

//function for mode1 motor control
void mode1_motor_control(unsigned char dir,unsigned char microStepping, unsigned long int distance);

//function for mode2 motor control, here distance is in mm
void mode2_motor_control(unsigned char rounds, unsigned char microStepping, unsigned long int distance);

//modes and main menu display functions
void main_menu();
void mode1(void);
void mode2(void);

//function for LCD display controls
void print_space(unsigned char column, unsigned char row, unsigned char size);
void read_write_lcd(unsigned char variable_type);

void setup() {
  //initialising pins
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(MS3, OUTPUT);
  
  //initialising LCD
  lcd.begin(20,4,LCD_5x8DOTS);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.cursor();
  lcd.blink();

  Serial.begin(9600);
}

void loop() 
{
  main_menu();
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
  lcd.setCursor(7,0);
  lcd.print("Mode 1");
  
  read_write_lcd(1);
} 

void mode2(void)
{
  lcd.clear();
  lcd.setCursor(7,0);
  lcd.print("Mode 2");
  
  read_write_lcd(2);
}

void read_write_lcd(unsigned char variable_type)
{
  print_space(0, 1, lcd_columns_count - 1, lcd_rows_count - 1);

  if(variable_type == 1)
  {
    lcd.setCursor(0,1);
    lcd.print("Dir   (0/1):");
    lcd.setCursor(0,2); 
    lcd.print("uStep(1-16):");
    lcd.setCursor(0,3);
    lcd.print("Dist(um)   :");

    dir = (char) readNumberFromKeypad(12,1);
    while(!(dir == 0 || dir == 1))
    {
      dir = (char) readNumberFromKeypad(12,1);
    }

    microStepping = (unsigned char) readNumberFromKeypad(12,2);
    while(!((microStepping >= 1 || microStepping <= 16) and (microStepping % 2 == 0)))
    {
      microStepping = (unsigned char) readNumberFromKeypad(12,2);
    }

    distance = readNumberFromKeypad(12,3);
    while(!(distance >= 0 && distance <= 10000))
    {
      distance = readNumberFromKeypad(12,3);
    }    
        
    print_space(0, 1, lcd_columns_count - 1, lcd_rows_count - 1);
    
    lcd.setCursor(0,1);
    lcd.print("StepSize (um) :");
    lcd.print(distance);

    lcd.setCursor(0,2);
    lcd.print("Press '0' for step");
    lcd.setCursor(0,3);
    lcd.print("Press '1' to exit");
    
    char choice = 0;
    readChoice : choice = readNumberFromKeypad(17, 3);
    
    if(choice == 0)
    {
      //move motor in mode1
      mode1_motor_control(dir, microStepping, distance);
      
      goto readChoice;
    }
    else if(choice == 1)
    {
      //exit from mode 1
      return;
    }
    else
    {
      //do nothing
    }
    
  }
    
  else if(variable_type == 2)
  {
    lcd.setCursor(0,1);
    lcd.print("Rounds     :");
    lcd.setCursor(0,2); 
    lcd.print("uStep(1-16):");
    lcd.setCursor(0,3);
    lcd.print("Dist (mm)  :");

    rounds = (char) readNumberFromKeypad(12,1);
    //putting no limits on number of rounds
    
    microStepping = (unsigned char) readNumberFromKeypad(12,2);
    while(!(microStepping >= 1 || microStepping <= 16) and (microStepping % 2 == 0))
    {
      microStepping = (unsigned char) readNumberFromKeypad(12,2);
    }

    distance = readNumberFromKeypad(12,3);
    while(!(distance >= 0 && distance <= 100))
    {
      distance = readNumberFromKeypad(12,3);
    }

    //move motor in mode2
     mode2_motor_control(rounds, microStepping, distance);
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
  lcd.setCursor(0,2);
  lcd.print("Mode 2");
  lcd.setCursor(0,3);
  lcd.print("Enter mode :");

  unsigned char choice = (unsigned char) readNumberFromKeypad(12, 3);

  while(!(choice >= 1 && choice <= 2))
  {
    choice = (unsigned char) readNumberFromKeypad(12, 3);
  }

  if(choice == 1)
  {
    mode1();
  }
  else if(choice == 2)
  {
    mode2();
  }
  else
  {
    //do nothing
  }
}

void selectMicroStepping(unsigned char n)
{
  switch (n)
  {
    case 1:
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, LOW);
      digitalWrite(MS3, LOW);
    break;

    case 2:
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, LOW);
      digitalWrite(MS3, LOW);
    break;

    case 4:
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, HIGH);
      digitalWrite(MS3, LOW);
    break;

    case 8:
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, HIGH);
      digitalWrite(MS3, LOW);
    break;

    case 16:
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, HIGH);
      digitalWrite(MS3, HIGH);
    break;

    default:
    break;
  }
  
}

//function for mode1 motor control
void mode1_motor_control(unsigned char dir,unsigned char microStepping, unsigned long int distance)
{
  unsigned const int stepsPerSecond = 500;
  unsigned int pulseTimePeriod = 1000 / stepsPerSecond; //in milliseconds
  unsigned long int totalSteps = distance * microStepping / distancePerStep; //distance is in micro-meter

  digitalWrite(dirPin, dir);
  selectMicroStepping(microStepping);
  
 for(unsigned long i = 1; i <=totalSteps ; i++)
 {
  digitalWrite(stepPin, LOW);
  delay(pulseTimePeriod / 2);
          
  digitalWrite(stepPin, HIGH);
  delay(pulseTimePeriod / 2);
 }
}

//function for mode2 motor control
void mode2_motor_control(unsigned char rounds, unsigned char microStepping, unsigned long int distance)
{
  unsigned const int stepsPerSecond = 500;
  unsigned int pulseTimePeriod = 1000 / stepsPerSecond; //in milliseconds
  unsigned long int totalSteps = (distance * 1000) * microStepping / distancePerStep; //distance is in micro-meter

  selectMicroStepping(microStepping);

  for(unsigned char r = 1; r <= rounds; r++)
  {
    digitalWrite(dirPin, 0);
    
    for(unsigned long i = 1; i <=totalSteps ; i++)
    {
     digitalWrite(stepPin, LOW);
     delay(pulseTimePeriod / 2);
            
     digitalWrite(stepPin, HIGH);
     delay(pulseTimePeriod / 2);
    }
  
    digitalWrite(dirPin, 1);
    
    for(unsigned long i = 1; i <=totalSteps ; i++)
    {
     digitalWrite(stepPin, LOW);
     delay(pulseTimePeriod / 2);
             
     digitalWrite(stepPin, HIGH);
     delay(pulseTimePeriod / 2);
    }
  }
}
