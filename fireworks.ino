//Fireworks, by Ian Wall, 2017
//
//Optimized for speed - uses FastLED sin8 and cos8 for speedy approximate sin and cos calculations,
//uses memcpy by Daniel Vik, which improves performance over standard library by ~8 times,
//pre-calculates constants, uses DMA buffer transfer, and uses swapable buffers as, ultimately, the 
//bottlenext here is in the transfer of the buffer to the display, despite using DMA. The second buffer 
//allows execution to continue whilst previous buffer is in transit to the display
// 
//Try changing:
//
//BYTESPERPIXEL  1 or 2. Sets the color depth to 8 or 16 bit. Faster performance at 1, better colors at 2
//arrSize        number of particles in each animation. 60-200 is a good range, but experiment!
//
//Creating new animations:
//
//Add a new case to the switch(type) statement, increment n in statement: type=random(n) so it is randomly picked :)
//Within case statement, initialize overall values for gravity, tInc, trails, steps and (in loop) angle and velocity per particle:
//gravity - simulates effect of gravity over time on all particles
//tInc - time increment during loop. Higher values speeds up all particles in animation
//trails - leaves particle pixel in buffer for a while, faded out over several steps. Smaller values will fade out particles quicker.
//Steps - number of frames to run animation for
//Loop for arrSize and set values per particle:
//  angle[] - the angle of the particles motion
//  velocity[] - how fast the particle will move
//
//Play around with random and silly values; I did! Be creative - type 5 below uses 2 colors and 
//sets up 2 different motions in one animation, using half the particles for each

//FastLED libraries for fast sin, cos approximations
#include "FastLED.h"

//Fast memcopy function
#include "memcopy.h"

//display libraries
#include <TinyScreen.h>

//Declare display object
TinyScreen display = TinyScreen(TinyScreenPlus);

//Display constants
#define SCREENWIDTH 96
#define SCREENHEIGHT 64
#define BYTESPERPIXEL 1

//Declare two buffers for screen output
uint8_t outBuffer[2][SCREENWIDTH * SCREENHEIGHT * BYTESPERPIXEL];

//Set initial buffer
uint8_t currBuffer=0;

//Values for timing
uint32_t startStartBufferMicros, endStartBufferMicros, totalStartBufferMicros;
uint32_t startBufferWaitMicros, endBufferWaitMicros, totalBufferWaitMicros;
uint32_t startEndBufferMicros, endEndBufferMicros, totalEndBufferMicros;
uint32_t startMicros, endMicros;

void setup() {

  SerialUSB.begin(115200);
  displayInitialize(); 
}

void loop() {
  fireWorks();
}

void fireWorks() {

  const float val4=128/PI;
  
  uint8_t x, y, xOff, yOff;
  float ti=0;
  float val1, val2, val3, yOffset;
  uint16_t frame;
  bool activeFound=false;

  uint16_t gravity;
  float tInc;
  uint16_t steps;
  uint8_t trails;

  //Number of particles set here
  const uint16_t arrSize=90;

  float angle[arrSize]={};
  float velocity[arrSize]={};
  bool active[arrSize]={};

  //Choose random color and center point
  uint16_t color16=colorWheel16bit(random(255));
  uint16_t color16_2;
  xOff=random(48)+24;
  yOff=random(32)+16;

  //Initialize all particles to active
  for (uint16_t i=0; i<arrSize; i++) {
    active[i]=true;
  }

  //Choose type
  uint8_t type=random(8); //0 to 7

  switch (type) {
    case 0: //Random burst
      gravity=4000; tInc=0.0020; trails=5; steps=90;
     
      for (uint16_t i=0; i<arrSize; i++) { 
        angle[i]=random(360);
        velocity[i]=random(400)+200;
      }
      break;
      
    case 1: //Circle, moving out
      gravity=750; tInc=0.0010; trails=5; steps=200;
     
      for (uint16_t i=0; i<arrSize; i++) { 
        angle[i]=i+random(i);
        velocity[i]=random(50)+300;
      }  
      break;
      
    case 2: //Star, moving out
      gravity=250; tInc=0.0015; trails=5; steps=90;

      for (uint16_t i=0; i<arrSize; i++) { 
        if (i%2==0) angle[i]=i/PI/4;
        else angle[i]=PI-i/PI/4;
        velocity[i]=550;
      }  
      break;
      
    case 3: //3 circles, moving out
      gravity=500; tInc=0.0012; trails=5; steps=80;

      val1 = (float)rand()/(float)(RAND_MAX/(2*PI));
      for (uint16_t i=0; i<arrSize; i++) { 
        if (i%2==0) angle[i]=val1+i;
        else angle[i]=val1-i;
        velocity[i]=(i+10)*20;
      }  
      break;
      
    case 4: //Random burst
      gravity=500; tInc=0.0042; trails=5; steps=110;

      for (uint16_t i=0; i<arrSize; i++) { 
        angle[i]=(float)rand()/(float)(RAND_MAX/(2*PI));
        velocity[i]=700*(sin8(arrSize-i)-128)/128;
      }  
      break;
      
    case 5: //Combo, 1 and 4
      color16_2=colorWheel16bit(random(255));
      
      gravity=50; tInc=0.0040; trails=5; steps=150;

      for (uint16_t i=0; i<arrSize/2; i++) { 
        angle[i]=i+random(i);
        velocity[i]=random(10)+50;
      }
      for (uint16_t i=arrSize/2; i<arrSize; i++) { 
        angle[i]=(float)rand()/(float)(RAND_MAX/(2*PI));
        velocity[i]=600*(sin8(arrSize-i)-128)/128;
      }  
      break;
      
    case 6: //Falling shower
      gravity=1000; tInc=0.0020; trails=5; steps=200;

      for (uint16_t i=0; i<arrSize; i++) { 
        angle[i]=(PI*1.5) + ((float)rand()/(float)(RAND_MAX/(PI/4))) - PI/8;
        velocity[i]=random(200)+200;
      }
      break;
      
    case 7: //Star spiral
      gravity=500; tInc=0.0025; trails=5; steps=90;
     
      for (uint16_t i=0; i<arrSize; i++) { 
        angle[i]=i/PI;
        velocity[i]=i*5;
      }
      break;
  }

  //Clear display and initialize buffers
  startBuffer(0);

  totalStartBufferMicros=0;
  totalEndBufferMicros=0;
  totalBufferWaitMicros=0;
  startMicros=micros();

  //Main loop, increments timw, dims particles in buffer
  for (frame=0; frame<steps; frame+=1) {
    
    ti+=tInc;

    startStartBufferMicros=micros();
    if (frame%trails!=0) {
      startBuffer(-1);
    }
    else {
      startBuffer(1);  
  
      if (frame>(steps-(trails*5))) {
        color16=dim16(color16, 1);
        color16_2=dim16(color16_2, 1);
      }      
    }
    endStartBufferMicros=micros();
    totalStartBufferMicros+=endStartBufferMicros-startStartBufferMicros;

    //Calculate offset for gravity
    yOffset=gravity*(ti*ti);
    val3=ti/128;
    
    activeFound=false;

    //Particle loop
    for (uint16_t i=0; i<arrSize; i++) {

      //Calculate new x, y location for particle
      val1=velocity[i]*val3; val2=angle[i]*val4;
      x=val1 * (cos8(val2)-128);
      y=val1 * (sin8(val2)-128)+yOffset;

      //Set color      
      uint16_t col=color16;
      if (type==5 && i<arrSize/2) col=color16_2;

      //Draw particle if active. drawPixel returns false if out of bounds
      if (active[i]==true) {
        active[i]=drawPixel(xOff+x, yOff+y, col);
        activeFound=true;
      }
    }
    
    //Buffer drawing complete, transmit
    startEndBufferMicros=micros();
    endBuffer();
    endEndBufferMicros=micros();
    totalEndBufferMicros+=endEndBufferMicros-startEndBufferMicros;

    //If no particles are active, quit loop
    if (!activeFound) {
      break;
    }
  }
  endMicros=micros();

  //Stats
  SerialUSB.print(type); SerialUSB.print(": "); SerialUSB.print(1000000/((endMicros-startMicros)/frame)); 
  SerialUSB.print("fps, frames: "); SerialUSB.print(frame); SerialUSB.print(", time: "); SerialUSB.print(endMicros-startMicros);
  SerialUSB.print(", endBuf: "); SerialUSB.print(totalEndBufferMicros); SerialUSB.print(" ("); 
  SerialUSB.print((float)totalEndBufferMicros/(endMicros-startMicros)*100); 
  SerialUSB.print("%), waitEndBuf: "); SerialUSB.print(totalBufferWaitMicros); SerialUSB.print(" ("); 
  SerialUSB.print((float)totalBufferWaitMicros/(endMicros-startMicros)*100); 
  SerialUSB.print("%), startBuf: ");SerialUSB.print(totalStartBufferMicros); SerialUSB.print(" ("); 
  SerialUSB.print((float)totalStartBufferMicros/(endMicros-startMicros)*100); 
  SerialUSB.print("%), body: "); SerialUSB.print(endMicros-startMicros-totalEndBufferMicros-totalStartBufferMicros); SerialUSB.print(" ("); 
  SerialUSB.print((float)(endMicros-startMicros-totalEndBufferMicros-totalStartBufferMicros)/(endMicros-startMicros)*100); 
  SerialUSB.println("%)");
}


////////////////////
// Display functions
////////////////////

void displayInitialize() {
  //display setup
  display.begin();

  //Set display color bit depth - 0 is 8 bits, 1 is 16 bits
  display.setBitDepth(BYTESPERPIXEL-1);

  //Set 16 bit color mode to RGB
  display.setColorMode(TSColorModeRGB);

  //Invert screen
  display.setFlip(true);
  display.setBrightness(15);

  //Init DMA
  display.initDMA();    
}

//Param mul determines clearing or dimming of buffer. -1: no clear, 0: clear, >0: dim mul bits
void startBuffer(int8_t mul) {

  //mul==0, clear both buffers
  if (mul==0) {
    memset(outBuffer[0], 0, SCREENWIDTH * SCREENHEIGHT * BYTESPERPIXEL * 2);    
    return;
  }
  
  //copy previous buffer to current buffer to preserve fade effects. Approx 8 times faster than standard memcpy
  memcopy(outBuffer[currBuffer], outBuffer[1-currBuffer], SCREENWIDTH * SCREENHEIGHT * BYTESPERPIXEL);
      
  //If no fade required, leave
  if (mul<0) {
    return;
  }

  //Fade RGB components by bitshifting color components right by mul bits
  for (uint16_t l=0; l<SCREENWIDTH * SCREENHEIGHT * BYTESPERPIXEL; l+=BYTESPERPIXEL) {

    if (BYTESPERPIXEL==2) {
      uint16_t col16=(outBuffer[currBuffer][l]<<8) | outBuffer[currBuffer][l+1];
      if (col16 !=0) {
        col16=dim16(col16, mul);
        outBuffer[currBuffer][l]=col16 >> 8;
        outBuffer[currBuffer][l+1]=col16;   
      }         
    }
    else
    {
      uint8_t col8=outBuffer[currBuffer][l];
      if (col8 !=0) {
        col8=dim8(col8, mul);
        outBuffer[currBuffer][l]=col8;
      }
    }
  }
}

//Called after buffer drawing is complete, waits to transfer nexr buffer
void endBuffer() {
  
  //Wait for previous transfer to complete
  startBufferWaitMicros=micros();
  while( !display.getReadyStatusDMA());  //30-50% of code execution time spent here!
  endBufferWaitMicros=micros();
  totalBufferWaitMicros+=endBufferWaitMicros-startBufferWaitMicros;
  display.endTransfer();  

  //Start new transfer
  display.goTo(0, 0);
  display.startData();
  display.writeBufferDMA(outBuffer[currBuffer], SCREENWIDTH * SCREENHEIGHT * BYTESPERPIXEL);

  //Flip buffer, so can fill new buffer whilst waiting for previous DMA buffer transfer to complete
  currBuffer=1-currBuffer;
}

//Places a bounds-checked pixel in the buffer
bool drawPixel(int8_t x, int8_t y, uint16_t color) {

  //Bounds check and 'kill' pixel if out of bounds
  if (x<0 || x>=SCREENWIDTH || y<0 || y>=SCREENHEIGHT) {
    return false;
  }
  else {
    uint16_t bOffset=(x * BYTESPERPIXEL) + (y * SCREENWIDTH * BYTESPERPIXEL);
    if (BYTESPERPIXEL == 2) {
      outBuffer[currBuffer][bOffset] = color>>8;
      outBuffer[currBuffer][bOffset+1] = color;
    }
    else {
      outBuffer[currBuffer][bOffset] = (color) & 0x03 | ((color >> 8) & 0x07) << 2 | ((color >> 13) & 0x07) << 5;
    }
    return true; 
  }
}

/////////////////
//Color functions
/////////////////

//Returns a dimmed version of 16 bit color
uint16_t dim16(uint16_t col16, uint8_t mul) {
  return ((((col16 >> 11) & 0x1F)>>mul)<<11) | ((((col16>>5) & 0x3F)>>mul)<<5) | (col16 & 0x1F)>>mul;
}

//Returns a dimmed version of 8 bit color
uint8_t dim8(uint8_t col8, uint8_t mul) {
  return ((((col8 >> 5) & 7)>>mul)<<5) | ((((col8>>2) & 7)>>mul)<<2) | (col8 & 3)>>mul;
}

//Input a value 0 to 255 to get a 16 bit color value. The colours are a transition r - g - b - back to r.
uint16_t colorWheel16bit(uint8_t wheelPos) {

  if (wheelPos < 85) {
    return (wheelPos*3 & 0xF8)<<8  | (255-wheelPos*3 & 0xFC)<<3 | 0>>3;
  }
  else if (wheelPos < 170) {
    return (255-wheelPos*3 & 0xF8)<<8 | 0<<3 | ((wheelPos*3 & 0xF8)>>3) ;
  }
  else {
    wheelPos -= 170;
    return 0<<8 | ((wheelPos*3 & 0xFC)<<3) | ((255-wheelPos*3 & 0xF8)>>3);
  }
}

