#define BOARD       "DE10-Standard"

//Cyclone V FGPA Devices
#define LED_BASE	      0xFF200000  // 10 LEDS (0-3 for battery))
#define HEX3_HEX0_BASE        0xFF200020  // Last 4 7 Seg Displays
#define HEX5_HEX4_BASE        0xFF200030  // +/- Display (only -)
#define SW_BASE               0xFF200040  // On/Off (mapped to switches 0) (1-9 unused) and C/F Keys (mapped to key 2) (3 unused)
#define KEY_BASE              0xFF200050  // Up/Down Keys (mapped to key 0,1) 
#define Device_Heater         0x40000000  // Heater, instead we use LEDS (LED 4, 5)
#define Device_Cooler         0x40000020  // Cooler, instead we use LEDS (LED 6, 7)
#define Sensor_Thermistor     0x40000040  // Not used, instead a user enters the 'Actual temp' value, *** Line 35 
#define Sensor_Battery        0xFF200040  // Not used, instead we simulate it using a user defined battery level, *** Line 31

volatile int *LED_ptr = (int *)LED_BASE;
volatile int *HEX_ptr = (int *)HEX3_HEX0_BASE;
volatile int *HEX1_ptr = (int *)HEX5_HEX4_BASE;
volatile int *SW_ptr = (int *)SW_BASE;
volatile int *KEY_ptr = (int *)KEY_BASE;
volatile int *HTR_ptr = (int *)Device_Heater;
volatile int *CLR_ptr = (int *)Device_Cooler;
volatile int *TMR_ptr = (int *)Sensor_Thermistor;
volatile int *BLVL_ptr =  (int *)Sensor_Battery;

unsigned char lookuptable[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; //

int main() 
{
  // INITIALIZATION
  int BatteryVal = 70; // *** Remaining battery life as a percentage
  int CorF = 0; // Celsius(C) Mode is 0 and Fahrenheit(F) Mode is 1, (default is C)
  int Current = 0x39; // Initialized to display 'C'
  *(HEX_ptr) |= 0x3F3F3F39; // Initialized to 000 C (x6)
  int  TempAct = 10; // *** The actual temp read by 'thermistor' we set
  int  TempSet = 25; // Default set temp (to ompare with actual temp)
  int Hundred = 0, Ten = 2, One = 5; // Set temperature numbers (to set with code)

  // LOOP
  while(1)
  {
    if(*SW_ptr & 0x1) // On/Off switch. By defaults its ON
    {
      *(HEX_ptr) = 0x00000000; // Clears Hex displays
      *(HEX1_ptr) = 0x0000;
      *(LED_ptr) = 0x0; // Clear bit 0, 1, 2 and 3
      while(*SW_ptr & 0x1){}
    }

    /*power level: displays 4 LEDs (each represent 25%) */
    if(BatteryVal >= 1 && BatteryVal <= 25) // If 1-25% only one light is ON
    {
      *(LED_ptr) |= 0x8; // Set bits 3
      *(LED_ptr) &= 0x8; // Clear bit 0, 1 and 2
    }
    else if(BatteryVal >= 26 && BatteryVal <= 50) //If 26-50% two lights are ON, SW bit 2 
    {
      *(LED_ptr) |= 0xC; // Set bits 2 and 3
      *(LED_ptr) &= 0xC; // Clear bits 0 and 1
    }
    else if(BatteryVal >= 51 && BatteryVal <= 75) //If 51-75% three lights are ON, SW bit 3
    {
      *(LED_ptr) |= 0xE; // Set bits 1, 2 and 3
      *(LED_ptr) &= 0xE; // Clear bit 0
    }
    else if(BatteryVal >= 76 && BatteryVal <= 100) //If 76-100% all four lights are ON, SW bit 2 and 3
    { 
      *(LED_ptr) |= 0xF; // Set bits 0, 1, 2 and 3
    }
  
    //Both the coolers and heaters are either on (high) or off (low) and turn on the cooling or heating LED_ptr too
    if(TempSet < TempAct) //actual temperature is higher than set temperature turn cooler on; heater off
    {
      *LED_ptr &= 0x3F; //Clear Heater LED & Turn off Heater
      *LED_ptr |= 0x30; //Set Cooler LED & Turn on Cooler 
    }
    else if (TempSet > TempAct) //actual temperature is lower than set temperature turn heater on; cooler off
    {
      *LED_ptr &= 0xCF; //Clear Cooler LED & Turn off Cooler
      *LED_ptr |= 0xC0; //Set Heater LED & Turn on Heater 
    } 
    else
    {
        *LED_ptr &= 0x0F; //Clear Cooler & Heater LEDs, Turn off Cooler and Heater
    }
    
    //7-segment display showing temperature
    *HEX_ptr = Current; //Displays either C or F based on mode
    *HEX_ptr |= lookuptable[One] << 8; // One's Display
    *HEX_ptr |= lookuptable[Ten] << 16; // Ten's Display
    *HEX_ptr |= lookuptable[Hundred] << 24; // Hundred's Display

    if(One == -5 && Ten == 0 && Hundred == 0) // When to turn on - on display
    {
      *(HEX1_ptr) = 0x40;
      One = 5;
    }
    if(One == 0 && Ten == 0 && Hundred == 0) // When to turn off - on display
    {
      *(HEX1_ptr) = 0x00;
    }
    if(One == 10) // Reset ones
    {
      Ten++;
      One = 0;
    }
    if(Ten == 10) // Reset tens
    {
      Hundred++;
      Ten = 0;
    }
    if(One == -5) // Switch from positive to negative
    {
      Ten--;
      One = 5;
    }
    if(Ten == -1)
    {
      Hundred--;
      Ten = 9;
    }

    //4 pushbuttons 1) increase T; 2) decrease T; 3) switch between C and F; 4) on/off button

    if(*(KEY_ptr) & 0x2) // Temp up
    {
      if(CorF == 0)
        TempSet += 5;
      else
        TempSet += 10;
      if(*(HEX1_ptr) == 0x00 && CorF == 0) // C
        One += 5;
      else if(*(HEX1_ptr) == 0x00 && CorF == 1) // F 
        Ten++;
      else if(*(HEX1_ptr) == 0x40 && CorF == 0) // Accounts for negative value, C
        One -= 5;
      else if(*(HEX1_ptr) == 0x40 && CorF == 1) // Accounts for negative value, F
        Ten--;

      if(CorF == 0 && TempSet == 85) // If 85°C it goes back down to 80°C
      {
        TempSet -= 5;
        One -= 5;
      }
      if(CorF == 1 && TempSet == 190) // If 190°F it goes back down to 180°F
      {
        TempSet -= 10;
        Ten--;
      }
      while(*(KEY_ptr) & 0x2){}
    }
    
    if(*(KEY_ptr) & 0x1) // Temp down
    {
      if(CorF == 0)
        TempSet -= 5;
      else
        TempSet -= 10;
      if(*(HEX1_ptr) == 0x00 && CorF == 0) // C
        One -= 5;
      else if(*(HEX1_ptr) == 0x00 && CorF == 1) // F 
        Ten--;
      else if(*(HEX1_ptr) == 0x40 && CorF == 0) // Accounts for negative value, C
        One += 5;
      else if(*(HEX1_ptr) == 0x40 && CorF == 1) // Accounts for negative value, F
        Ten++;

      if(CorF == 0 && TempSet == -15) // If -15°C it goes back up to -10°C
      {
        TempSet += 5;
        Hundred = 0;
        Ten = 1;
        One = 0;
      }
      if(CorF == 1 && TempSet == 0) // If 0°F it goes back up to 10°F
      {
        TempSet += 10;
        Hundred = 0;
        Ten = 1;
        One = 0;
      }
      
      while(*(KEY_ptr) & 0x1){}
    }

    if(*(KEY_ptr) & 0x4) //Switch between Celsius and Fahrenheit
    {
      if(CorF == 1) // Switches to Celsius
      {
        Current = 0x39;
        CorF = 0;
        TempSet = 25;
        Hundred = 0; // Set default temperature in Celsius
        Ten = 2;
        One = 5; 
        *(HEX1_ptr) = 0x00;
      }
      else // Switches to Fahrenheit
      {
        Current = 0x71;
        CorF = 1;
        TempSet = 80;
        Hundred = 0; // Set default temperature in Fahrenheit
        Ten = 8; 
        One = 0; 
        *(HEX1_ptr) = 0x00;
      }
      while(*(KEY_ptr) & 0x4){}
    }
  }
}
