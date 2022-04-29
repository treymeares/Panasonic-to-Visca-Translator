/*
 * 
 * Sony VISCA to Panasonic AW Serial converter
 * for trey@lodestarlabs.co
 */

#include <SoftwareSerial.h>

// Define debug
//#define debug

// Define our tx/rx pins

#define InTx 8   // tx pin IN
#define InRx 9   // rx pin IN

#define OutTx 10 // tx pin OUT
#define OutRx 11 // rx pin OUT

SoftwareSerial InSerial(InRx,InTx);
SoftwareSerial OutSerial(OutRx,OutTx);

// define some variables for VISCA
byte InByte;
byte InCommandArray [16];
byte ArrayPos=0;
bool CommandFlag=false;
byte DstDeviceAddress=0;

byte SelfAddress=3;

// variables for Panasonic
byte OutCommandArray [16];
byte OutByte;
byte OutAnswerArray [64];
byte OutAnswerByte;


void ProcessArray();
void PSPower(bool InState);
void PSPanSpeedControl(byte InSpeed);
void PSTiltSpeedControl(byte InSpeed);
void PSZoomSpeedControl(byte InSpeed);


void setup() {


  Serial.begin(9600);
  // init software serials
  OutSerial.begin(9600);
  OutSerial.listen();
  // init terminal serial
  PSPower(true);
  
  InSerial.begin(9600);     // set IN speed
  InSerial.listen();
  // init terminal serial
  Serial.print("Started with address - ");
  Serial.println(SelfAddress);
  Serial.end();

}

void loop() {

  // Check if we have something in the IN port
  if(InSerial.available()){
      InByte = InSerial.read();

      #ifdef debug
        Serial.print("0x");
        Serial.println(InByte,HEX);
      #endif
      
      if(!CommandFlag){
                
        // check for command header
        if(InByte>0x80 && InByte<0x89){
          CommandFlag=true;
        }
        InCommandArray[ArrayPos]=InByte;
        ArrayPos++;
        
      }else{
        // store byte in to array
        InCommandArray[ArrayPos]=InByte;
        ArrayPos++;
        // check for command end 0xff
        if(InByte==0xff){
          ProcessArray();
          ArrayPos=0;
          CommandFlag=false;  
          for(int i=0;i<16;i++){
            InCommandArray[i]=0;
          }
        }
     }
  }  
}

// ATEM functions

void ProcessBroadcast(){
  
    // Set address command
    if(InCommandArray[1]==0x30){
    #ifdef debug
    Serial.print("Address set to ");
    Serial.println(SelfAddress);
    #endif
      InCommandArray[0]=0x88;
      InCommandArray[1]=0x30;
      InCommandArray[2]=SelfAddress++;
      InCommandArray[3]=0xFF;

      delay(200);
      
      byte b=InSerial.write(InCommandArray,4);
      
    #ifdef debug
      Serial.print("Bytes out - ");
      Serial.println(b);
    #endif
    }
}

void SendAck(){
  
      InCommandArray[0]=(SelfAddress+8<<4);
      InCommandArray[1]=0x41;
      InCommandArray[2]=0xFF;

      byte ackl=InSerial.write(InCommandArray,3);
      #ifdef debug
      Serial.print("Ack - ");
      Serial.println(ackl);
      #endif
      
}

void SendNAck(){
  
      InCommandArray[0]=(SelfAddress+8<<4);
      InCommandArray[1]=0x60;
      InCommandArray[2]=0x02;
      InCommandArray[3]=0xFF;

      byte ackl=InSerial.write(InCommandArray,4);
      #ifdef debug
      Serial.print("Ack - ");
      Serial.println(ackl);
      #endif
      
}

void SendComplete(){
      
      InCommandArray[0]=(SelfAddress+8<<4);
      InCommandArray[1]=0x51;
      InCommandArray[2]=0xFF;

      byte scl=InSerial.write(InCommandArray,3);
      #ifdef debug
      Serial.print("Complete - ");
      Serial.println(scl);
      #endif
}


void ProcessControlCommand(){
  
  #ifdef debug
    Serial.println("Control command");
  #endif

    // check categories, we need 0x04 (camera control for zoom & focus) & 0x06 (pan-tilt control)
    // we can add more categories later

    // 0x04 
    if(InCommandArray[2]==0x04){
  #ifdef debug
    Serial.println("Category 0x04");
  #endif

      // CAM_Power 
      if(InCommandArray[3]==0x00){
        switch(InCommandArray[4]){
        
          case 0x02:
          // Camera power ON
  #ifdef debug
    Serial.println("Camera power ON");
  #endif
            SendAck();
            PSPower(true);
            SendComplete();
          break;
          case 0x03:
          // Camera power OFF
  #ifdef debug
    Serial.println("Camera power OFF");
  #endif
            SendAck();
            PSPower(false);
            SendComplete();
          
          break;        
        
        }  
      }

      // CAM_Zoom
      if(InCommandArray[3]==0x07){
        
        // test for stop && standard tele/wide
        switch(InCommandArray[4]){
          case 0x00:
          // Stop
  #ifdef debug
    Serial.println("Zoom - STOP");
  #endif
            SendAck();
            PSZoomSpeedControl(50);
            SendComplete();
          break;
          case 0x02:
          // Tele(Standard)
   #ifdef debug
    Serial.println("Zoom Tele(standard)");
  #endif
            SendAck();
            SendComplete();
         break;
          case 0x03:
          // Wide (Standard)
  #ifdef debug
    Serial.println("Zoom Wide(standard)");
  #endif
            SendAck();
            SendComplete();
          break;            
        }

        // test for variable tele/wide
        byte TempByte=(InCommandArray[4]>>4);
    
        switch(TempByte){
          case 0x02:
          // tele(variable)
  #ifdef debug
    Serial.println("Zoom Tele(variable)");
  #endif
            SendAck();
            {
                byte ZoomSpeed=50+(InCommandArray[4]&0x0F)*7;
                Serial.print("Zoom tele - ");
                Serial.println(ZoomSpeed);
                PSZoomSpeedControl(ZoomSpeed);
            }
            SendComplete();
          
          break;
          case 0x03:
          // wide (variable)
  #ifdef debug
    Serial.println("Zoom Wide(variable)");
  #endif
            SendAck();
            {
                byte ZoomSpeed=50-(InCommandArray[4]&0x0F)*7;
                Serial.print("Zoom widie - ");
                Serial.println(ZoomSpeed);
                PSZoomSpeedControl(ZoomSpeed);
            }
            SendComplete();        
          break;
        }                 
      }

      // CAM_Zoom DIRECT
      if(InCommandArray[3]==0x47){
        
  #ifdef debug
    Serial.println("CAM_Zoom Direct");
  #endif
          SendAck();
          SendComplete();
        
      }
      
    }
    // 0x06
    if(InCommandArray[2]==0x06){

  #ifdef debug
    Serial.println("Category 0x06");
  #endif
  
    // Pan_tiltDrive movements
      if(InCommandArray[3]==0x01){
        
        // extract command
        unsigned int Command=InCommandArray[6]<<8;
        Command=Command+InCommandArray[7];

        switch(Command){
          case 0x0301:
          // Up
 #ifdef debug
    Serial.println("Pan/tilt - UP");
  #endif
            SendAck();
            {
              byte TiltSpeed=50-InCommandArray[5]*2;                        
                Serial.print("UP - ");
                Serial.println(TiltSpeed);
              PSTiltSpeedControl(TiltSpeed);
            }
            SendComplete();
         break;
         case 0x0302:
          // Down
  #ifdef debug
    Serial.println("Pan/tilt - Down");
  #endif
            SendAck();
            {
              byte TiltSpeed=50+InCommandArray[5]*2;
              Serial.print("DOWN - ");
              Serial.println(TiltSpeed);
              PSTiltSpeedControl(TiltSpeed);
            }
            SendComplete();
         break;
         case 0x0103:
          // Left
  #ifdef debug
    Serial.println("Pan/tilt - Left");
  #endif
            SendAck();
            {            
              byte PanSpeed=50-InCommandArray[4]*2;                        
                Serial.print("LEFT - ");
                Serial.println(PanSpeed);
              PSPanSpeedControl(PanSpeed);
            }
            SendComplete();
         break;
         case 0x0203:
          // Right
  #ifdef debug
    Serial.println("Pan/tilt - Right");
  #endif 
            SendAck();
            {
                byte PanSpeed=50+InCommandArray[4]*2;
                Serial.print("RIGHT - ");
                Serial.println(PanSpeed);
                PSPanSpeedControl(PanSpeed);
            }
            SendComplete();
         break;
         case 0x0101:
          // Upleft
  #ifdef debug
    Serial.println("Pan/tilt - Upleft");
  #endif
            SendAck();
            {
              byte TiltSpeed=50-InCommandArray[5]*2;
              Serial.print("UPLEFT T- ");
              Serial.println(TiltSpeed);
              PSTiltSpeedControl(TiltSpeed);
              byte PanSpeed=50-InCommandArray[4]*2;                        
              Serial.print("UPLEFT P- ");
              Serial.println(PanSpeed);
              PSPanSpeedControl(PanSpeed);
            }
            SendComplete();
         break;
         case 0x0201:
          // Upright
  #ifdef debug
    Serial.println("Pan/tilt - Upright");
  #endif
         
            SendAck();
            {
              byte TiltSpeed=50-InCommandArray[5]*2;
              Serial.print("UPRIGHT T- ");
              Serial.println(TiltSpeed);
              PSTiltSpeedControl(TiltSpeed);
              byte PanSpeed=50+InCommandArray[4]*2;  
              Serial.print("UPRIGHT P- ");
              Serial.println(PanSpeed);
              PSPanSpeedControl(PanSpeed);
            }
            SendComplete();
         break;
         case 0x0102:
          // Downleft
  #ifdef debug
    Serial.println("Pan/tilt - Downleft");
  #endif
         
            SendAck();
            {
              byte TiltSpeed=50+InCommandArray[5]*2; 
              Serial.print("DOWNLEFT T- ");
              Serial.println(TiltSpeed);
              PSTiltSpeedControl(TiltSpeed);
              byte PanSpeed=50-InCommandArray[4]*2;
              Serial.print("DOWNLEFT P- ");
              Serial.println(PanSpeed);
                                    
              PSPanSpeedControl(PanSpeed);
            }
            SendComplete();
         break;          
         case 0x0202:
          // Downright
  #ifdef debug
    Serial.println("Pan/tilt - Downright");
  #endif
 
            SendAck();
            {
              byte TiltSpeed=50+InCommandArray[5]*2;                        
              Serial.print("DOWNRIGHT T- ");
              Serial.println(TiltSpeed);
              PSTiltSpeedControl(TiltSpeed);
              byte PanSpeed=50+InCommandArray[4]*2;   
              Serial.print("DOWNRIGHT P- ");
              Serial.println(PanSpeed);
                                 
              PSPanSpeedControl(PanSpeed);
            }
            SendComplete();
         break;
         case 0x0303:
          // Stop
  #ifdef debug
    Serial.println("Pan/tilt - Stop");
  #endif
  
            SendAck();
            PSTiltSpeedControl(50);
            PSPanSpeedControl(50);
            SendComplete();
         break;
        }        
      }

      // Pan_tiltDrive Absolute position
      if(InCommandArray[3]==0x02){
  #ifdef debug
    Serial.println("Pan/tilt - Absolute position");
  #endif
        SendAck();
        SendComplete();
       
      }
      
      // Pan_tiltDrive Relative position
      if(InCommandArray[3]==0x03){
 #ifdef debug
    Serial.println("Pan/tilt - Relative position");
  #endif
        SendAck();
        SendComplete();
        
      }

      // Pan_tiltDrive Home
      if(InCommandArray[3]==0x04){
  #ifdef debug
    Serial.println("Pan/tilt - Home");
  #endif
        SendAck();
        SendComplete();
       
      }

      // Pan_tiltDrive Reset
      if(InCommandArray[3]==0x05){
  #ifdef debug
    Serial.println("Pan/tilt - Reset");
  #endif
        SendAck();
        SendComplete();
       
      }
    }
  
}

void ProcessInfoRequest(){

  #ifdef debug
    Serial.println("ProcessInfoRequest");
  #endif

  SendNAck();

  return;

  // prepare answer
  InCommandArray[0]=(SelfAddress+8<<4);
  InCommandArray[1]=0x50;
    
  if (InCommandArray[2]==0x06 && InCommandArray[3]==0x12){
    // Pan-tiltPosInq  
  #ifdef debug
    Serial.println("Pan-tiltPosInq");
  #endif
//y5 50 0w 0 w 0 w 0w    wwww pan pos
//0z 0z 0z 0z FF        zzzz tilt pos

      // pan pos
      InCommandArray[2]=0x00;
      InCommandArray[3]=0x00;
      InCommandArray[4]=0x00;
      InCommandArray[5]=0x00;

      // tilt pos
      InCommandArray[6]=0x00;
      InCommandArray[7]=0x00;
      InCommandArray[8]=0x00;
      InCommandArray[9]=0x00;

      // terminator
      InCommandArray[10]=0xFF;            
      
      Serial.print("Pan-tiltPosInq reply - ");
      Serial.println(InSerial.write(InCommandArray,11));

      return;
  }

  if (InCommandArray[2]==0x04 && InCommandArray[3]==0x47){
    // CAM_ZoomPosInq
  #ifdef debug
    Serial.println("CAM_ZoomPosInq");
  #endif    

//y0 50 0p 0q 0r 0s FF zoom pos

      InCommandArray[2]=0x02;
      InCommandArray[3]=0x00;
      InCommandArray[4]=0x00;
      InCommandArray[5]=0x00;
    
      // terminator
      InCommandArray[6]=0xFF;

      Serial.print("CAM_ZoomPosInq reply - ");
      Serial.println(InSerial.write(InCommandArray,7));

      return;
  }

  #ifdef debug
    Serial.print("Unknown information request - 0x");
    Serial.print(InCommandArray[2],HEX);
    Serial.print(" 0x");
    Serial.println(InCommandArray[3],HEX);
  #endif
}

void ProcessArray(){
  
  // ok, we have something in the our array that looks like a command

  #ifdef debug
    Serial.println("Incoming command captured");
  #endif

  // process broadcast
  if(InCommandArray[0]==0x88){
  
  #ifdef debug
    Serial.println("Broadcast message received");
  #endif
    ProcessBroadcast();
    return;
  }

  // extract destination device address (in VISCA terms) (first 3 bits)
  DstDeviceAddress=(InCommandArray[0]&0x07);

  #ifdef debug
    Serial.print("Destination address - 0x");
    Serial.println(DstDeviceAddress,HEX);
  #endif


  // check for com mode (0x01 - control command)
  #ifdef debug
    Serial.print("Com mode - 0x");
    Serial.println(InCommandArray[1],HEX);
  #endif

  // Process control command
  if(InCommandArray[1]==0x01 && DstDeviceAddress==SelfAddress){
     ProcessControlCommand();    
     return;
  }

  // Process information request 
  if(InCommandArray[1]==0x09 && DstDeviceAddress==SelfAddress){
    ProcessInfoRequest();  
    return;
  }
    
};




// panasonic functions

bool PSExecuteCommand(){

  OutSerial.listen();

  byte ecm = OutSerial.write(OutCommandArray,OutByte);
  #ifdef debug
  Serial.print("Execute command, bytes - ");
  Serial.println(ecm);
  #endif 

  InSerial.listen();
}


bool PSPowerGetAnswer(){

  OutSerial.listen();
  
  //int t=millis();
  
  bool flag=false;
  bool AnswerFlag=false;
  char answ=0;

  

  while(OutSerial.available()>0){// && ((millis()-t)<2000)){
    answ=OutSerial.read();
    #ifdef debug
      Serial.print(answ);
    #endif
  }

  #ifdef debug
  Serial.println();
  #endif

  InSerial.listen();
}

bool PSGetAnswer(){

  OutSerial.listen();
  
  int t=millis();
  
  bool flag=false;
  bool AnswerFlag=false;
  byte answ=0;

  while(!flag && ((millis()-t)<200)){

    // check for bytes in port
    if(OutSerial.available()>0){
      answ=OutSerial.read();
      // Is answer started?
      if(AnswerFlag){          
          // check for answer end 0x0d
          if(answ==0x0d){
            flag=true;
          }
          OutAnswerArray[OutAnswerByte]=answ;
          OutAnswerByte++;        
      }
    }
  }
  InSerial.listen();
  
  #ifdef debug
  Serial.print("PSGetAnswer: ");
  Serial.print(OutAnswerByte);
  Serial.println(" bytes received");

  if(OutAnswerByte>0){
    for(byte k=0;k++;k<OutAnswerByte){
      Serial.print("0x");
      Serial.print(OutAnswerArray[k],HEX);
      Serial.print(" ");
    }  
    Serial.println();
  }
  #endif
 
}



void PSPower(bool InState){
  
  if(InState){
  #ifdef debug
    Serial.println("PowerOn");
  #endif
    OutCommandArray[0]=0x23;
    OutCommandArray[1]=0x4f;
    OutCommandArray[2]=0x31;
    OutCommandArray[3]=0x0d;
  }else{
  #ifdef debug
    Serial.println("PowerOff");
  #endif
    OutCommandArray[0]=0x23;
    OutCommandArray[1]=0x4f;
    OutCommandArray[2]=0x30;
    OutCommandArray[3]=0x0d;
  }

  OutByte=4;

  PSExecuteCommand();
  delay(3000);
  PSPowerGetAnswer();
    
};

void PSPanSpeedControl(byte InSpeed){
  
  #ifdef debug
    Serial.println("PanSpeed control");
  #endif
  String speed = String(InSpeed,DEC);
  
  OutCommandArray[0]=0x23;//#
  OutCommandArray[1]=0x50;//P
  if(InSpeed<10){
    OutCommandArray[2]='0';//2
    OutCommandArray[3]=speed.charAt(0);//5
  }  else{
     OutCommandArray[2]=speed.charAt(0);//2
     OutCommandArray[3]=speed.charAt(1);//5
  }

  OutCommandArray[4]=0x0d;//cd


  Serial.print(OutCommandArray[0]);
  Serial.print(OutCommandArray[1]);
  Serial.print(OutCommandArray[2]);
  Serial.print(OutCommandArray[3]);
  Serial.println(OutCommandArray[4]);

  OutByte=5;

  PSExecuteCommand();

};

void PSTiltSpeedControl(byte InSpeed){
  
  #ifdef debug
    Serial.println("TiltSpeed control");
  #endif
  String speed = String(InSpeed,DEC);

  OutCommandArray[0]=0x23;//#
  OutCommandArray[1]=0x54;//T
  if(InSpeed<10){
    OutCommandArray[2]='0';//7
    OutCommandArray[3]=speed.charAt(0);//5
  }else{
    OutCommandArray[2]=speed.charAt(0);//7
    OutCommandArray[3]=speed.charAt(1);//5
  }
  OutCommandArray[4]=0x0d;//cd

  Serial.print(OutCommandArray[0]);
  Serial.print(OutCommandArray[1]);
  Serial.print(OutCommandArray[2]);
  Serial.print(OutCommandArray[3]);
  Serial.println(OutCommandArray[4]);

  OutByte=5;

  PSExecuteCommand();


 
};

void PSPanTiltPositionControl(){

   #ifdef debug
    Serial.println("PanTilt neutral");
  #endif
  //strcpy(OutCommandArray,"#U40004000");

  OutCommandArray[0]=0x23;//#
  OutCommandArray[1]=0x55;//U
  OutCommandArray[2]=0x33;//3
  OutCommandArray[3]=0x30;//0
  OutCommandArray[4]=0x30;//0
  OutCommandArray[5]=0x30;//0
  OutCommandArray[6]=0x33;//3
  OutCommandArray[7]=0x30;//0
  OutCommandArray[8]=0x30;//0
  OutCommandArray[9]=0x30;//0
  OutCommandArray[11]=0x0d;//cd

  byte cs=OutCommandArray[0]+OutCommandArray[1]+OutCommandArray[2]+OutCommandArray[3]+OutCommandArray[4]+OutCommandArray[5]+OutCommandArray[6]+OutCommandArray[7]+OutCommandArray[8]+OutCommandArray[9];
  if(cs==0x00){
    cs=0x01; 
  }
  if(cs==0x0d){
    cs=0x0e;  
  }

  OutCommandArray[10]=cs; // cs

  OutByte=12;

  PSExecuteCommand();

  PSGetAnswer();
    
};

void PSZoomSpeedControl(byte InSpeed){
  
  #ifdef debug
    Serial.println("ZoomSpeed control");
  #endif

  String speed = String(InSpeed,DEC);

  OutCommandArray[0]=0x23;//#
  OutCommandArray[1]=0x5A;//Z
  if(InSpeed<10){
    OutCommandArray[2]='0';//7
    OutCommandArray[3]=speed.charAt(0);//5
  }else{
    OutCommandArray[2]=speed.charAt(0);//7
    OutCommandArray[3]=speed.charAt(1);//5
  }
  OutCommandArray[4]=0x0d;//cd

  Serial.print(OutCommandArray[0]);
  Serial.print(OutCommandArray[1]);
  Serial.print(OutCommandArray[2]);
  Serial.print(OutCommandArray[3]);
  Serial.println(OutCommandArray[4]);

  OutByte=5;

  PSExecuteCommand();

//  PSGetAnswer();
  
};
