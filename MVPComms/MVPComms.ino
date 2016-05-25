/*
  Module: MVP Test program.

  Author: K Lawson
  Target: Arduino Nano ATmega328
  
  Description:
  Communicate to MVP @ 19K2 using soft serial
  Arduino monitor is @ 19K2 using Arduino UART
  
  * A button allows for fast / slow test
  * And a LED shows status
  * Comms to the board is over 485 using (soft ser)
  * A terminal displays test status info (default ser)
  
 */
#include <SoftwareSerial.h>


SoftwareSerial SoftSerial(11, 10); // RX, TX
#define SLOW 1
#define FAST 2

#define BUSY 8
#define DONE 9

#define IS_MA 16
#define IS_V  17
  
struct TMVPComms {

  // DO
  byte   SensorSupply[5];  // 1-5
  byte   MoistPwr;         // 6

  // AI
  unsigned int SensorVal[10];       // 1-10
  unsigned int MoistVal;            // 11
  unsigned int TempVal;             // 12
  unsigned int PCBVolts;            // 13
  unsigned int PCBTemp;             // 14
  
  // AO
  int Solenoid[15];        // 1-15 -A +B (30 outputs)
 
} MVPComms;

// A structure to hold test results
struct TMVPResults {
  bool   AddressFound;    // did we find the board?
  bool   TestComplete;    // finished?
  byte   TestNum;         // # tests done so far
  byte   SensorDrive[10]; // includes config info mA/V
  bool   SensorNull[10];
  bool   Moist;           // moist ip
  bool   Temp;            // pt100 ip
  bool   PCBVolts;        // PBC onboards...
  bool   PCBTemp;
  bool   PropDrive[15];   // prop results
  bool   PropNull[15];
  bool   Fail;            // were there any fails?
  
  long   CommsOk;         // stats         
  long   CommsError;
} MVPResults;
                          
//GPIO addresses
int TXMODE = 12; // controls MAX 485

int LEDAG = 5;
int LEDK  = 6;
int LEDAR = 7;

int BUTTON_IN = 8;
int BUTTON_OUT = 9;

int CURRENT_IN = 7;

// address#
int ADDRESS = 0;


// terminal

// Table of CRC values for high-order byte
//progmem breaks it??
static const unsigned char CRC_HiTable[]  =
{
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};
// Table of CRC values for low-order byte
static const unsigned char CRC_LoTable[]  =
{
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};


// setup GPIO
//
void setup()
{
  byte i;
  
  pinMode(TXMODE, OUTPUT);
  
  // LED, set high to make come on
  pinMode(LEDAG, OUTPUT);
  pinMode(LEDAR,OUTPUT);
  pinMode(LEDK,OUTPUT);
  digitalWrite(LEDK,LOW);
  
  for(i=0;i<8;i++) {   
    digitalWrite(LEDAR,LOW);
    digitalWrite(LEDAG,HIGH);
    delay(100);
    digitalWrite(LEDAG,LOW);
    digitalWrite(LEDAR,HIGH);
    delay(100);
  }
 

  // Btn goes low when pressed
  pinMode(BUTTON_OUT,OUTPUT);
  pinMode(BUTTON_IN,INPUT_PULLUP);
  digitalWrite(BUTTON_OUT,LOW);
 
  Serial.begin(19200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println("Init...");

  // set the data rate for the SoftwareSerial port (MFVP)
  SoftSerial.begin(19200);
 
}


// Main application loop
//
void loop() // run over and over
{
  static int Current,aveCurrent;
  static byte state,result=BUSY;
  byte mode; 
  static int count;
  
  // @@@@@ Clock routine at 200Hz @@@@@@@@
  // all functs must return immediately!
  delay(5);
  
  // @@@@ SM FOR AUTO TEST @@@@@@@@  
  switch(state){
    // get user input
    case 0: mode = GetInput(); // can hang for 2s initially
            state = mode; 
            if(mode > 0) {
              ResetResults();       
            }        
            ResetMVPOut(&MVPComms); //zero all outputs
            break;
            
    // execute test fast
    case FAST: 
            result = RunTest(FAST,Current); 
            if(result != BUSY) {
               state == 0;
            }         
            break;
            
    //execute test slow
    case SLOW:
            result = RunTest(SLOW,Current);
            if(result != BUSY) {
               state == 0;
            }
            break;
  }
  // @@@@ SM FOR AUTO TEST @@@@@@@@
  
  // Periodic terminal update.
  // 
  count++;
  if(count==1000) {
    UpdateTerminal(); // 5s
    count = 0;
  }
   UpdateLED(result);

  // Need to average the hell out of this
  //
  Current = (analogRead(CURRENT_IN) -512) * 37;
  //aveCurrent = Current * .01 + aveCurrent * .99;

}// END loop



// set results to void state
//
void ResetResults()
{
  byte i;

  MVPResults.AddressFound = false; 
  MVPResults.TestNum = 0;
  MVPResults.TestComplete = false; 
  
  for(i=0;i<10;i++) {
    MVPResults.SensorDrive[i] = 0;  
    MVPResults.SensorNull[i] = false;
  }
  
  MVPResults.Moist = false;       
  MVPResults.Temp = false;        
  MVPResults.PCBVolts = false;   
  MVPResults.PCBTemp = false;   
  MVPResults.Fail = false;
  
  for(i=0;i<10;i++) {
    MVPResults.PropDrive[i] = false;
    MVPResults.PropNull[i] = false;
  }

  MVPResults.CommsOk=0;
  MVPResults.CommsError= 0;
} 


// Write terminal from a buffer
//
void UpdateTerminal(void)
{
  byte tests,i;
  
  // clear screen
  Serial.write(27);       // ESC command
  Serial.print("[2J");    // clear screen command
  Serial.write(27);
  Serial.print("[H");     // cursor to home command

  Serial.println(F("======== MFVP Board tester (Hi-res only) ========"));
  Serial.println("");

  tests = MVPResults.TestNum;
  if(tests == 0) {
     Serial.println(F("Waiting for user input..."));
  }else
  {
    // #1
    if(MVPResults.AddressFound){
      Serial.print(F("Test 1: Found board address at address "));
      Serial.println(ADDRESS);
    }else{
      Serial.println(F("ERROR: - Can't communicate to board"));
    }
    if((tests--) == 0) return;

    // #2
    Serial.print(F("Test 2: Proportionals on: "));
    for(i=0;i<14;i++){
      if(MVPResults.PropDrive[i])
        Serial.print("ok, ");
      else
        Serial.print(",Fail ");
    }
    if((tests--) == 0) return;
 
    // #3
    Serial.print(F("Test 3: Proportionals off: "));
    for(i=0;i<14;i++){
      //Serial.print(i+1);
      if(MVPResults.PropNull[i])
        Serial.print("ok, ");
      else
        Serial.print("Fail, ");
    }
    if((tests--) == 0) return;

    // #4
    Serial.print(F("Test 4: Sensors on: "));
    for(i=0;i<9;i++){
      if(MVPResults.SensorDrive[i]==IS_MA)
        Serial.print("mA ok, ");
      else if(MVPResults.PropDrive[i]==IS_V)
        Serial.print("V ok, ");
      else
        Serial.print("fail, ");  
    }
    if((tests--) == 0) return;

    // #5
    Serial.print(F("Test 5: Sensors off: "));
    for(i=0;i<9;i++){
      Serial.print(i+1);
      if(MVPResults.SensorNull[i])
        Serial.print(":ok ");
      else
        Serial.print(":Fail ");
    }
    if((tests--) == 0) return;

    // #6
    Serial.print(F("Test 6: Moisture "));
      if(MVPResults.Moist)
        Serial.print(":ok ");
      else
        Serial.print(":Fail ");
    if((tests--) == 0) return; 
  
    // #7
    Serial.print(F("Test 7: Temp "));
      if(MVPResults.Temp)
        Serial.print(":ok ");
      else
        Serial.print(":Fail ");
    if((tests--) == 0) return;

    // #8
    Serial.print(F("Test 8: PCBVolts "));
      if(MVPResults.PCBVolts)
        Serial.print(":ok ");
      else
        Serial.print(":Fail ");  
    if((tests--) == 0) return;
    
     // #9
    Serial.print(F("Test 9: PCBTemp "));
      if(MVPResults.PCBTemp)
        Serial.print(":ok ");
      else
        Serial.print(":Fail ");  
    if((tests--) == 0) return;

    if(MVPResults.TestComplete){
      Serial.println("");
      Serial.print("Comms error rate was: ");
      Serial.print(MVPResults.CommsOk);
      Serial.print(" ok ");
      Serial.print(MVPResults.CommsError);
      Serial.println(" errors ");
      Serial.println("");
   
      if(MVPResults.Fail == true)
        Serial.println(F("Test is complete, Failed"));
      else
        Serial.println(F("Test is complete, Pass"));
    }
  }
}


// run the test on the MFVP
// ! return immediately with BUSY /  DONE, the latter 2 stop this funct being called and conclude testing
// In here we do a blocking comms poll, then check some test to see if it went ok
// we return status and fill a char buffer which gets sent to the serial tty.
// 
byte RunTest(byte Speed,int Current)
{
  // main state machine variable
  static byte state = 0,channel=0; //state machine vars
  static char retval;      // return val 
  int aveCurrent,aveSensor;
  byte result;
  static int SAMPLES,statecnt=0;

  // Current FB
  const int SOL_HI = 1000; // mA
  const int SOL_LO = 900; // mA
  const int SOL_OFF = 20;
  const int SOL_DRIVE = 50; // 0-255 demand
  // Sensors
  const int mA_HI = 500; // 0-1023 raw val
  const int mA_LO = 480;
  const int V_HI = 600;
  const int V_LO = 500;
  const int SEN_OFF = 10;
  const int MOIST_MIN = 100;
  const int MOIST_MAX = 200;
  const int TEMP_MIN = 100;
  const int TEMP_MAX = 200;
  const int PCBV_MIN = 100;
  const int PCBV_MAX = 200; 
  const int PCBT_MIN = 100;
  const int PCBT_MAX = 200;

  // clear incomming messgae area 
  result = UpdateMVP(&MVPComms);
  ResetMVPOut(&MVPComms); //zero all outputs
   
  
  // ok got comms now.
  switch(state)
  {
    case 0: // init 
            retval = BUSY;
            statecnt=0;
            channel=0;
            state++;
            ADDRESS=1;
            if(Speed == SLOW)
              SAMPLES = 100;
            else  
              SAMPLES = 2000;
              
            break;
     
     case 1: // board addr scan
            statecnt++;
            // @ check...
            if(statecnt==SAMPLES){
              statecnt=0;
              ADDRESS++;
            }
            // -->> bad
            if(ADDRESS==17){
                state = 0;
                retval = DONE;
                MVPResults.Fail = true;
            }
            // -->> ok
            if(result==1) {
              MVPResults.AddressFound = true; 
              MVPResults.TestNum++;
              state++;
              channel = 0;
              statecnt = 0;
              GetAveCurrent(0,0);
            }
            break;
            
    case 2: // scan all proportional outputs (positive) ...
            // 
            // @ check...
            aveCurrent = GetAveCurrent(SAMPLES,Current); // # samples
            MVPComms.Solenoid[channel] = SOL_DRIVE;
            
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveCurrent(0,0); // reset
              
              // -->> test results
              if((aveCurrent < SOL_HI) && (aveCurrent > SOL_LO)){
                // ok, current is good
                MVPResults.PropDrive[channel] = true;                          
              } else {
                // fail
                MVPResults.PropDrive[channel] = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==15) {
              MVPResults.TestNum++;
              state++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0);
            }
            break;
          
    case 3: // scan all proportional outputs (null) ...
            // 
            // @ check...
            aveCurrent = GetAveCurrent(SAMPLES,Current); // # samples
            
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveCurrent(0,0); // reset
              
              // -->> test results
              if(aveCurrent < SOL_OFF){
                // ok, current is good
                MVPResults.PropNull[channel] = true;
              } else {
                // fail
                MVPResults.PropNull[channel] = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==15) {
              MVPResults.TestNum++;
              state++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0);
            }
            break;
            
    case 4: // scan all sensor inputs for (positive)...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,channel); // # samples
            MVPComms.SensorSupply[channel/2]=1; //turn on PSU 1 per 2 sensors...
            
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveSensor(0,0); // reset
              
              // -->> test results
              if( (aveSensor < mA_HI) && (aveCurrent > mA_LO)) {
                 MVPResults.SensorDrive[channel] = IS_MA;
              }
              else if ((aveSensor < V_HI) && (aveCurrent > V_LO)){
                // ok, current is good
                MVPResults.SensorDrive[channel] = IS_V;
              } else {
                // fail
                MVPResults.SensorDrive[channel] = 0;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==10) {
              MVPResults.TestNum++;
              state++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
            }
            break;
            
    case 5: // scan all sensor inputs (null) values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,channel); // # sample
            
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveSensor(0,0); // reset
              
              // -->> test results
              if(aveSensor < SEN_OFF) {
                // ok, 
                MVPResults.SensorNull[channel] = true;
              } else {
                // fail
                MVPResults.SensorNull[channel] = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==10) {
              MVPResults.TestNum++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
            }
            break;

    case 6: // scan moisture values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,10); // # sample
            MVPComms.MoistPwr = 1;
               
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveSensor(0,0); // reset
              
              // -->> test results
              if((aveSensor > MOIST_MIN) && (aveSensor < MOIST_MAX)) {
                // ok, 
                MVPResults.Moist = true;
              } else {
                // fail
                MVPResults.Moist = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==1) {
              MVPResults.TestNum++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
            }
            break;

    case 7: // scan temp values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,11); // # sample
               
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveSensor(0,0); // reset
              
              // -->> test results
              if((aveSensor > TEMP_MIN) && (aveSensor < TEMP_MAX)) {
                // ok, 
                MVPResults.Temp = true;
              } else {
                // fail
                MVPResults.Temp = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==1) {
              MVPResults.TestNum++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
            }
            break;

    case 8: // scan PCBVolts values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,12); // # sample
               
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveSensor(0,0); // reset
              
              // -->> test results
              if((aveSensor > PCBV_MIN) && (aveSensor < PCBV_MAX)) {
                // ok, 
                MVPResults.PCBVolts = true;
              } else {
                // fail
                MVPResults.PCBVolts = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==1) {
              MVPResults.TestNum++;         
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
              MVPResults.TestComplete = true;
            }
            break;
            
    case 9: // scan PCB Temp values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,13); // # sample
               
            if(statecnt==SAMPLES){
              statecnt=0;
              channel++;
              GetAveSensor(0,0); // reset
              
              // -->> test results
              if((aveSensor > PCBT_MIN) && (aveSensor < PCBT_MAX)) {
                // ok, 
                MVPResults.PCBVolts = true;
              } else {
                // fail
                MVPResults.PCBVolts = false;
                MVPResults.Fail = true;
              }
            }
            
            // -->> done
            if(channel==1) {
              MVPResults.TestNum++;
              state=0;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
              retval = DONE;
              MVPResults.TestComplete = true;
            }
            break;

  }

  // Comms check, error rate should be low
  if(result==1)
    MVPResults.CommsOk++;
  else
    MVPResults.CommsError++;

  if(MVPResults.CommsError > SAMPLES){
    state = 0;
    retval = DONE;
    MVPResults.Fail = true;
  }
  
  return retval;
}


// calc average current, also has a reset function
//
int GetAveCurrent(byte samples,int Current)
{
  static float ave;
   
  if(samples > 0) {
    samples *= .3; // exponent compensation
    ave = (float)(samples-1) / (float)samples * ave  + (float)Current/(float)samples ;
  }
  else
    ave = 0; //reset
}

// calc average sensor value, also has a reset function
//
int GetAveSensor(byte samples,byte channel)
{
  static float ave[11];
  byte i;
  
  if((channel < 0) || (channel > 10))
    return -1;

  samples *= .3; // exponent compensation
     
  if(channel == 10) {
    ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.MoistVal/(float)samples ;
  }
  else if(samples > 0) {
     ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.SensorVal[channel]/(float)samples ;
  }
  else{
    for(i=0;i<10;i++)
      ave[i] = 0; //reset
  }
}


// Status LED, it's bicolour and has 3 states 
//
void UpdateLED(byte Result)
{
  if(Result == DONE) {
    if(MVPResults.Fail){
      digitalWrite(LEDAR,HIGH); // red
      digitalWrite(LEDAG,LOW);
    }else{
      digitalWrite(LEDAR,LOW); // gn
      digitalWrite(LEDAG,HIGH);
    }
  } else {
    digitalWrite(LEDAR,LOW); // clear
    digitalWrite(LEDAG,LOW);
  }
}

// We want to get user input here quick or long test
//
byte GetInput(void)
{
  byte x = 0;
  
  if(digitalRead(BUTTON_IN) == LOW) {
    for(x=0;x<20;x++) {
      if(digitalRead(BUTTON_IN))
        break;
      
      delay(80);
    }
    
    if(x==20) {
      return SLOW;  //long
    } else {
      return FAST;  //short
    } 
  }
  
  return 0;
}


// Turn off all mvp outputs
//
void ResetMVPOut(struct TMVPComms *MVPComms)
{
  byte i;
  
  for(i=0;i<14;i++)
    MVPComms->Solenoid[i] = 0;
    
  for(i=0;i<4;i++)
    MVPComms->SensorSupply[i] = 0;
}



// Update the MVP, send commands an process the reply.
//
int UpdateMVP(struct TMVPComms *MVPComms)
{
  int result,i=0;
  byte Buffer[100];

 
  //  =========================================================================
  // === 1st processor TX                                                    ===
  //  =========================================================================
  memset(Buffer,0,sizeof(Buffer));
  //SoftSerial.setTimeout(50);
  
  digitalWrite(TXMODE, HIGH);
  delay(5);
  
  Buffer[i++] = ADDRESS;
  Buffer[i++] = 4;
  Buffer[i++] = 0;
  Buffer[i++] = 0x15;
  
  Buffer[i++] = abs(MVPComms->Solenoid[0] ); //1,2
  Buffer[i++] = abs(MVPComms->Solenoid[1] );
  Buffer[i++] = abs(MVPComms->Solenoid[2] ); //5,6
  Buffer[i++] = abs(MVPComms->Solenoid[3] );
  Buffer[i++] = abs(MVPComms->Solenoid[4] ); //9,10
  Buffer[i++] = abs(MVPComms->Solenoid[5] );
  Buffer[i++] = abs(MVPComms->Solenoid[6] ); //13.14
  Buffer[i++] = abs(MVPComms->Solenoid[7] );
  Buffer[i++] = abs(MVPComms->Solenoid[8] ); //17,18
  Buffer[i++] = abs(MVPComms->Solenoid[9] );
  Buffer[i++] = abs(MVPComms->Solenoid[10] ); //21,22
  Buffer[i++] = abs(MVPComms->Solenoid[11] );
  Buffer[i++] = abs(MVPComms->Solenoid[12] ); //25,26
  Buffer[i++] = abs(MVPComms->Solenoid[13] );
  Buffer[i++] = abs(MVPComms->Solenoid[14] ); //29,30
   // sol 9-16
  Buffer[i] = 0;
  if(MVPComms->Solenoid[4] > 0) Buffer[i] += 1;
  if(MVPComms->Solenoid[4] < 0) Buffer[i] += 2;
  if(MVPComms->Solenoid[5] > 0) Buffer[i] += 4;
  if(MVPComms->Solenoid[5] < 0) Buffer[i] += 8;
  if(MVPComms->Solenoid[6] > 0) Buffer[i] += 16;
  if(MVPComms->Solenoid[6] < 0) Buffer[i] += 32;
  if(MVPComms->Solenoid[7] > 0) Buffer[i] += 64;
  if(MVPComms->Solenoid[7] < 0) Buffer[i] += 128;
  i++;
  
   // sol 1-8
  Buffer[i] = 0;
  if(MVPComms->Solenoid[0] > 0) Buffer[i] += 1;
  if(MVPComms->Solenoid[0] < 0) Buffer[i] += 2;
  if(MVPComms->Solenoid[1] > 0) Buffer[i] += 4;
  if(MVPComms->Solenoid[1] < 0) Buffer[i] += 8;
  if(MVPComms->Solenoid[2] > 0) Buffer[i] += 16;
  if(MVPComms->Solenoid[2] < 0) Buffer[i] += 32;
  if(MVPComms->Solenoid[3] > 0) Buffer[i] += 64;
  if(MVPComms->Solenoid[3] < 0) Buffer[i] += 128;
  i++;
  
  // 25-30 & PSU 1-4
  Buffer[i] = 0;
  if(MVPComms->Solenoid[12] > 0) Buffer[i] += 1;
  if(MVPComms->Solenoid[12] < 0) Buffer[i] += 2;
  if(MVPComms->Solenoid[13] > 0) Buffer[i] += 4;
  if(MVPComms->Solenoid[13] < 0) Buffer[i] += 8;
  if(MVPComms->Solenoid[14] > 0) Buffer[i] += 16;
  if(MVPComms->Solenoid[14] < 0) Buffer[i] += 32;
  if(MVPComms->SensorSupply[0] > 0) Buffer[i] += 64; // PSU 1 & 2
  if(MVPComms->SensorSupply[1] < 0) Buffer[i] += 128; // PSU 3 & 4
  i++;
  
  // 17-24
  Buffer[i] = 0;
  if(MVPComms->Solenoid[8] > 0) Buffer[i] += 1;
  if(MVPComms->Solenoid[8] < 0) Buffer[i] += 2;
  if(MVPComms->Solenoid[9] > 0) Buffer[i] += 4;
  if(MVPComms->Solenoid[9] < 0) Buffer[i] += 8;
  if(MVPComms->Solenoid[10] > 0)Buffer[i] += 16;
  if(MVPComms->Solenoid[10] < 0)Buffer[i] += 32;
  if(MVPComms->Solenoid[11] > 0) Buffer[i] += 64; 
  if(MVPComms->Solenoid[11] < 0) Buffer[i] += 128; 
  i++;  
   Buffer[i++] = 0;
   
  // PSU 5-10
  Buffer[i] = 0;
  if(MVPComms->SensorSupply[2] > 0) Buffer[i] += 1;
  if(MVPComms->SensorSupply[3] < 0) Buffer[i] += 2;
  if(MVPComms->SensorSupply[4] > 0) Buffer[i] += 4;
  if(MVPComms->MoistPwr) Buffer[i] += 8;  // Moist PSU
  i++;
  
  // CRC 
  CalcCRC(Buffer,i /*tgt*/);
  i+=2;
  
  // Serial stuff
  // DEBUG
  /*Serial.println("Dump...");
  for(i=0;i<27;i++)
  {
    Serial.print(Buffer[i],HEX);
    Serial.print(",");
  }*/
  Serial.println("!");
  SoftSerial.write(Buffer, i); //waits till txd
  digitalWrite(TXMODE, LOW);
  
  //  =========================================================================
  // === 1st processor RX                                                    ===
  //  =========================================================================
  //Serial.println("");Serial.print("A");
  memset(Buffer,0,sizeof(Buffer));
  SoftSerial.readBytes(Buffer, 23);
   
  result = CheckCRC(Buffer,21); 
     
  i=0;

  if(result)
  { 
    if(Buffer[i++] != ADDRESS + 16)
      return -1;
      
    if(Buffer[i++] != 4)
      return -2;
   
    i+=2;
    if(Buffer[i++] != 0x10)
      return -3;

    MVPComms->SensorVal[0] = Buffer[6] + (Buffer[5] << 8);
    MVPComms->SensorVal[1] = Buffer[8] + (Buffer[7] << 8);
    MVPComms->SensorVal[2] = Buffer[10] + (Buffer[9] << 8);
    MVPComms->SensorVal[3] = Buffer[12] + (Buffer[11] << 8);
    MVPComms->SensorVal[4] = Buffer[14] + (Buffer[13] << 8);  
    MVPComms->SensorVal[5] = Buffer[16] + (Buffer[15] << 8);
    MVPComms->SensorVal[6] = Buffer[18] + (Buffer[17] << 8);
    MVPComms->SensorVal[7] = Buffer[20] + (Buffer[19] << 8);
  }
  else
    return -4;  
    
    
  //  =========================================================================
  // === 2nd processor RX                                                    ===
  //  =========================================================================
  memset(Buffer,0,sizeof(Buffer));
  //Serial.println("B");
  SoftSerial.readBytes(Buffer, 19);
   
  result = CheckCRC(Buffer,17); 
   
  i=0;
   
  if(result)
  {
    if(Buffer[i++] != (ADDRESS + 32))
      return -5;
      
    if(Buffer[i++] != 4)
      return -6;
      
    i+=2;
    if(Buffer[i++] != 0x0c)
      return -7;
      
    MVPComms->SensorVal[8] = Buffer[6] + (Buffer[5] << 8);
    MVPComms->SensorVal[9] = Buffer[8] + (Buffer[7] << 8);
    MVPComms->MoistVal = Buffer[10] + (Buffer[9] << 8);
    MVPComms->TempVal = Buffer[12] + (Buffer[11] << 8);
    MVPComms->PCBTemp = Buffer[14] + (Buffer[13] << 8);
    MVPComms->PCBVolts = Buffer[16] + (Buffer[15] << 8);   
  }
  else
    return -8;  
    
  return 1;
}


// Just pass me the data area.  I'll put the CRC on the end
// [                 DATA                   ] + [ CRC HI ] [CRC LO ]
//  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
void CalcCRC(byte *Data,byte Len)
{
  unsigned int uIndex;
  byte uchCRCHi = 0xff;
  byte uchCRCLo = 0xff;
  byte i;
  
  i=Len;
  while (i--)
  {
    uIndex = uchCRCHi ^ *Data++;
    uchCRCHi = uchCRCLo ^ CRC_HiTable[uIndex];
    uchCRCLo = CRC_LoTable[uIndex];
  }

  *(Data++) = uchCRCHi;
  *Data = uchCRCLo;
}


// Just pass me the data area.  I'll check the CRC on the end
// [                 DATA                   ] [ CRC HI ] [CRC LO ]
//  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// 1 = Ok
// -1 = Fail
char CheckCRC(byte *Data,byte Len)
{
   byte uchCRCHi =  Data[Len];
   byte uchCRCLo =  Data[Len+1];

   Data[Len] = 0;
   CalcCRC(Data,Len);
  
   if((uchCRCHi != Data[Len]) || (uchCRCLo != Data[Len+1]))
   {
      Serial.println("CRC fail" );
      return -1;
   }
   else
   {
     return 1;
   }
}

// Wraparound functionm for LED scrolling
// Say 6 would go 0-5
int ring(int i,int len)
{
  if(i>=len)
    return i- len;
    
  if(i<0)
    return i+len;
}


