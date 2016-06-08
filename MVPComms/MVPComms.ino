/* 
  Module: MVP Test program.

  Author: K Lawson
  Target: Arduino Nano ATmega328
  
  Description:
  Communicate to MVP @ 19K2 using soft serial
  Arduino monitor is @ 19K2 using Arduino UART
  
  * A button allows for fast / slow test - hold in for slower test / press for fast test
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

// define test constants
  // Current FB - HI RES
  const int SOL_HI = 2120; // mA
  //2042
  const int SOL_LO = 1930; // mA
  int SOL_OFF_LOAD = 210;  //nominally it's 180mA just to power the board
  int SOL_DRIVE = 230; // 0-255 demand
  
  // Sensors
  const int mA_HI = 446; // 426 raw val
  const int mA_LO = 406;
  const int V_HI = 988; // 968
  const int V_LO = 948;
  const int SEN_OFF = 10;
  const int MOIST_MIN = 672; //692
  const int MOIST_MAX = 712;
  const int TEMP_MIN = 498; //518
  const int TEMP_MAX = 538;
  const int PCBV_MIN = 514; //534
  const int PCBV_MAX = 554; 
  const int PCBT_MIN = 567; //587
  const int PCBT_MAX = 627; 
  
  
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

// results defns 
#define IS_MA 16
#define IS_V  17

// A structure to hold test results
// true = pass, false = fail
//
struct TMVPResults {
  bool   AddressFound;    // did we find the board?
  bool   IsNewBoard;      // old / new MFVP PCB
  bool   TestComplete;    // finished?
  byte   TestNum;         // # tests done so far
  byte   SensorDrive[10]; // includes config info mA/V
  bool   SensorNull[10];
  bool   Moist;           // moist ip
  bool   Temp;            // pt100 ip
  bool   PCBVolts;        // PBC onboards...
  bool   PCBTemp;

  int   MoistVal;           // moist ip
  int   TempVal;            // pt100 ip
  int   PCBVoltsVal;        // PBC onboards...
  int   PCBTempVal;
  
  bool   RunningTest;
  bool   PropDriveA[15];   // prop results
  bool   PropDriveB[15];
  bool   PropNull[15];
  int    PropDriveValA[15];
  int    PropDriveValB[15]; 
  int    PropNullVal[15]; 
  
  int    SensorVal[10];
  int    SensorNullVal[10];
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

  // set the data rate for the SoftwareSerial port (MFVP)
  SoftSerial.begin(19200);
  SoftSerial.setTimeout(100); //make addr scan faster
 
}


// Main application loop
//
void loop() // run over and over
{
  static int Current,aveCurrent;
  static byte state,result=BUSY;
  byte mode,sampletime; 
  static unsigned long Update = 0;
  
  
  // @@@@@ Clock routine at 20Hz 1 / (7ms + 38ms) @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  // all functs must return immediately!
  sampletime=100; //x .1ms conversion time - 7ms
  while(sampletime){
    Current = (analogRead(CURRENT_IN) -512) * 36; //mA
    aveCurrent = GetAveCurrent(1000,Current); // # samples
    sampletime--;
  };
   
  // @@@@ SM FOR AUTO TEST @@@@@@@@  
  switch(state){
    // get user input
    case 0: mode = GetInput(); // can hang for 2s initially
            state = mode; 
            if(mode > 0) {
              ResetResults();  
              Update=millis()+5; // kick terminal     
            }        
            ResetMVPOut(&MVPComms); //zero all outputs
            break;
            
    // execute test fast
    case FAST: 
            result = RunTest(FAST,aveCurrent); // note this will take 38ms to execute due to bus timings and the fact it's a blocking transaction
            if(result != BUSY) {
               state = 0;
            }         
            break;
            
    //execute test slow
    case SLOW:
           result = RunTest(SLOW,aveCurrent);
            if(result != BUSY) {
               state = 0;
            }
            break;
  }
  // @@@@ SM FOR AUTO TEST @@@@@@@@
  
  // Periodic terminal update.
  // 
  if(millis() > Update) {
    if(MVPResults.TestComplete == true){
      Update = millis() + 15000; 
    }
    else{
      Update = millis() + 5000; //done
    }
    UpdateTerminal(); // 5s
  }
  UpdateLED(result);

}// END loop



// set results to void state
//
void ResetResults()
{
  byte i;

  MVPResults.RunningTest = false;
  MVPResults.AddressFound = false; 
  MVPResults.TestNum = 0;
  MVPResults.TestComplete = false; 
  
  MVPResults.MoistVal = 0;           // moist ip
  MVPResults.TempVal = 0;            // pt100 ip
  MVPResults.PCBVoltsVal = 0;        // PBC onboards...
  MVPResults.PCBTempVal = 0;
  
  for(i=0;i<10;i++) {
    MVPResults.SensorDrive[i] = 0;  
    MVPResults.SensorNull[i] = false;
    MVPResults.SensorVal[i] = 0;
    MVPResults.SensorNullVal[i] = 0;
  }
  
  MVPResults.IsNewBoard = true; //default
  MVPResults.Moist = false;       
  MVPResults.Temp = false;        
  MVPResults.PCBVolts = false;   
  MVPResults.PCBTemp = false;   
  MVPResults.Fail = false;
  
  for(i=0;i<=14;i++) {
    MVPResults.PropDriveA[i] = false;
    MVPResults.PropDriveB[i] = false;
    MVPResults.PropNull[i] = false;
    MVPResults.PropNullVal[i] = 0;
    MVPResults.PropDriveValA[i] = false;
    MVPResults.PropDriveValB[i] = false;
  }

  MVPResults.CommsOk=0;
  MVPResults.CommsError= 0;
  ADDRESS = 0;
} 


// Write terminal from a buffer
//
void UpdateTerminal(void)
{
  byte tests,i;
  bool wasfault=false;
  
  // clear screen
  Serial.write(27);       // ESC command
  Serial.print("[2J");    // clear screen command
  Serial.write(27);
  Serial.print("[H");     // cursor to home command

  Serial.println(F("=================== SMD MFVP Board tester (Hi/Lo Res) V1.0 ===================="));
  Serial.println("");
  tests = MVPResults.TestNum;
  if(MVPResults.RunningTest == false) {
    Serial.println(F("Waiting for user input..."));
  }
  else if (tests == 0) {
    Serial.println(F("Scanning possible MVP Addresses..."));
  }else
  {
    // #1
    if(MVPResults.AddressFound){
      Serial.print(F("TEST1>>> Found board address at address #"));
      Serial.print(ADDRESS);
      Serial.print(" / ");
      Serial.print(ADDRESS,BIN);
      if(MVPResults.IsNewBoard){
        Serial.println(" - high(er) res board.");  
      }else{
        Serial.println(" - low res board.");   
      }
    }else{
      Serial.println(F("TEST1>>> Can't communicate to board on any address"));
    }
    if(MVPResults.AddressFound == false) return;

    // #2
    Serial.print(F("TEST2>>> Drive prop A channels, check current is between "));
    Serial.print(SOL_HI);
    Serial.print("mA and ");
    Serial.print(SOL_LO);
    Serial.println("mA");
 
    Serial.print("  ");
    for(i=0;i<=14;i++){
      if(i==8){
        Serial.println();
        Serial.print("  ");
      }
      Serial.print(MVPResults.PropDriveValA[i]);
      if(MVPResults.PropDriveA[i])
        Serial.print(" ok, ");
      else
        Serial.print(" fail, ");
    }
    if((--tests) == 0) return;
 
    // #3  
    Serial.println();
    Serial.print(F("TEST3>>> Drive prop B channels, check current is between "));
    Serial.print(SOL_HI);
    Serial.print("mA and ");
    Serial.print(SOL_LO);
    Serial.println("mA");
    
    Serial.print("  ");
    for(i=0;i<=14;i++){
      if(i==8){
        Serial.println();
        Serial.print("  ");
      }
      Serial.print(MVPResults.PropDriveValB[i]);
      if(MVPResults.PropDriveB[i])
        Serial.print(" ok, ");
      else
        Serial.print(" fail, ");
    }
    if((--tests) == 0) return;
    
    // #4
    Serial.println();
    Serial.print(F("TEST4>>> Check proportionals channels turn off, board consumes <"));
    Serial.print(SOL_OFF_LOAD);
    Serial.println("mA");

    Serial.print("  ");
    for(i=0;i<=14;i++){
      if(i==8){
        Serial.println();
        Serial.print("  ");
      }
      Serial.print(MVPResults.PropNullVal[i]);
      if(MVPResults.PropNull[i]){
        Serial.print(" ok, ");
      } else {
        Serial.print(" fail, ");
      }
    }
    //Serial.println();
    if((--tests) == 0) return;

    // #5
    Serial.println();
    Serial.println(F("TEST5>>> Test sensor power supplies & inputs, detect config"));
    Serial.print("  ");
    for(i=0;i<=9;i++){
      if(i==5){
        Serial.println();
        Serial.print("  ");
      }
      Serial.print(MVPResults.SensorVal[i]);
      if(MVPResults.SensorDrive[i]==IS_MA) {
        Serial.print(" mA ok, ");
      }
      else if(MVPResults.SensorDrive[i]==IS_V) {
        Serial.print(" V ok, ");
      } else {
        Serial.print(" fail, ");  
      }
    }

    if((--tests) == 0) return;
    for(i=0;i<=9;i+=2){
      if((MVPResults.SensorDrive[i] == 0) && (MVPResults.SensorDrive[i+1] == 0)) {
        Serial.println();
        Serial.print("  Tip - this looks like a power supply fault");
        break;
      }
    } 
    for(i=0;i<=9;i+=2){
      if((MVPResults.SensorDrive[i] == 0 && MVPResults.SensorDrive[i+1] == 1) && (MVPResults.SensorDrive[i] == 1 && MVPResults.SensorDrive[i+1] == 0)) {
        Serial.println("");
        Serial.print("  Tip - this looks like a sensor fault");
        break;
      }
    } 
    Serial.println("");
    
    // #6
    Serial.println(F("TEST6>>> Check sensor values are zero when power supplies are turned off. "));
    Serial.print("  ");
    for(i=0;i<=9;i++){
      Serial.print(MVPResults.SensorNullVal[i]);
      
      if(MVPResults.SensorNull[i])
        Serial.print(" ok,");
      else
        Serial.print(" fail,");
    }
    if((--tests) == 0) return;
    if(((MVPResults.SensorNull[0] == 0) && (MVPResults.SensorNull[1] == 0)) || 
       ((MVPResults.SensorNull[2] == 0) && (MVPResults.SensorNull[3] == 0)) ||
       ((MVPResults.SensorNull[4] == 0) && (MVPResults.SensorNull[5] == 0)) ||
       ((MVPResults.SensorNull[6] == 0) && (MVPResults.SensorNull[7] == 0)) ||
       ((MVPResults.SensorNull[8] == 0) && (MVPResults.SensorNull[9] == 0)) )  
    {
      Serial.println();
      Serial.print("  Tip - this looks like a power supply fault");
    }
    
    if((MVPResults.SensorNull[0] != MVPResults.SensorNull[1]) || 
       (MVPResults.SensorNull[2] != MVPResults.SensorNull[3]) ||
       (MVPResults.SensorNull[4] != MVPResults.SensorNull[5]) ||
       (MVPResults.SensorNull[6] != MVPResults.SensorNull[7]) ||
       (MVPResults.SensorNull[8] != MVPResults.SensorNull[9]))  
    {
      Serial.println();
      Serial.print("  Tip - this looks like a power supply fault");
    }
  
    for(i=0;i<=9;i+=2){
      if((MVPResults.SensorDrive[i] == 0 && MVPResults.SensorDrive[i+1] == 1) && (MVPResults.SensorDrive[i] == 1 && MVPResults.SensorDrive[i+1] == 0)) {
        Serial.println();
        Serial.print("  Tip - this looks like a sensor fault");
        break;
      }
    }   

    // #7
    Serial.println("");
    Serial.print(F("TEST7>>> Check moisture turns on and off and sensor works: "));
    Serial.print(  MVPResults.MoistVal );           // moist ip
      if(MVPResults.Moist) {
        Serial.print(" ok ");
      } else {
        Serial.print(" fail ");
      }
    Serial.println("");
    if((--tests) == 0) return; 
  
    // #8
    Serial.print(F("TEST8>>> Check PT100 temperature input works: "));
    Serial.print(  MVPResults.TempVal );            // pt100 ip
      if(MVPResults.Temp)
        Serial.print(" ok ");
      else
        Serial.print(" fail ");
    Serial.println("");
    if((--tests) == 0) return;

    // #9
    Serial.print(F("TEST9>>> Check onboard PCBVolts sensor works: "));
    Serial.print(  MVPResults.PCBVoltsVal );        // PBC onboards...
      if(MVPResults.PCBVolts)
        Serial.print(" ok ");
      else
        Serial.print(" fail ");  
    if((--tests) == 0) return;
    
    // #10
    Serial.println();
    Serial.print(F("TEST10>>> Check onboard PCBTemp sensor works: "));
    Serial.print(  MVPResults.PCBTempVal );
      if(MVPResults.PCBTemp)
        Serial.print(" ok ");
      else
        Serial.print(" fail ");  
    if((--tests) == 0) return;

    if(MVPResults.TestComplete){
      Serial.println();
      Serial.println();
      Serial.print("DONE>>> ");
      Serial.print("Comms error rate was: ");
      Serial.print(MVPResults.CommsOk);
      Serial.print(" ok ");
      Serial.print(MVPResults.CommsError);
      Serial.print(" errors ");
  
      if(MVPResults.Fail == true){
        Serial.print(F("- Test has failed"));
      }
      else {
        Serial.print(F("- Test has passed"));
      }
    }
  }
}


// run the test on the MFVP
// ! return immediately with BUSY /  DONE, the latter 2 stop this funct being called and conclude testing
// In here we do a blocking comms poll, then check some test to see if it went ok
// all test results get put in a structure.  When we reply DONE, we immediately restart so it's up to the calling 
// funct to stop calling us.
// 
byte RunTest(byte Speed,int Current)
{
  // main state machine variable
  static byte Attempt=0, state = 0,channel=0; //state machine vars
  static char retval;      // return val 
  int aveCurrent,aveSensor;
  byte result;
  static int SAMPLES,statecnt=0;

  // clear incomming messgae area 
  result = UpdateMVP(&MVPComms);

  ResetMVPOut(&MVPComms); //zero all outputs

  // ok got comms now.
  switch(state)
  {
    case 0: // init 
            MVPResults.RunningTest=true;
            retval = BUSY;
            statecnt=0;
            channel=0;
            state++;
            GetAveCurrent(0,0);
            ADDRESS=1;
            Attempt=0;
            if(Speed == FAST){
              SAMPLES = 50;
            }
            else{  
              SAMPLES = 150;
            } 
            break;
     
     case 1: // board addr scan
            statecnt++;
            aveCurrent = Current; // # now averaged elsewhere
            // @ check...
            if(statecnt==2){ //3 goes, it's slow due to timeout
              statecnt=0;
              ADDRESS++;
              result=0;
              Serial.print(ADDRESS-1);
              Serial.print(",");
            }
            // -->> bad
            if(ADDRESS==17){
              Attempt++;
              ADDRESS = 0;
            }
              
            if((ADDRESS==17) && (Attempt == 5)){
                MVPResults.AddressFound = false; 
                MVPResults.TestNum++;
                state = 0;
                retval = DONE;
                MVPResults.Fail = true;
                Serial.println("FAIL!!!");
            }else if (result==1) {
              MVPResults.AddressFound = true; 
              MVPResults.TestNum++;
              state++;
              channel = 0;
              statecnt = 0;
              GetAveCurrent(0,0);
              // new board / old board detection?
              if(aveCurrent < 220) {// new board 170-190
                SOL_OFF_LOAD = 210; // for no load test criterea
                SOL_DRIVE = 230;
                MVPResults.IsNewBoard = true;    
              } else { // range is 231-271mA for old board
                SOL_OFF_LOAD = 290;
                SOL_DRIVE = 94;
                MVPResults.IsNewBoard = false;  
              }      
            }
            break;
            
    case 2: // scan all proportional outputs (positive) ...
            // 
            // @ check...
            aveCurrent = Current; // # now averaged elsewhere
            MVPComms.Solenoid[channel] = SOL_DRIVE;
            
            statecnt++;
            if(statecnt==SAMPLES){
              statecnt=0;
              GetAveCurrent(0,0); // reset
              
              // -->> test results
              if((aveCurrent < SOL_HI) && (aveCurrent > SOL_LO)){
                // ok, current is good
                MVPResults.PropDriveA[channel] = true;                          
              } else {
                // fail
                MVPResults.PropDriveA[channel] = false;
                MVPResults.Fail = true;
              }
              MVPResults.PropDriveValA[channel] = aveCurrent;
              channel++;
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
    
    case 3: // scan all proportional outputs (negative) ...
            // 
            // @ check...
            aveCurrent = Current; // # now averaged elsewhere
            MVPComms.Solenoid[channel] = -1 * SOL_DRIVE;
            
            statecnt++;
            if(statecnt==SAMPLES){
              statecnt=0;
              GetAveCurrent(0,0); // reset
              
              // -->> test results
              if((aveCurrent < SOL_HI) && (aveCurrent > SOL_LO)){
                // ok, current is good
                MVPResults.PropDriveB[channel] = true;                          
              } else {
                // fail
                MVPResults.PropDriveB[channel] = false;
                MVPResults.Fail = true;
              }
              MVPResults.PropDriveValB[channel] = aveCurrent;
              channel++;
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
            
    case 4: // scan all proportional outputs (null) ...
            // 
            // @ check...
            aveCurrent = GetAveCurrent(SAMPLES,Current); // # samples
            
            statecnt++;            
            if(statecnt==20){
              statecnt=0;          
              GetAveCurrent(0,0); // reset
              
              // -->> test results
              if(aveCurrent < SOL_OFF_LOAD){
                // ok, current is good
                MVPResults.PropNull[channel] = true;
              } else {
                // fail
                MVPResults.PropNull[channel] = false;
                MVPResults.Fail = true;
              }
              MVPResults.PropNullVal[channel] = aveCurrent;
              channel++;
            }
            
            
            // -->> done
            if(channel==17) {
              MVPResults.TestNum++;
              state++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0);
            }
            break;
                      
    case 5: // scan all sensor inputs for (positive)...
            // 
            // @ check...
            statecnt++;
            aveSensor = GetAveSensor(SAMPLES,channel); // # samples
            MVPComms.SensorSupply[channel/2]=1; //turn on PSU 1 per 2 sensors...

            if(statecnt==SAMPLES){
              statecnt=0;           
              GetAveSensor(0,0); // reset
                         
              // -->> test results
              if( (aveSensor < mA_HI) && (aveSensor > mA_LO)) {
                 MVPResults.SensorDrive[channel] = IS_MA;
              }
              else if ((aveSensor < V_HI) && (aveSensor > V_LO)){
                // ok, current is good
                  MVPResults.SensorDrive[channel] = IS_V;
              }else {
                // fail
                MVPResults.SensorDrive[channel] = 0;
                MVPResults.Fail = true;
              }
              MVPResults.SensorVal[channel] = aveSensor;
              channel++;
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
            
    case 6: // scan all sensor inputs (null) values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,channel); // # sample
            statecnt++;
            
            if(statecnt==20){
              statecnt=0;
              
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
              MVPResults.SensorNullVal[channel] = aveSensor;
              channel++;
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

    case 7: // scan moisture values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,10); // # sample
            MVPComms.MoistPwr = 1;
            statecnt++;
               
            if(statecnt==SAMPLES){
              statecnt=0;           
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
              MVPResults.MoistVal = aveSensor;           // moist ip
              channel++;
            }
            
            // -->> done
            if(channel==1) {
              state++;
              MVPResults.TestNum++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
            }
            break;

    case 8: // scan temp values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,11); // # sample
            statecnt++;
             
            if(statecnt==SAMPLES){
              statecnt=0;
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
              MVPResults.TempVal = aveSensor;            // pt100 ip
              channel++;
            }
            
            // -->> done
            if(channel==1) {
              state++;
              MVPResults.TestNum++;
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
            }
            break;

    case 9: // scan PCBVolts values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,12); // # sample
            statecnt++;
            
            if(statecnt==SAMPLES){
              statecnt=0;
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
              MVPResults.PCBVoltsVal = aveSensor;        // PBC onboards...
              channel++;
            }
            
            // -->> done
            if(channel==1) {
              state++;
              MVPResults.TestNum++;         
              channel = 0;
              statecnt = 0;
              GetAveSensor(0,0); // reset
              MVPResults.TestComplete = true;
            }
            break;
            
    case 10: // scan PCB Temp values...
            // 
            // @ check...
            aveSensor = GetAveSensor(SAMPLES,13); // # sample
            statecnt++;
        
            if(statecnt==SAMPLES){
              statecnt=0;
              GetAveSensor(0,0); // reset
              // -->> test results
              if((aveSensor > PCBT_MIN) && (aveSensor < PCBT_MAX)) {
                // ok, 
                MVPResults.PCBTemp = true;
              } else {
                // fail
                MVPResults.PCBTemp = false;
                MVPResults.Fail = true;
              }
              MVPResults.PCBTempVal = aveSensor;
              channel++;
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
  else if (state > 1)
    MVPResults.CommsError++;

  return retval;
}


// calc average current, also has a reset function
//  samples - 0 to reset else smoothing period
//  Current - a float to smooth
//
int GetAveCurrent(byte samples,int Current)
{
  static float ave;
   
  if(samples > 0) {
    samples *= .3; // exponent compensation
    ave = (float)(samples-1) / (float)samples * ave  + (float)Current/(float)samples ;
  }
  else
    ave = Current; //reset
    
  return (int)ave; //mA
}

// calc average sensor value, also has a reset function
//  samples - 0 to reset else it's the smoothing period.
//  channel - to smooth 0-9 (ana) 10 (moist) 11 (temp)
//
int GetAveSensor(byte samples,byte channel)
{
  static float ave[14];
  byte i;
  
  if((channel < 0) || (channel > 13)){
    Serial.println("GetAveSensor: Range ERROR");
    return -1;
  }

  samples *= .3; // exponent compensation
     
  if(channel == 10) {
    ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.MoistVal/(float)samples ;
  }else if(channel == 11) {
    ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.TempVal/(float)samples ;
  }
  else if(channel == 12) {
    ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.PCBTemp/(float)samples ;
  }
  else if(channel == 13) {
    ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.PCBVolts/(float)samples ;
  }
  else if(samples > 0) {
     ave[channel] = (float)(samples-1) / (float)samples * ave[channel]  + (float)MVPComms.SensorVal[channel]/(float)samples ;
  }
  else{
    for(i=0;i<=11;i++)
      ave[i] = 0; //reset
  }
  
  return (int)ave[channel];
}


// Status LED, it's bicolour and has 3 states 
//
void UpdateLED(byte Result)
{ 
  if(MVPResults.Fail){
    digitalWrite(LEDAR,HIGH); // red
    digitalWrite(LEDAG,LOW);
  }else if ((MVPResults.Fail == false) && (MVPResults.TestComplete == true)){
    digitalWrite(LEDAR,LOW); // gn
    digitalWrite(LEDAG,HIGH);
  } else {
    digitalWrite(LEDAR,LOW); // clear
    digitalWrite(LEDAG,LOW);
  }
}

// We want to get user input here quick or long test, hold in btn for long test
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
    
    if(x>15) {
      Serial.println("Running normal test");
      delay(500);
      return SLOW;  //long
    } else {
      Serial.println("Running short test");
      delay(500);
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
  
  for(i=0;i<=14;i++)
    MVPComms->Solenoid[i] = 0;
    
  for(i=0;i<=4;i++)
    MVPComms->SensorSupply[i] = 0;
    
  MVPComms->MoistPwr = 0;
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
  if(MVPComms->SensorSupply[1] > 0) Buffer[i] += 128; // PSU 3 & 4
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
  if(MVPComms->SensorSupply[3] > 0) Buffer[i] += 2;
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
//Serial.println("!");
  SoftSerial.write(Buffer, i); //waits till txd
  digitalWrite(TXMODE, LOW);
  
  //  =========================================================================
  // === 1st processor RX                                                    ===
  //  =========================================================================
  //Serial.println("");Serial.print("A");
  memset(Buffer,0,sizeof(Buffer));
  SoftSerial.readBytes((char *)Buffer, 23);
   
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
  SoftSerial.readBytes((char *)Buffer, 19);
   
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
      //Serial.println("CRC fail" );
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


