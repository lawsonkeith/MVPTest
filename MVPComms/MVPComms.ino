/*
  MVP Test program.

  Communicate to MVP @ 38K4 using soft serial
  Arduino monitor is @ 19K2 using Arduino UART
  
  Address must be 1, 
  Arduino  10  Tx
  Arduino  11  Rx
  Arduino  12  WE
  
  
 */
#include <SoftwareSerial.h>

SoftwareSerial SoftSerial(10, 11); // RX, TX
byte Tx[27] = {  1,4,0,0x15,0,  0x64,0,0x64,0,0x64  ,0,0,0,0,0x32,  0x32,0x32,0x32,0x32,0x04,  0x84,0xea,0x50,0,0x0f,  0xd5,0x60}; //see pp20

struct TMVPComms {
  byte   MoistPwr;
  byte SensorSupply[6];
  int SensorVal[10];
  
  int PCBVolts;
  int PCBTemp;
  
  int Solenoid[15];
 
} MVPComms;

int TXMODE = 12; // controls MAX 485
int ADDRESS = 1;

// Table of CRC values for high-order byte

static const unsigned char CRC_HiTable[] =
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
static const unsigned char CRC_LoTable[] =
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



void setup()
{
 
  Serial.begin(19200);
  pinMode(TXMODE, OUTPUT);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println("Init...");

  // set the data rate for the SoftwareSerial port
  SoftSerial.begin(19200);
}

void loop() // run over and over
{
  static int ch;
  static int demand;
  int result;
   
  memset(&MVPComms,0,sizeof(MVPComms));
  for(ch=0;ch<15;ch++)
  {
    for(demand=-250;demand<250;demand+=499)
    {
      MVPComms.Solenoid[ch]=demand;
      MVPComms.Solenoid[ ring(ch-1,15) ]=demand;
      MVPComms.Solenoid[ ring(ch-2,15) ]=demand;
      MVPComms.Solenoid[ ring(ch-3,15) ]=0;
      result = UpdateMVP(&MVPComms);
    }
    //MVPComms.Solenoid[ch]=0;
    Serial.println();
    Serial.print(ch);
  
  }
  delay(100);
}


// Update the MVP, send commands an process the reply.
//
int UpdateMVP(struct TMVPComms *MVPComms)
{
  int result,i=0;
  static byte TxBuffer[100];
  static byte RxBuffer[100];
  
 
  //  =========================================================================
  // === 1st processor TX                                                    ===
  //  =========================================================================
  memset(TxBuffer,0,sizeof(TxBuffer));
  //SoftSerial.setTimeout(50);
  
  digitalWrite(TXMODE, HIGH);
  delay(5);
  
  TxBuffer[i++] = ADDRESS;
  TxBuffer[i++] = 4;
  TxBuffer[i++] = 0;
  TxBuffer[i++] = 0x15;
  
  TxBuffer[i++] = abs(MVPComms->Solenoid[0] ); //1,2
  TxBuffer[i++] = abs(MVPComms->Solenoid[1] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[2] ); //5,6
  TxBuffer[i++] = abs(MVPComms->Solenoid[3] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[4] ); //9,10
  TxBuffer[i++] = abs(MVPComms->Solenoid[5] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[6] ); //13.14
  TxBuffer[i++] = abs(MVPComms->Solenoid[7] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[8] ); //17,18
  TxBuffer[i++] = abs(MVPComms->Solenoid[9] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[10] ); //21,22
  TxBuffer[i++] = abs(MVPComms->Solenoid[11] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[12] ); //25,26
  TxBuffer[i++] = abs(MVPComms->Solenoid[13] );
  TxBuffer[i++] = abs(MVPComms->Solenoid[14] ); //29,30

  // sol 9-16
  TxBuffer[i] = 0;
  if(MVPComms->Solenoid[4] > 0) TxBuffer[i] += 1;
  if(MVPComms->Solenoid[4] < 0) TxBuffer[i] += 2;
  if(MVPComms->Solenoid[5] > 0) TxBuffer[i] += 4;
  if(MVPComms->Solenoid[5] < 0) TxBuffer[i] += 8;
  if(MVPComms->Solenoid[6] > 0) TxBuffer[i] += 16;
  if(MVPComms->Solenoid[6] < 0) TxBuffer[i] += 32;
  if(MVPComms->Solenoid[7] > 0) TxBuffer[i] += 64;
  if(MVPComms->Solenoid[7] < 0) TxBuffer[i] += 128;
  i++;
  
   // sol 1-8
  TxBuffer[i] = 0;
  if(MVPComms->Solenoid[0] > 0) TxBuffer[i] += 1;
  if(MVPComms->Solenoid[0] < 0) TxBuffer[i] += 2;
  if(MVPComms->Solenoid[1] > 0) TxBuffer[i] += 4;
  if(MVPComms->Solenoid[1] < 0) TxBuffer[i] += 8;
  if(MVPComms->Solenoid[2] > 0) TxBuffer[i] += 16;
  if(MVPComms->Solenoid[2] < 0) TxBuffer[i] += 32;
  if(MVPComms->Solenoid[3] > 0) TxBuffer[i] += 64;
  if(MVPComms->Solenoid[3] < 0) TxBuffer[i] += 128;
  i++;
  
  // 25-30 & PSU 1-4
  TxBuffer[i] = 0;
  if(MVPComms->Solenoid[12] > 0) TxBuffer[i] += 1;
  if(MVPComms->Solenoid[12] < 0) TxBuffer[i] += 2;
  if(MVPComms->Solenoid[13] > 0) TxBuffer[i] += 4;
  if(MVPComms->Solenoid[13] < 0) TxBuffer[i] += 8;
  if(MVPComms->Solenoid[14] > 0) TxBuffer[i] += 16;
  if(MVPComms->Solenoid[14] < 0) TxBuffer[i] += 32;
  if(MVPComms->SensorSupply[0] > 0) TxBuffer[i] += 64; // PSU 1 & 2
  if(MVPComms->SensorSupply[1] < 0) TxBuffer[i] += 128; // PSU 3 & 4
  i++;
  
  // 17-24
  TxBuffer[i] = 0;
  if(MVPComms->Solenoid[8] > 0) TxBuffer[i] += 1;
  if(MVPComms->Solenoid[8] < 0) TxBuffer[i] += 2;
  if(MVPComms->Solenoid[9] > 0) TxBuffer[i] += 4;
  if(MVPComms->Solenoid[9] < 0) TxBuffer[i] += 8;
  if(MVPComms->Solenoid[10] > 0) TxBuffer[i] += 16;
  if(MVPComms->Solenoid[10] < 0) TxBuffer[i] += 32;
  if(MVPComms->Solenoid[11] > 0) TxBuffer[i] += 64; 
  if(MVPComms->Solenoid[11] < 0) TxBuffer[i] += 128; 
  i++;
  
   TxBuffer[i++] = 0;
   
  // PSU 5-10
  TxBuffer[i] = 0;
  if(MVPComms->SensorSupply[2] > 0) TxBuffer[i] += 1;
  if(MVPComms->SensorSupply[3] < 0) TxBuffer[i] += 2;
  if(MVPComms->SensorSupply[4] > 0) TxBuffer[i] += 4;
  if(MVPComms->MoistPwr) TxBuffer[i] += 8;  // Moist PSU
  i++;
  
  // CRC 
  CalcCRC(TxBuffer,i /*tgt*/);
  i+=2;
  
  // Serial stuff
  // DEBUG
  /*Serial.println("Dump...");
  for(i=0;i<27;i++)
  {
    Serial.print(TxBuffer[i],HEX);
    Serial.print(",");
  }*/
  Serial.println("!");
  SoftSerial.write(TxBuffer, i); //waits till txd
  digitalWrite(TXMODE, LOW);
  
  //  =========================================================================
  // === 1st processor RX                                                    ===
  //  =========================================================================
  //Serial.println("");Serial.print("A");
  memset(RxBuffer,0,sizeof(RxBuffer));
  SoftSerial.readBytes(RxBuffer, 23);
   
  result = CheckCRC(RxBuffer,21); 
     
  i=0;

  if(result)
  { 
    if(RxBuffer[i++] != ADDRESS + 16)
      return -1;
      
    if(RxBuffer[i++] != 4)
      return -2;
   
    i+=2;
    if(RxBuffer[i++] != 0x10)
      return -3;

    MVPComms->SensorVal[0] = RxBuffer[7] + (RxBuffer[6] << 8);
    MVPComms->SensorVal[1] = RxBuffer[9] + (RxBuffer[8] << 8);
    MVPComms->SensorVal[2] = RxBuffer[11] + (RxBuffer[10] << 8);
    MVPComms->SensorVal[3] = RxBuffer[13] + (RxBuffer[12] << 8);
    MVPComms->SensorVal[4] = RxBuffer[15] + (RxBuffer[14] << 8);
    MVPComms->SensorVal[5] = RxBuffer[17] + (RxBuffer[16] << 8);
    MVPComms->SensorVal[6] = RxBuffer[19] + (RxBuffer[18] << 8);
    MVPComms->SensorVal[7] = RxBuffer[21] + (RxBuffer[20] << 8);
  }
  else
    return -4;  
    
    
  //  =========================================================================
  // === 2nd processor RX                                                    ===
  //  =========================================================================
  memset(RxBuffer,0,sizeof(RxBuffer));
  //Serial.println("B");
  SoftSerial.readBytes(RxBuffer, 19);
   
  result = CheckCRC(RxBuffer,17); 
   
  i=0;
   
  if(result)
  {
    if(RxBuffer[i++] != (ADDRESS + 32))
      return -5;
      
    if(RxBuffer[i++] != 4)
      return -6;
      
    i+=2;
    if(RxBuffer[i++] != 0x0c)
      return -7;
      
    MVPComms->SensorVal[8] = RxBuffer[7] + (RxBuffer[6] << 8);
    MVPComms->SensorVal[9] = RxBuffer[9] + (RxBuffer[8] << 8);
    MVPComms->SensorVal[10] = RxBuffer[11] + (RxBuffer[10] << 8);
    MVPComms->SensorVal[11] = RxBuffer[13] + (RxBuffer[12] << 8);
    MVPComms->PCBTemp = RxBuffer[15] + (RxBuffer[14] << 8);
    MVPComms->PCBVolts = RxBuffer[17] + (RxBuffer[16] << 8);   
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
//
int ring(int i,int len)
{
  if(i>len)
    return i- len;
    
  if(i<0)
    return i+len;
}

