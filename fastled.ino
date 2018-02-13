#define FASTLED_INTERRUPT_RETRY_COUNT 0 
#include "FastLED.h"

#define NUM_LEDS 50
#define DATA_PIN 3

#define MIN_INTERVAL 1000
#define MAX_INTERVAL 10000
#define MIN_ERA_CYCLES 2
#define MAX_ERA_CYCLES 3
#define ERA_COUNT 8

long minLightInterval = 0;
long maxLightInterval = 0;
long minDarkInterval = 0;
long maxDarkInterval = 0;
long eraInterval = 0;
long eraIndex = -1;
float minHue = 0.0f;
float maxHue = 6.0f;
bool acceptHue( float hue )
{
    if ( eraIndex == 0 )
    {
        return hue >= 5.75f || hue <= 0.25f;
    }
    else if ( eraIndex == 1 )
    {
        return hue >= 3.75f && hue <= 4.25f;
    }
    else if ( eraIndex == 2 )
    {
        return hue >= 1.75f && hue <= 2.25f;
    }
    else if ( eraIndex == 3 )
    {
        return hue >= 0.75f && hue <= 1.25f;
    }
    else 
    {
        return hue >= 0.75f && hue <= 1.0f ||
            hue >= 3.75f && hue <= 4.00f;
    }
    return true;
}

float minBrightness = 0.1f;
float maxBrightness = 0.8f;

CRGB leds[NUM_LEDS];
int interval[NUM_LEDS];
float phase[NUM_LEDS];
float caps[NUM_LEDS];
float hue[NUM_LEDS];

long lastMillis;;
long lastEraMillis;


void setup() 
{ 
    Serial.begin(115200); 
    randomSeed( analogRead( 0 ) );
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    for ( int i = 0; i < NUM_LEDS; i++ )
    {
        newCycle( i );
    }
    lastMillis = lastEraMillis = millis();
    newEra();
}

void newEra()
{
    lastEraMillis = lastMillis;
    minLightInterval = random( MIN_INTERVAL, MAX_INTERVAL );
    maxLightInterval = minLightInterval + random( MIN_INTERVAL, MAX_INTERVAL );
    minDarkInterval = random( MIN_INTERVAL, MAX_INTERVAL );
    maxDarkInterval = minDarkInterval + random( MIN_INTERVAL, MAX_INTERVAL );
    eraInterval = random( MIN_ERA_CYCLES, MAX_ERA_CYCLES ) * MAX_INTERVAL;
    eraIndex = ( eraIndex + 1 ) % ERA_COUNT;
}

void newCycle( int ledId )
{
    interval[ ledId ] = random( minLightInterval, maxLightInterval );

    while ( true )
    {
        hue[ ledId ] = minHue + (float)random( 0, 
            (long)( 10000.0f * ( maxHue - minHue ) ) ) / 10000.0f;
        if ( acceptHue( hue[ ledId ] ) ) break;
    }

    while ( hue[ ledId ] < 0.0f ) hue[ ledId ] += 6.0f;
    while ( hue[ ledId ] > 6.0f ) hue[ ledId ] -= 6.0f;

    phase[ ledId ] = -random( minDarkInterval, maxDarkInterval );
    caps[ ledId ] = minBrightness + random( 0, (int)( 
        10000 * ( maxBrightness - minBrightness ) ) ) / 10000.0f;
    Serial.print( String( eraIndex ) );
    Serial.print( ", " );
    Serial.println( String( hue[ ledId ] ) );
}

void advance( int ledId, long dt )
{
    long thisPhase = ( phase[ ledId ] += dt );
    if ( thisPhase > interval[ ledId ] )
    {
        newCycle( ledId );
    }
    thisPhase = phase[ ledId ];

    int r, g, b;
    float thisHue = hue[ ledId ];
    if ( thisHue < 1.0f )
    {
        r = 255;
        g = (int)( 255 * thisHue );
        b = 0;
    }
    else if ( thisHue < 2.0f )
    {
        r = (int)( 255 * ( 2.0f - thisHue ) );
        g = 255;
        b = 0;
    }
    else if ( thisHue < 3.0f )
    {
        r = 0;
        g = 255;
        b = (int)( 255 * ( thisHue - 2.0f ) );
    }
    else if ( thisHue < 4.0f )
    {
        r = 0;
        g = (int)( 255 * ( 4.0f - thisHue ) );
        b = 255;
    }
    else if ( thisHue < 5.0f )
    {
        r = (int)( 255 * ( thisHue - 4.0f ) );
        g = 0;
        b = 255;
    }
    else
    {
        r = 255;
        g = 0;
        b = (int)( 255 * ( 6.0f - thisHue ) );
    }

    float intensity = thisPhase < 0 ? 0.0f : 
        sin( (float)thisPhase / (float)interval[ ledId ] * 3.14 );
    intensity *= caps[ ledId ];

    if ( ledId == 0 )
    {
        // Serial.println( String( eraIndex ) );
        // Serial.print( ", " );
        // Serial.println( String( caps[ ledId ] ) );
    }

    if ( intensity < 0.5 )
    {
        r = (int)( r * intensity * 2 );
        g = (int)( g * intensity * 2 );
        b = (int)( b * intensity * 2 );
    }
    else 
    {
        r = 255 - (int)( ( 255 - r ) * ( 1.0f - ( intensity - 0.5f ) * 2 ) );
        g = 255 - (int)( ( 255 - g ) * ( 1.0f - ( intensity - 0.5f ) * 2 ) );
        b = 255 - (int)( ( 255 - b ) * ( 1.0f - ( intensity - 0.5f ) * 2 ) );
    }

    if ( r < 0 ) r = 0;
    if ( g < 0 ) g = 0;
    if ( b < 0 ) b = 0;
    if ( r > 255 ) r = 255;
    if ( g > 255 ) g = 255;
    if ( b > 255 ) b = 255;
    
    leds[ ledId ].r = r;
    leds[ ledId ].g = g;
    leds[ ledId ].b = b;
}

void loop()
{ 
    long now = millis();
    long dt = now - lastMillis;
    lastMillis = now;
    if ( now - lastEraMillis > eraInterval ) newEra();

    for ( int i = 0; i < NUM_LEDS; i++ )
    {
        advance( i, dt );
    }

    FastLED.show();
    delay( 16 );
}
