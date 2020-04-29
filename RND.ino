/*
        { }
 */

// I2C Things
#include <Wire.h>
#define DAC 0x60  
byte b1=64;
byte b2=0;
byte b3=0;


// Sequencing things
int serial_mode=2; // 0: text, 1: midi, 2: none
int counter=0;
int step2=0;
int laststep=0;
unsigned long time_now = 0;
int sequence[64]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float interval=7.5/2; // about 125 BPM
int looped=0;
bool looped_trig=false;
int ratcheting_steps=0;
int origin=0;
int scale_mode=1;
bool scale_trig=false;
int Scales_L[8][12]={{1,1,1,1,1,1,1,1,1,1,1,1},{1,0,1,0,1,1,0,1,0,1,0,1},{1,1,0,1,0,1,1,0,1,0,1,0},{1,0,1,1,0,1,0,1,0,1,1,0},{1,0,1,0,1,0,1,1,0,1,0,1},{1,1,0,1,0,1,0,1,1,0,1,0},{1,0,1,0,1,1,0,1,0,1,1,0},{1,0,1,1,0,1,0,1,1,0,1,0}};
float MV = 1/12.0;
int loop_size=64;
int pulse_out=0;
int pitch=0;
int pitch_out=0;
bool testnote=false;
int CV_mode=2;
bool CV_mode_trig=false;
bool scale_disable=false;
int loop_chrono=0;
int CV_chrono=0;
int note_dac=0;
bool Scale_menu=true;
bool Loop_menu=false;
bool FirstLoop = true;
float scale=36;
float n = 10000.0/scale;


// Inputs
float Amount=100.0;
float Range=100.0;
int SendMode;
int lastSendMode=2;
float Attenuator;
int SendParam=38;
float clock_input=0;
bool internal_clock=true;
bool clk_received=false;
float CVinput=0;
float note =0;
float RangeModulated=100.0;
bool last_clock=false;
bool reseted=false;


//MIDI out stuff
int noteON = 144;//144 = 10010000 in binary, note on command
int noteOFF = 128;//128 = 10000000 in binary, note off command
bool note_sent=false;
bool note_stoped=false;


void setup() {     
    Wire.begin();  
    Wire.setClock(400000);         
    pinMode(11, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(4, INPUT);
    pinMode(5, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    scale_leds();
    if (serial_mode==0) Serial.begin(9600);  
    else if (serial_mode==1) {
      Serial.begin(31250);
      ALLnotesOff(); 
      }
    }


void clocking_input() { 

    // Clock input processing
    if (analogRead(A6)>20) { clock_input = true;}
    else clock_input  = false;  
 
    // Clock test
    // if (last_clock!=clock_input) { last_clock =!last_clock;
    // tempo_test(50);
    // }

    // External clocking check     
    if (digitalRead(12)==LOW) internal_clock = false;
    else internal_clock  = true;

    }

void resetCV(){

  // Reset Input processing
    if (digitalRead(4)==HIGH && !reseted) { 
        reseted=!reseted;
        step2=0;
        laststep=70;
        }
    if (digitalRead(4)==LOW && reseted) { 
        reseted=!reseted;
        }
    }

    
void steps() {
        
   // Loop processing   
    if (digitalRead(11)==LOW && !looped_trig) { 
        loop_chrono=1;
        looped_trig=true;
        }     
    if (loop_chrono>0) { loop_chrono+=1;
        if (loop_chrono > 149) LEDS(1,1,1,1,1,1);
        }     
    if (digitalRead(11)==HIGH && looped_trig) {
        looped_trig=false;
        if (serial_mode==0) Serial.println((String)"Pushed");
        if (0<loop_chrono && loop_chrono<140) {
            if (Loop_menu==true) looped += 1;
            Loop_menu=true;
            Scale_menu=false;
            if (looped==4) looped=1;
            if (looped==0) loop_size=64;
            if (looped==1) {loop_size=16; 
                if (FirstLoop) {   
                origin=step2;
                step2=0;
                if (laststep==0) laststep=70;
                FirstLoop=false;}
                }
            if (looped==2) loop_size=32;
            if (looped==3) loop_size=64; 
            }
        else if (loop_chrono>149) {
            //step2=origin; // reset, old behavior
            looped=0;
            FirstLoop=true;
            loop_size=64;
            Loop_menu=true;
            Scale_menu=false;
            }
        loop_chrono=0;
        loop_leds();
        }
  
    }

void inputs() { 
    // Knobs processing
    Amount = clamp(analogRead(A0)/10.23,0,100);
    Range = clamp(analogRead(A1)/10.23,0,100);
    SendParam=clamp(analogRead(A3)/10.23,0,80)+20;
    if ((4*SendParam-69)%16==0) SendParam=SendParam-1;
    Attenuator = clamp(analogRead(A7)/10.23,0,100);
    
    // CV input processing
    CVinput = clamp(analogRead(A2)/10.23,0,100);
    if (CV_mode==1) Range=clamp(Range+CVinput*Attenuator*2/100,0,100);
    else if (CV_mode==2) SendParam = clamp(SendParam+CVinput*Attenuator*2/100,0,100);
    else if (CV_mode==0) Amount= clamp(Amount+CVinput*Attenuator*2/100,0,100);
    
    // Switch processing 
    if (digitalRead(13)==HIGH) SendMode = 2;
    else SendMode  = 1;  


    // CV mode processing
    if (digitalRead(11)==LOW && digitalRead(10)==LOW && !CV_mode_trig) { 
        CV_mode_trig=true;
        looped_trig=true;
        CV_mode +=1;
        CV_mode=CV_mode%3;
        scale_disable=true;
        CV_chrono=1;
        }
    if (digitalRead(11)==HIGH && digitalRead(10)==LOW && CV_mode_trig) {CV_mode_trig=false;
        CV_chrono=0;
        }

    if (CV_chrono>0) { CV_chrono+=1;
        if (CV_chrono > 4) CV_leds();
        }     

 

    // Scale processing
    if (digitalRead(10)==LOW && !scale_trig && !scale_disable) { 
        scale_trig=true;}
    if (digitalRead(10)==HIGH && scale_trig && !scale_disable) {scale_trig=false;
        if (Scale_menu==true) scale_mode += 1;
        scale_mode=scale_mode%8;
        CV_mode_trig=false;
        scale_leds();
        Scale_menu=true;
        Loop_menu=false;
        }
    if (digitalRead(10)==HIGH && scale_disable) {scale_disable=false;
        scale_trig=false;}



    if (serial_mode==0) Serial.println((String)"Amount:"+Amount+" Range:"+Range+" SendParam:"+SendParam+" Attenuator:"+Attenuator+" CVinput:"+CVinput+" clock_input:"+clock_input+" internal_clock:"+internal_clock+" SendMode:"+SendMode+" Reseted:"+reseted);


    }

void outputs() {                    
    if (pulse_out>0) {
        pulse_out+=-1;
        if (!note_sent) { 
            pitch_out=pitch;
            if (serial_mode==1) MIDImessage(noteON, pitch_out+48);
            digitalWrite(9, HIGH);
            note_dac=(int)note;
            b2=note_dac>>4;
            b3=note_dac<<4;
            DACout(b2,b3);
            note_sent=true;
            note_stoped=false;            
            }
        }
    else { 
        if (!note_stoped) {
            if (serial_mode==1) MIDImessage(noteOFF, pitch_out+48);
            digitalWrite(9, LOW);
            note_sent=false;
            note_stoped=true;
            }
        }
    }

void MIDImessage(int command, int MIDInote) {
    if (serial_mode==1) {
        Serial.write(command);//send note on or note off command 
        Serial.write(MIDInote);//send pitch data
        Serial.write(100);//send velocity data   
        }
    }

void ALLnotesOff() {
   if (serial_mode==1) {
            Serial.write(176);
            Serial.write(123);
            Serial.write(0);
            }
    }

void loop() {

    time_now = millis();
    inputs();
    clocking_input();
    resetCV();   


    // Ratcheting or Gate mode switching processing
    if (lastSendMode!=SendMode) {
        ALLnotesOff();
        ratcheting_steps=0;
        pulse_out=0;
        }

    // Counter management
    if (!internal_clock) { 
        if (clock_input && !clk_received) {
            clk_received = true;
            if (step2>=63) step2=0;
            else step2+=1;
            counter=0;
            }
        if (!clock_input) clk_received=false;
        }
    else { 
        counter+=1;
        if (counter==32) {          
            if (step2==63) step2=0;
            else step2+=1;
            counter=0;
            }
        }
    
    steps();   

    // Processing
    if (step2 !=laststep) {
        laststep=step2;
        if (looped==0) {
            if (ratcheting_steps==0) {
                sequence[step2]=0;
                float random1=rand()%100;
                if (random1<=Amount) {
                    float random2=rand()%100+1;
                    sequence[step2]=random2;
                    }
                }
            }
        // Ratcheting steps, sending a pulse, pitch stays the same
        if (ratcheting_steps>0) {
            ratcheting_steps-=1;
            pulse_out=23;
            }
        // Trig that is not a ratcheting step, calculating the note to send
        else if (sequence[((looped==0) ? step2 :  (step2%loop_size + origin-loop_size+1+ 64)%64)]>0) {
            // pitch between 0 and 10000, accounting for 3 octaves
            if (pulse_out==0) pitch = floor(sequence[((looped==0) ? step2 : (step2%loop_size +origin-loop_size +1+ 64)%64)]*Range/n);
            // First note is C2 = 1.250mV?
            if (Scales_L[scale_mode][pitch%12]==1) {
                note = (pitch*MV +1.250)*4096/5 ;  // () gives the voltage desired, then we /5 x4096 to get I2C value               
                }
            else {pitch=pitch+1;
                note = ((pitch)*MV +1.250)*4096/5;}
            if (pulse_out==0) pulse_out=(4*SendParam-69)*2;
            if (SendMode==1) {ratcheting_steps=floor(SendParam/20-1);
                pulse_out=23;}
            else ratcheting_steps=0;
            }    
        }

    outputs();
    lastSendMode=SendMode;

    
    // Looping management
    if (millis() > time_now + interval) {
        //tempo_test(60);
        }
    while(millis() < time_now + interval){
        }
    }

float clamp(float x, float a, float b) {
  return (x < a) ? a : (b < x) ? b : x;
    }

void tempo_test(int y) {
      if (!testnote) { MIDImessage(noteON, y);
        testnote=!testnote;
        }
     else { MIDImessage(noteOFF, y);
        testnote=!testnote;
        }
    }

void LEDS(bool g1, bool g2, bool g3, bool r1, bool r2, bool r3) {
    if (g1) digitalWrite(5, HIGH);
    else digitalWrite(5, LOW);
    if (g2) digitalWrite(3, HIGH);
    else digitalWrite(3, LOW);
    if (g3) digitalWrite(7, HIGH);
    else digitalWrite(7, LOW);
    if (r1) digitalWrite(2, HIGH);
    else digitalWrite(2, LOW);
    if (r2) digitalWrite(6, HIGH);
    else digitalWrite(6, LOW);
    if (r3) digitalWrite(8, HIGH);
    else digitalWrite(8, LOW);
    }

void loop_leds() {
    if (looped==0) LEDS(0,0,0,0,0,0);
    else if (looped==3) LEDS(1,0,0,0,0,0);
    else if (looped==2) LEDS(0,1,0,0,0,0);
    else if (looped==1) LEDS(0,0,1,0,0,0);
    }

void scale_leds() {
    if (scale_mode==0) LEDS(0,0,0,0,0,0);
    else if (scale_mode==1) LEDS(0,0,0,0,0,1);
    else if (scale_mode==2) LEDS(0,0,0,0,1,0);
    else if (scale_mode==3) LEDS(0,0,0,0,1,1);
    else if (scale_mode==4) LEDS(0,0,0,1,0,0);
    else if (scale_mode==5) LEDS(0,0,0,1,0,1);
    else if (scale_mode==6) LEDS(0,0,0,1,1,0);
    else if (scale_mode==7) LEDS(0,0,0,1,1,1);
    }

void CV_leds() {
    if (CV_mode==0) LEDS(1,0,0,1,0,0);
    else if (CV_mode==1) LEDS(0,1,0,0,1,0);
    else if (CV_mode==2) LEDS(0,0,1,0,0,1);
    }

void DACout(byte l,byte m) {
    Wire.beginTransmission(DAC);
    Wire.write(b1); 
    Wire.write(l); 
    Wire.write(m); 
    Wire.endTransmission(); 
    }

