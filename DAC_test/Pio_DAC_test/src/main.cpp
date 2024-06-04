#include <Arduino.h>
#include <SimpleCLI.h>

void(* resetFunc) (void) = 0;

SimpleCLI cli;

  

#define RdWr 39
#define strobePin 41

#define ChCnt 8

char buffer[50];
uint16_t LastVout[8];                                         //last set output for channel 
uint8_t LastCh;                                               //last channel set
uint16_t SquareVoutPos;                                       //used in command ss and sa
uint16_t SquareVoutNeg;
boolean DoSquareSync=false;
boolean DoSquareAlt=false;
boolean DoSquareChan=false;
boolean DoDataBusNoise=false;
boolean SquareTgl;

uint16_t VoltToUSB(float Vout) {
  if (Vout>9.9997) Vout=9.9997;
  return((uint16_t)( ((Vout + 10) / 20) * 65536 ));
}

void SetVout(uint8_t Ch, boolean Show=0) {
  char fbuf[10];
  PORTA = Ch+48;                                                 //Base (A7-A3) set to b.0011.0dddsa 
  PORTC = highByte(LastVout[Ch]);
  PORTL = lowByte(LastVout[Ch]);
  //digitalWrite(RdWr, LOW);                 //>>debug
  digitalWrite(strobePin, HIGH);
  digitalWrite(strobePin, LOW);
  //digitalWrite(RdWr, HIGH);                //>>Debugv 8
  if (Show) {
    double Vout = ((double)LastVout[Ch] * 20/65536)-10;
    dtostrf(Vout,6,4,fbuf);
    sprintf(buffer," Set Ch%u to %sV (bin. 0x%x)", Ch+1, fbuf, LastVout[Ch]); 
    Serial.println(buffer);
  }  
}

void CB_FwRst(cmd * c) {
  Command cmd(c);                                             // Create wrapper object  
  Serial.print("Soft Resetting now.."); 
  delay(100);
  resetFunc();
}

void CB_SetAllChan(cmd *c) {                                  //sa <Vout>
  Command cmd(c);
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum>1) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);
  String sVout= arg.getValue();
  float Vout = sVout.toFloat();
  for (uint8_t i=0; i<ChCnt; i++) {
    LastVout[i]= VoltToUSB(Vout);
    SetVout(i,true); 
  }
}

void CB_SquareChan(cmd *c) {                                  //Q <channel> <vout>
  Command cmd(c);
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum<2) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);
  String sCh= arg.getValue();
  uint8_t ch= sCh.toInt();
  LastCh = ch-1; 
  arg = cmd.getArg(1);
  String sVout= arg.getValue();
  float Vout = sVout.toFloat();
  if (Vout>=0) {
    SquareVoutPos = VoltToUSB(Vout);   
    Vout *= -1;
    SquareVoutNeg = VoltToUSB(Vout);   
  } else {
    SquareVoutNeg = VoltToUSB(Vout);   
    Vout *= -1;
    SquareVoutPos = VoltToUSB(Vout);   
  } 
  DoSquareChan=true;
  sprintf(buffer," Single Channel SquareWaveSync: 0x%x <-> 0x%x)", SquareVoutNeg, SquareVoutPos ); 
  Serial.println(buffer);
}

void CB_SquareSync(cmd* c) {                                  //Qs <Vout>
  Command cmd(c);
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum>1) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);
  String sVout= arg.getValue();
  float Vout = sVout.toFloat();
  if (Vout>=0) {
    SquareVoutPos = VoltToUSB(Vout);   
    Vout *= -1;
    SquareVoutNeg = VoltToUSB(Vout);   
  } else {
    SquareVoutNeg = VoltToUSB(Vout);   
    Vout *= -1;
    SquareVoutPos = VoltToUSB(Vout);   
  }
  DoSquareSync=true;
  sprintf(buffer," All channel SquareWaveSync: 0x%x <-> 0x%x)", SquareVoutNeg, SquareVoutPos ); 
  Serial.println(buffer);
}


void CB_SquareAlt(cmd* c) {                                   //qa <Vout>
  Command cmd(c);
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum>1) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);
  String sVout= arg.getValue();
  float Vout = sVout.toFloat();
  if (Vout>=0) {
    SquareVoutPos = VoltToUSB(Vout);   
    Vout *= -1;
    SquareVoutNeg = VoltToUSB(Vout);   
  } else {
    SquareVoutNeg = VoltToUSB(Vout);   
    Vout *= -1;
    SquareVoutPos = VoltToUSB(Vout);   
  }
  DoSquareAlt=true;
  sprintf(buffer," All channel SquareWaveAlt: 0x%x <-> 0x%x)", SquareVoutNeg, SquareVoutPos ); 
  Serial.println(buffer);

}

void CB_NoiseDataBus(cmd* c) {                                //nd <option>
  Command cmd(c);
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum>1) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);

  String sOption= arg.getValue();
  uint8_t Option = sOption.toInt();

  switch(Option) {
    case 0:
      PORTC = 0;  PORTL = 0;
      sprintf(buffer," Databus set to 0x0000"); 
      return;
    case 1:
      PORTC = 0xff; PORTL = 0xff;
      sprintf(buffer," Databus set to 0xFFFF"); 
      return;
    case 3:
      SquareVoutPos=0xFFFF;
      SquareVoutNeg=0x0;
      break;
    default:
      SquareVoutPos=0xAAAA;
      SquareVoutNeg=0x5555;
      break;
  }   
  DoDataBusNoise=true;
  sprintf(buffer," Noise on the databus: 0x%x <-> 0x%x)", SquareVoutNeg, SquareVoutPos ); 
  Serial.println(buffer);
}


void CB_ChanVout(cmd* c) {                                    // v <ch> <float value>: Set Vout for ch 
  Command cmd(c);                                             // Create wrapper object
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum<2) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);
  String sCh= arg.getValue();
  uint8_t ch= sCh.toInt();
  arg = cmd.getArg(1);
  String sVout= arg.getValue();
  float Vout = sVout.toFloat();
  if (Vout>9.9997) Vout=9.9997;
  uint16_t BinVal = VoltToUSB(Vout);   
  LastCh = ch-1;                                              //For user channel numbers ar 1...4 !!
  LastVout[LastCh]=BinVal; 
  SetVout(LastCh, true);
}

void CB_ChanBinVout(cmd* c) {                                 // b <ch> <Hex Value>: Set Vout for ch 
  char *s;
  long BinVal;
  char buf[4];
  Command cmd(c);                                             // Create wrapper object
  int argNum = cmd.countArgs();                               // Get number of arguments
  if (argNum<2) {Serial.println("Parm error!"); return;}
  Argument arg = cmd.getArg(0);
  String sCh= arg.getValue();
  uint8_t ch= sCh.toInt();
  arg = cmd.getArg(1);
  String sVout= arg.getValue();
  sVout.toCharArray(buf,5);                                   //WHY 5??? 4 will read 0xFFF 
  BinVal = strtol(buf,&s,0x10); 
  LastCh = ch-1;                                              //For user channel numbers ar 1...4 !!
  LastVout[LastCh]=(uint16_t)BinVal; 
  SetVout(LastCh, true);
}

// Callback in case of an error
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}


Command ChanVout;                             //v <ch> <Value>  (decimal)
Command ChanBinVout;                          //b <ch> <hexValue.
Command FwReset;                              //rst
Command SquareChan;                           //qc <ch> <Vout>  
Command SquareSync;                           //qs <Vout>
Command SquareAlt;                            //qa <Vout>
Command SetAllChan;                           //sa <vout>
Command NoiseDataBus;                         //nd <noise choice>

void setup() {
    DDRA = 0Xff;
    DDRC = 0Xff;
    DDRL = 0Xff;
    PORTA = 0; // set PORTA (digital 7~0) to outputs           //Card Address A7-A0
    PORTC = 0; // set PORTC (digital 7~0) to outputs           //Dac DataBus D15-D8
    PORTL = 0; // set PORTL (digital 7~0) to outputs           //Dac DataBus D7-D0
    pinMode(strobePin, OUTPUT); digitalWrite(strobePin, LOW);
    pinMode(RdWr, OUTPUT); digitalWrite(RdWr, HIGH);
    LastCh=0;
    LastVout[0]=0x8000;LastVout[1]=0x8000;LastVout[2]=0x8000;LastVout[3]=0x8000;
    LastVout[4]=0x8000;LastVout[5]=0x8000;LastVout[6]=0x8000;LastVout[7]=0x8000;

    Serial.begin(9600);                                               // Enable Serial connection at given baud rate
    Serial.setTimeout(10000);
    
    ChanVout = cli.addBoundlessCommand("v", CB_ChanVout);
    SetAllChan = cli.addBoundlessCommand("sa", CB_SetAllChan);
    ChanBinVout = cli.addBoundlessCommand("b", CB_ChanBinVout);
    FwReset = cli.addBoundlessCommand("rst", CB_FwRst);
    SquareChan= cli.addBoundlessCommand("qc", CB_SquareChan);
    SquareSync = cli.addBoundlessCommand("qs", CB_SquareSync);
    SquareAlt = cli.addBoundlessCommand("qa", CB_SquareAlt);
    NoiseDataBus = cli.addBoundlessCommand("nb", CB_NoiseDataBus);

    cli.setOnError(errorCallback);                                    // Set error Callback
    Serial.print("\n** Dac test firmware **\nCommand: ");                                 // Start the loop
    pinMode(strobePin, OUTPUT); 
}

String input;
boolean DoSetVoutUD=false;

void loop() {
  // Check if user typed something into the serial monitor
  if (Serial.available()) { 
    DoSquareSync=false;                                   //any input stops all the Squarewaving
    DoSquareAlt=false;
    DoSquareChan = false;
    DoDataBusNoise = false;
    int ib = Serial.read();
    if (ib =='\n') {
      cli.parse(input);                                   // Parse the input
      input="";
      Serial.print("Command: ");
    } else if (ib=='u') {                                 // increase the voltage value of last outputted channel by 1 lsb
      if (LastVout[LastCh]<0xFFFF) {                      // boundary increment
        LastVout[LastCh]+=1;   
        DoSetVoutUD=true;
      }  
    } else if (ib=='d') {                                 // decrease the voltage value of last outputted channel by 1 lsb                              
      if (LastVout[LastCh]>0) {                           // boundary decrement  
        LastVout[LastCh]-=1;        
        DoSetVoutUD=true;
      }  
    } else {
      input += char(ib);
    }  

    if (DoSetVoutUD) {
      Serial.println();
      SetVout(LastCh,true); 
      DoSetVoutUD=false;
      Serial.print("\nCommand: ");
    }  
  } // close if (serial.available....



  if (DoSquareChan) {
    SquareTgl=!SquareTgl;
    if (SquareTgl) LastVout[LastCh]=SquareVoutPos; else LastVout[LastCh]=SquareVoutNeg;
    SetVout(LastCh,false); 
  } else if (DoSquareSync) {
    SquareTgl=!SquareTgl;
    for (uint8_t i=0; i<ChCnt; i++) {
      if (SquareTgl) LastVout[i]=SquareVoutPos; else LastVout[i]=SquareVoutNeg;
      SetVout(i,false); 
      //delay(100); Serial.print(".");
    }
  } else if (DoSquareAlt) {
    SquareTgl=!SquareTgl;
    for (uint8_t i=0; i<ChCnt; i++) {
      if (SquareTgl) { 
        if (i==0 || i==2 || i==4 || i==6) LastVout[i]=SquareVoutPos; 
        if (i==1 || i==3 || i==5 || i==7) LastVout[i]=SquareVoutNeg; 
      } else {
        if (i==0 || i==2 || i==4 || i==6) LastVout[i]=SquareVoutNeg; 
        if (i==1 || i==3 || i==5 || i==7) LastVout[i]=SquareVoutPos;  
      } 
      SetVout(i,false); 
    }
  } else if (DoDataBusNoise) {
    SquareTgl=!SquareTgl;
    if (SquareTgl) { 
      PORTC = highByte(SquareVoutPos); 
      PORTL = lowByte(SquareVoutPos);
    } else {
      PORTC = highByte(SquareVoutNeg); 
      PORTL = lowByte(SquareVoutNeg);
    }  
  }

}