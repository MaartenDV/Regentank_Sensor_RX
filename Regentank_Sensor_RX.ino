//COM 61
//arduino UNO


//************ INCLUDES ************
#include <RH_ASK.h>
#include <LiquidCrystal.h>
#ifdef RH_HAVE_HARDWARE_SPI
  #include <SPI.h> // Not actually used but needed to compile
#endif


//************ DEFINES ************
#define ANALOG_PIN  0
#define PIN_RX    2   
#define PIN_TX    3   //unused
#define PIN_d4    4 
#define PIN_d5    5 
#define PIN_d6    6 
#define PIN_d7    7
#define PIN_RS    8
#define PIN_EN    9 
#define PIN_BL    10 

//#define lcd_brightness      125
#define TASK_DELAY_MS         100
#define PACKET_BYTE_LENGTH    5
#define PACKET_REV            2
//#define TASK_DELAY_MS         1000

#define TASK_BUTTON_MILI      5

//rain well parameters
//#define OUTFLOW_HEIGHT_MM           1500    //max water height
//#define MUD_THICKNESS_MM            100     //mud in well height
//#define SENSOR_HEIGHT_MM            1700    //height sensor above empty well
//#define ULTRASONIC_DEPTH_SAMPLES    32

//lcd keypad defines
#define BTN_RIGHT  0
#define BTN_UP     1
#define BTN_DOWN   2
#define BTN_LEFT   3
#define BTN_SELECT 4
#define BTN_NONE   5
#define BTN_UNCLEAR   6   //most likely during a debounce moment

#define MAIN_PAGE     0
#define PAGE_1        1
#define PAGE_2        2

#define DEBOUNCE_LENGTH   8


//************ VARIABLES ************
RH_ASK driver(2000, PIN_RX, PIN_TX, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85), 
LiquidCrystal lcd(PIN_RS,  PIN_EN,  PIN_d4,  PIN_d5,  PIN_d6,  PIN_d7);
unsigned long button_task_millis;
uint8_t button_state;
uint8_t buttons_debounce_array[DEBOUNCE_LENGTH];
uint8_t previous_buttons_state;
bool bool_button_state_changed;

//unsigned long previousMillis = 0;
//unsigned long currentMillis = 0;
//word ultrasonic_depth; //unsigned 16bit
//float ds18b20_temperature; //signed 16 bit
//uint8_t tx_packet[PACKET_BYTE_LENGTH];
//word last_update_seconds = 0;
//word max_last_update_seconds = 0;
//averaging variables
//unsigned long ultrasonic_depth_samples_array[ULTRASONIC_DEPTH_SAMPLES];
//unsigned long average_ultrasonic_depth = 0;

//lcd variables
//uint16_t number_of_samples = 0;
uint8_t page_selected = MAIN_PAGE;

//button variables
uint16_t adc_key_in  = 0;


//************ FUNCTIONS ************
void(* resetFunc) (void) = 0;//declare reset function at address 0


//******* FUNCTION DISCIPTION *******
void do_button_task(void);
void enabel_lcd_backlight(void);
uint8_t convert_analog_to_button(uint16_t);
uint8_t debounce_button_state(uint8_t);
bool button_state_ready(void);


// ***************************************
//                 SETUP
//****************************************
void setup() {

  //KEEPOUT START
  #ifdef RH_HAVE_SERIAL
      Serial.begin(9600);    // Debugging only
  #endif
      if (!driver.init())
  #ifdef RH_HAVE_SERIAL
           Serial.println("init failed");
  #else
    ;
  #endif
  //KEEPOUT END

  //lcd
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Rain Level Meter");
  lcd.setCursor(0,1);
  lcd.print("Started...   v01");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for data...");

  //init buttons
  button_task_millis = millis();
  button_state = BTN_NONE;
  previous_buttons_state = BTN_NONE;
  bool_button_state_changed = false;
  for(int i = 0; i < DEBOUNCE_LENGTH; i++)  {//fil the debounce array
    buttons_debounce_array[i] = BTN_NONE;
  }

  //init LCD
  enabel_lcd_backlight();
}



// ***************************************
//                  MAIN
//****************************************
void loop() {

  // button task
  if ((millis() - button_task_millis) >=  TASK_BUTTON_MILI) {
    do_button_task();
    button_task_millis = millis();
  }
  
}



// ***************************************
//             TASKS button
//****************************************
void do_button_task() {
  uint16_t temp_analog_button_read;
  uint8_t temp_button_undebounced;
  previous_buttons_state = button_state;
  
  temp_analog_button_read = analogRead(ANALOG_PIN);
  temp_button_undebounced = convert_analog_to_button(temp_analog_button_read);
  button_state = debounce_button_state(temp_button_undebounced);

  if (button_state_ready() == true) {
    if (previous_buttons_state != button_state) {
      bool_button_state_changed = true;
      Serial.print("The NEW button state is : ");
      Serial.println(button_state);
    }
  }
}



// ***************************************
//            basic FUNCTIONS
//****************************************
void enabel_lcd_backlight() {
  digitalWrite(PIN_BL, HIGH);
}

// read the buttons
uint8_t convert_analog_to_button(uint16_t adc_key_in) {  
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return BTN_NONE; // We make this the 1st option for speed reasons since it will be the most likely result
  // For V1.1 use this threshold
  if (adc_key_in < 50)   return BTN_RIGHT;  
  if (adc_key_in < 200)  return BTN_UP; 
  if (adc_key_in < 400)  return BTN_DOWN; 
  if (adc_key_in < 600)  return BTN_LEFT; 
  if (adc_key_in < 800)  return BTN_SELECT;
}  

//debounce the buttons
uint8_t debounce_button_state(uint8_t button_undebounced) {
  //shift all values one position
  for (uint8_t i = (DEBOUNCE_LENGTH - 1) ; i > 0; i--) {
    buttons_debounce_array[i] = buttons_debounce_array[i - 1];  //shift values in array one place
  }
  //fill in new button
  buttons_debounce_array[0] = button_undebounced;
  
  //debounce and place the necessary flags (by checking if the first byte occurs in the intire debounce buffer)
  for(uint8_t j = 1; j < DEBOUNCE_LENGTH; j++) {
    if(buttons_debounce_array[0] != buttons_debounce_array[j]) {
      return BTN_UNCLEAR;
      break;
    }   
  }
  return buttons_debounce_array[0];
}

//check if valid button detected
bool button_state_ready() {
  return (button_state != BTN_UNCLEAR);
}
/*
  // check for incomming transmition
  if (driver.recv(tx_packet, PACKET_BYTE_LENGTH)) { // Non-blocking
  
    // Message with a good checksum received, dump it.
    driver.printBuffer("Got:", tx_packet, PACKET_BYTE_LENGTH);

    //check if valid frame version received
    if (tx_packet[0] == PACKET_REV) {
    
    //convert the transmitted number to value
    ultrasonic_depth = ((tx_packet[1] << 8) | tx_packet[2]);
    ds18b20_temperature = ((tx_packet[3] << 8) | tx_packet[4]);
    ds18b20_temperature = ds18b20_temperature / 100;

    //perform averaging
    average_ultrasonic_depth = 0;
    for (uint8_t i = (ULTRASONIC_DEPTH_SAMPLES - 1) ; i > 0; i--) {
      ultrasonic_depth_samples_array[i] = ultrasonic_depth_samples_array[i - 1];  //shift values in array one place
      average_ultrasonic_depth += ultrasonic_depth_samples_array[i];  //meanwhile, add all values together
    }
    ultrasonic_depth_samples_array[0] = (unsigned long) ultrasonic_depth;
    average_ultrasonic_depth += ultrasonic_depth_samples_array[0];  //add last value together
    average_ultrasonic_depth /= ULTRASONIC_DEPTH_SAMPLES; //devide by number of values in array -> average! :-) 

    //change visualisation variables
    if (number_of_samples < ULTRASONIC_DEPTH_SAMPLES) {
      number_of_samples++;
    }
    
    //print out for debugging
    Serial.print("The Distance is : ");
    Serial.print(ultrasonic_depth);
    Serial.println(" mm");
    Serial.print("The Temperature is : ");
    Serial.print(ds18b20_temperature);
    Serial.println(" °C");

    //put on display
    //moved to systick 

    //read buttons;
    //lcd_key = read_LCD_buttons();
    
    //other stuff
    last_update_seconds = 0;
    
    //end
    }
  }
    
  currentMillis = millis();
  if (currentMillis - previousMillis >= TASK_DELAY_MS) {

    //display the seconds after the last transmission
    if (last_update_seconds < 999 ) {
      last_update_seconds++;
    } else {
      resetFunc(); //call reset
    }
    
    if (max_last_update_seconds < last_update_seconds) {
      max_last_update_seconds = last_update_seconds;
    }

    switch (page_selected) {
      case MAIN_PAGE:
        if (number_of_samples < ULTRASONIC_DEPTH_SAMPLES) {
          lcd.clear();
          lcd.setCursor(0, 0);    //collumn - row
          lcd.print("S: "); 
          lcd.print(number_of_samples);
          lcd.print("/");
          lcd.print(ULTRASONIC_DEPTH_SAMPLES);
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);    //collumn - row
          lcd.print("D: "); 
          lcd.print(average_ultrasonic_depth);
          lcd.print("mm"); 
        }
        lcd.setCursor(0, 1);    //collumn - row
        lcd.print("T: "); 
        lcd.print(ds18b20_temperature);
        lcd.print((char)223);
        lcd.print("C");
        lcd.setCursor(12, 0);    //collumn - row
        lcd.print("d");
        lcd.print(last_update_seconds);
        lcd.setCursor(11, 1);    //collumn - row
        lcd.print("md");
        lcd.print(max_last_update_seconds);
        break;
    
      case PAGE_1:
        if (number_of_samples < ULTRASONIC_DEPTH_SAMPLES) {
          lcd.clear();
          lcd.setCursor(0, 0);    //collumn - row
          lcd.print("S: "); 
          lcd.print(number_of_samples);
          lcd.print("/");
          lcd.print(ULTRASONIC_DEPTH_SAMPLES);
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);    //collumn - row
          lcd.print("fR: "); 
          lcd.print(((SENSOR_HEIGHT_MM - average_ultrasonic_depth) - MUD_THICKNESS_MM) / (OUTFLOW_HEIGHT_MM / 100));
          lcd.print("%"); 
        }
          lcd.setCursor(0, 1);    //collumn - row
          lcd.print("T: "); 
          lcd.print(ds18b20_temperature);
          lcd.print((char)223);
          lcd.print("C");
          lcd.setCursor(12, 0);    //collumn - row
          lcd.print("d");
          lcd.print(last_update_seconds);
          lcd.setCursor(11, 1);    //collumn - row
          lcd.print("md");
          lcd.print(max_last_update_seconds);
        break;
    
      case PAGE_2:
        lcd.clear();
        lcd.setCursor(0, 0);    //collumn - row
        lcd.print("Page 2 ");
        break;
    
      default:
        page_selected = MAIN_PAGE;
    }
    previousMillis = currentMillis;
  }
}
*/
