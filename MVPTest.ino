/*

  MVPTest
  R Mobray 2016
  



  Software serial multple serial test

 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.

 The circuit:
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)

 Note:
 Not all pins on the Mega and Mega 2560 support change interrupts,
 so only the following can be used for RX:
 10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69

 Not all pins on the Leonardo support change interrupts,
 so only the following can be used for RX:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).

 created back in the mists of time
 modified 25 May 2012
 by Tom Igoe
 based on Mikal Hart's example

 This example code is in the public domain.

 */
#include <SoftwareSerial.h>
// Declare structure for dereks boards
/* struct DEREK
   {
     //INPUTS
     int   AI[15];
     int   VoltageFB;
     int   TempFB;
     
     //OUTPUTS
     char  Props[15];
   }Board;
   
   int State = 0;
   
   struct t_errors
   {
     // 0 =ok 1 = errorr
     AI[15];
     AO[15];
     Comms;
     Temp;
     Voltage;
     Moist;
   }Errors;
   
*/

SoftwareSerial mySerial(10, 11); // RX, TX


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
    
   pinMode(13, OUTPUT); // LED on nano
  
  // setup bicol LED (3 OPS) for status
  // setup switch IP
  // setup software serial
  // setup terminal serial
  // setup DO for RS485 bus control
  // setup DO for BUSY LED
}


  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(4800);
  mySerial.println("Hello, world?");
}



void loop() // run over and over
{
  int channel;
  /*
  if (mySerial.available())
    Serial.write(mySerial.read());
  if (Serial.available())
    mySerial.write(Serial.read());
  */
  switch(state)
  {
    case 0:  // waiting user input, not having run a test
            if btn pressed {
              state++;
              ClearErrors(&Errrors);
              ClaearBoard(&Boarsd);
              Busry led on
              Status led off
            }
            break;
            
    case 1:  // check comms
            if(BoardSend(&Board) > 0) {
              state ++;
            } else {
              Errors.comms ++;
              state = -1;
           }
           break;
           
   case 2: // chack analog inps
           for(channel=0;channels<10l;channel++) {
             for 
             delay(100);
              if(BoardSend(&Board) > 0)
                comms++;
                
               if(( Board.AI[channel] < MIN  || Board.AI[channel] > MAX))
                 Errors.AI[channel]++;
           }
           state ++;
           
  case 3: // AO

  case 4: // Voltage

  case 5: // temp

  case 6: // moist  
            Count = 0; 
           
        
              
              
    default: //error no comms, continuously print report
            Count++
            if(Count > 1000)
             {
               Count = 0;
                  if(TerminalPrint(Errros) {
                     status led good
                  }
                  else {
                    status led bad
                  }
             }
             
             if(starrt btn) {
               ClearErrors(&Errrors);
              Busry led on
              Status led off
                state  =1 ; 
             }    
        
                
                  break;
            
    
  }
  

}//END Loop


// functs

//TerminalPrint - funct to print report
//BoardSend - Talks to board once
//CalcCRC
//checkCRC
//ClearErrors(&Errrors); (memset)
//ClaearBoard(&Boarsd); clear AOs


