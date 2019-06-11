#include <FastLED.h>

#define LED_PIN     A3
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define NUM_LEDS    258

#define BRIGHTNESS  63
#define FRAMES_PER_SECOND 60

bool gReverseDirection = false;

CRGB leds[NUM_LEDS];
CRGBPalette16 gPal;

#include <ButtonDebounce.h>

ButtonDebounce button(A2, 50); //pin A2, 50ms debounce.
bool buttonDown = 0;

#include <timer.h>

Timer<> timer;
bool patternActive = 0;
bool patternFadeout = 0;
char stage = 0;
bool stageChange = 0;

void setup() {
  delay(2000); //sanity delay

  Serial.begin(115200);

  pinMode(16, INPUT_PULLUP); //button pin, normally open.
  
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  gPal = HeatColors_p;
  
  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  //gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);

}

void loop()
{
  button.update();
  timer.tick();

  if(button.state() == LOW) //button was pressed
  {
    if(!buttonDown)
    {
      buttonDown = 1;
      Serial.println("Pressed\n");
      if(!stage)
      {
        stage = 1;
        stageChange = 1;
      }
    }
  }
  else
  {
    if(buttonDown)
    {
      buttonDown = 0;
      Serial.println("Released\n");
    }
  }

  if(stageChange)
  {
    Serial.print("Stage ");
    Serial.print(stage);
    Serial.print('\n');
    
    switch(stage)
    {
      case 1:
        patternActive = 1;
        timer.in(3000, patternFadeoutFcn);
      break;
  
      case 2:
        patternActive = 0;
        patternFadeout = 1;
        timer.in(3000, patternClearFcn);
      break;
  
      case 0:
        patternActive = 0;
        patternFadeout = 0;
      default:
  
      break;
    }

    stageChange = 0;
  }

  if(patternActive)
  {
    random16_add_entropy(random());
  
    // Fourth, the most sophisticated: this one sets up a new palette every
    // time through the loop, based on a hue that changes every time.
    // The palette is a gradient from black, to a dark color based on the hue,
    // to a light color based on the hue, to white.
    //
    //   static uint8_t hue = 0;
    //   hue++;
    //   CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
    //   CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
    //   gPal = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);
  
  
    Fire2012WithPalette(); // run simulation frame, using palette colors
  }

  if(patternFadeout)
  {
    random16_add_entropy(random());
    Fire2012WithPaletteFadeout(); // run simulation frame, using palette colors
  }
  
  if(patternActive || patternFadeout) //while any pattern stage timer is active.
  {
    FastLED.show(); // display this frame
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  }
}


// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
#define COOLING  80

// More sparking = more roaring fire.  Less sparking = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 75

byte heat[NUM_LEDS];

void Fire2012WithPalette()
{
// Array of temperature readings at each simulation cell

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS/2; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber + NUM_LEDS/2] = color; //start at middle of string.
      leds[NUM_LEDS/2 - pixelnumber] = color; //reverse and copy.
    }
}

bool patternFadeoutFcn(void *)
{ 
  stage++;
  stageChange = 1;
  return false; //no repeat.
}

void Fire2012WithPaletteFadeout()
{

  int cooling = 255;
  
    // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
    }
    
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS/2; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (NUM_LEDS-1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber + NUM_LEDS/2] = color; //start at middle of string.
    leds[NUM_LEDS/2 - pixelnumber] = color; //reverse and copy.
  }

}

bool patternClearFcn(void *)
{ 
  patternFadeout = 0;
  stage = 0;
  stageChange = 1;

  FastLED.clear();
  FastLED.show();

  //reset fire values.
  for( int i = 0; i < NUM_LEDS; i++)
  {
      heat[i] = 0;
  }

  return false; //no repeat.
}
