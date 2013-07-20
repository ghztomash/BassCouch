/*
Bass Couch by Tomash Ghz
www.tomashg.com
ghz.tomash@gmail.com

Based on the FFT library http://neuroelec.com/2011/03/fft-library-for-arduino/

Analog signal is captured at 9.6 KHz, 64 spectrum bands each 150Hz which can be change from adcInit()

Original Fixed point FFT library is from ELM Chan, http://elm-chan.org/works/akilcd/report_e.html
A way to port it to the Arduino library and most demo codes are from AMurchick http://arduino.cc/forum/index.php/topic,37751.0.html
*/

#include <stdint.h>
#include <ffft.h>

#define  IR_AUDIO  0 // ADC channel to capture

volatile  byte  position = 0;
volatile  long  zero = 0;

int ledPin1=11;
int ledPin2=10;
int ledPin3=9;

int motorPin=3;
int motorPin2=5;

float rise;

float lowValue;
float lastLow=0;
float lastLow2=0;
float lastLow3=0;
int maxval=18; // was 400

float maxrise=80; // was 80  120

int16_t capture[FFT_N];			/* Wave captureing buffer */
complex_t bfly_buff[FFT_N];		/* FFT buffer */
uint16_t spektrum[FFT_N/2];		/* Spectrum output buffer */

void setup()
{
  pinMode(ledPin1,OUTPUT);
  pinMode(ledPin2,OUTPUT);
  pinMode(ledPin3,OUTPUT);
  pinMode(motorPin,OUTPUT);
  pinMode(motorPin2,OUTPUT);
  
  pinMode(13,OUTPUT);
  
  adcInit();
  adcCalb();
}

void loop()
{
  if (position == FFT_N)
  {
    fft_input(capture, bfly_buff);
    fft_execute(bfly_buff);
    fft_output(bfly_buff, spektrum);
    
    lowValue=(spektrum[1]+spektrum[2])/2; // get the low end of the spectrum
    
    if(lowValue>2){
      lowValue=constrain(lowValue,2,maxval); // scale the values a bit
      lowValue=map(lowValue,1,maxval,0,255);
    }
      else
        lowValue=0;
    
    rise=(lowValue-lastLow2)/2; // find the rate of change of the last two readings
    
    if(rise>10){
      rise=constrain(rise,10,maxrise); // scale the values a bit more
      rise=map(rise,10,maxrise,1,255);
    }else
     rise=0;
    
    //printGraph(rise);
    
    // trigger the VU meter LEDs
    if(rise<=85){
      analogWrite(ledPin1,map(rise,0,85,0,255));
      analogWrite(ledPin2,0);
      analogWrite(ledPin3,0);
    }else if(rise<=170){
      analogWrite(ledPin1,255);
      analogWrite(ledPin2,map(rise,85,170,0,255));
      analogWrite(ledPin3,0);
    }else{
      analogWrite(ledPin1,255);
      analogWrite(ledPin2,255);
      analogWrite(ledPin3,map(rise,170,255,0,255));
    }
    
    // trigger the motors
    if(rise>15){
      analogWrite(motorPin,map(rise,15,255,189,255));
      analogWrite(motorPin2,map(rise,15,255,189,255));
    }else{
      analogWrite(motorPin,16);
      analogWrite(motorPin2,16);
    }

    lastLow3=lastLow2;
    lastLow2=lastLow;
    lastLow=lowValue;
    
   position = 0;
  }
}

// free running ADC fills capture buffer
ISR(ADC_vect)
{
  if (position >= FFT_N)
    return;
  
  capture[position] = ADC + zero;
  if (capture[position] == -1 || capture[position] == 1)
    capture[position] = 0;

  position++;
}

// function to initialize the analog to digital converter
void adcInit(){
  /*  REFS0 : VCC use as a ref, IR_AUDIO : channel selection, ADEN : ADC Enable, ADSC : ADC Start, ADATE : ADC Auto Trigger Enable, ADIE : ADC Interrupt Enable,  ADPS : ADC Prescaler  */
  // free running ADC mode, f = ( 16MHz / prescaler ) / 13 cycles per conversion 
  ADMUX = _BV(REFS0) | IR_AUDIO; // | _BV(ADLAR); 
//  ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) //prescaler 64 : 19231 Hz - 300Hz per 64 divisions
  ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // prescaler 128 : 9615 Hz - 150 Hz per 64 divisions, better for most music
  sei();
}

// function to calibrate the signal
void adcCalb(){
  long midl = 0;
  digitalWrite(13,HIGH);
  // get 2 meashurment at 2 sec
  // on ADC input must be NO SIGNAL!!!
  for (byte i = 0; i < 2; i++)
  {
    position = 0;
    delay(100);
    midl += capture[0];
    delay(900);
  }
  zero = -midl/2;
  digitalWrite(13,LOW);
}

// function to draw out values to serial
void printGraph(int n)
  Serial.print(n);
  Serial.print(" \t ");
  
  for(int i=0;i<n;i++){
    Serial.print("|");
  }
  Serial.println("");
}
