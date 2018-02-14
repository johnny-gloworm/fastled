#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "credentials.h"

// credentials.h provide these definitions:
// #define WIFI_SSID "xxx"
// #define WIFI_PASSWORD "yyy"
// #define AQI_TOKEN "zzz"

#define NUM_LEDS 50
#define DATA_PIN 3

#define REQUEST_STATE_INITIAL 0
#define REQUEST_STATE_JUST_SENT 1
#define REQUEST_STATE_GOT_DATA 2


HTTPClient http;
CRGB leds[NUM_LEDS];
long aqis[NUM_LEDS];
long aqi = 0;

long stations[] =
{
    245,    // LA
    6323,   // Denver
    6298,   // Miami
    7377,   // WV
    5102,   // Buffalo
    7949,   // London
    6037,   // Munich
    9439,   // Rome
    6132,   // Berlin
    3399,   // Marszalkowska
    4143,   // Istanbul
    3782,   // Abu Dhabi
    7020,   // Mumbai
    9473,   // Chiang Mai
    1857,   // Bangkok
    1666,   // Singapore
    3305,   // Guangzhou
    3303,   // Beijing
    4487,   // Seoul
    2413,   // Tokyo
    9303,   // Auckland
};

struct ColorStop
{
    long stop;
    CRGB color;
};

ColorStop colorStops[] =
{
    { 0, CRGB( 0, 255, 0 ) },
    { 50, CRGB( 255, 255, 0) },
    { 100, CRGB( 255, 64, 0 ) },
    { 150, CRGB( 255, 0 ,0 ) },
    { 200, CRGB( 64, 0, 65 ) },
    { 250, CRGB( 0, 0, 128 ) }
};

struct RequestState
{
    int currentId;
    int state;
} requestState;



void setup()
{
    Serial.begin(115200);
    WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    for ( int i = 0; i < NUM_LEDS; i++ )
    {
        aqis[ i ] = -1;
    }
    showLeds();

    while ( WiFi.status() != WL_CONNECTED )
    {
        delay( 1000 );
        Serial.println( "Connecting..." );
    }
    Serial.println( "Connected!" );

    randomSeed( analogRead( 0 ) );
}

void loop()
{
    aqi++;
    if ( aqi == 300 ) aqi = 0;

    for ( int i = 0; i < NUM_LEDS; i++ )
    {
        aqis[ i ] = aqi;
    }

    delay( 200 );
    Serial.println( aqi );

    // processHttp();
    showLeds();
    // delay( requestState.currentId == 0 ? 10000 : 0 );
}

void showLeds()
{
    int stationCount = sizeof ( stations ) / sizeof ( long );
    for ( int i = 0; i < stationCount; i++ )
    {
        leds[ i ] = aqis[ i ] < 0 ?
            CRGB( 0, 0, 0 ) : getColor( aqis[ i ], colorStops );
        // Serial.print( i );
        // Serial.print( ": " );
        // Serial.print( aqis[ i ] );
        // Serial.print( ": " );
        // Serial.print( leds[ i ].r );
        // Serial.print( "," );
        // Serial.print( leds[ i ].g );
        // Serial.print( "," );
        // Serial.println( leds[ i ].b );
    }
    FastLED.show();
};

void processHttp()
{
    int stationId = stations[ requestState.currentId ];

    Serial.print( "Fetching data for station #" );
    Serial.println( stationId );
    String url =
        String( "http://api.waqi.info/feed/@" ) +
        String( stationId ) +
        String( "/?token=" ) +
        String( AQI_TOKEN );
    http.begin( url );
    Serial.println( url );

    int httpCode = http.GET();
    if ( httpCode > 0 )
    {
        String payload = http.getString();
        int startIndex = payload.indexOf( "\"aqi\":" );
        int endIndex = payload.indexOf( ",", startIndex + 6 );
        String substr = payload.substring( startIndex + 6, endIndex );
        aqis[ requestState.currentId ] = substr == "\"-\"" ? -1 : substr.toInt();
        Serial.println( substr );
    }
    else
    {
        Serial.println( "error" );
    }
    http.end();

    requestState.currentId = ( requestState.currentId + 1 ) %
        ( sizeof ( stations ) / sizeof ( long ) );
}

CRGB getColor( long value, ColorStop* stops )
{
    if ( value <= stops[ 0 ].stop ) return stops[ 0 ].color;
    int stopCount = sizeof ( colorStops ) / sizeof( ColorStop );

    for ( int i = 1; i < stopCount; i++ )
    {
        if ( value <= stops[ i ].stop )
        {
            float fraction = (float)( value - stops[ i - 1 ].stop ) /
                (float)( stops[ i ].stop - stops[ i - 1 ].stop );
            int r = (int)( fraction * stops[ i ].color.r + ( 1.0f - fraction ) * stops[ i - 1 ].color.r );
            int g = (int)( fraction * stops[ i ].color.g + ( 1.0f - fraction ) * stops[ i - 1 ].color.g );
            int b = (int)( fraction * stops[ i ].color.b + ( 1.0f - fraction ) * stops[ i - 1 ].color.b );
            return CRGB( r, g, b );
        }
    }
    return stops[ stopCount - 1 ].color;
}
