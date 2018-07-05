#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <FS.h>
extern "C" {
#include "user_interface.h"
}

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(...)    Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

#define LED_PIN 4
#define BUTTON_PIN 5

/* Set these to your desired credentials. */
const char *ssid = "ESPap";
const char *password = "thereisnospoon";

Adafruit_NeoPixel panel = Adafruit_NeoPixel(64, LED_PIN, NEO_GRB + NEO_KHZ800);

String zipCode = "10001";
String appId = "<youridhere>";

char urlhost[] = "api.openweathermap.org";

ESP8266WebServer server(80);
WiFiClient client;

struct bmp_file_header_t {
  uint16_t signature;
  uint32_t file_size;
  uint16_t reserved[2];
  uint32_t image_offset;
};

struct bmp_image_header_t {
  uint32_t header_size;
  uint32_t image_width;
  uint32_t image_height;
  uint16_t color_planes;
  uint16_t bits_per_pixel;
  uint32_t compression_method;
  uint32_t image_size;
  uint32_t horizontal_resolution;
  uint32_t vertical_resolution;
  uint32_t colors_in_palette;
  uint32_t important_colors;
};

void displayBitmap( String imageName ) {
  File bmpImage = SPIFFS.open( imageName, "r" );
  if ( !bmpImage ) DEBUG_PRINTLN( "Failed to open " + imageName );
  bmp_file_header_t fileHeader;
  char fileHeaderBuffer[ 4 ];
  bmpImage.readBytes( fileHeaderBuffer, 2 );
  memcpy( &fileHeader.signature, fileHeaderBuffer, 2 );
  bmpImage.readBytes( fileHeaderBuffer, 4 );
  memcpy( &fileHeader.file_size, fileHeaderBuffer, 4 );
  bmpImage.readBytes( fileHeaderBuffer, 2 );
  memcpy( &fileHeader.reserved[ 0 ], fileHeaderBuffer, 2 );
  bmpImage.readBytes( fileHeaderBuffer, 2 );
  memcpy( &fileHeader.reserved[ 1 ], fileHeaderBuffer, 2 );
  bmpImage.readBytes( fileHeaderBuffer, 4 );
  memcpy( &fileHeader.image_offset, fileHeaderBuffer, 4 );
  DEBUG_PRINT( "BMP Signature: " ); DEBUG_PRINTLN( fileHeader.signature, HEX );
  DEBUG_PRINT( "BMP File Size: " ); DEBUG_PRINTLN( fileHeader.file_size );
  DEBUG_PRINT( "BMP Reserved 0: " ); DEBUG_PRINTLN( fileHeader.reserved[0], HEX );
  DEBUG_PRINT( "BMP Image Offset: " ); DEBUG_PRINTLN( fileHeader.image_offset, HEX );
  bmp_image_header_t imageHeader;
  char imageHeaderBuffer[ 4 ];
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.header_size, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.image_width, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.image_height, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 2 );
  memcpy( &imageHeader.color_planes, imageHeaderBuffer, 2 );
  bmpImage.readBytes( imageHeaderBuffer, 2 );
  memcpy( &imageHeader.bits_per_pixel, imageHeaderBuffer, 2 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.compression_method, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.image_size, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.horizontal_resolution, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.vertical_resolution, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.colors_in_palette, imageHeaderBuffer, 4 );
  bmpImage.readBytes( imageHeaderBuffer, 4 );
  memcpy( &imageHeader.important_colors, imageHeaderBuffer, 4 );
  DEBUG_PRINT( "BMP Header Size: " ); DEBUG_PRINTLN( imageHeader.header_size );
  DEBUG_PRINT( "BMP Image Width: " ); DEBUG_PRINTLN( imageHeader.image_width );
  DEBUG_PRINT( "BMP Image Height: " ); DEBUG_PRINTLN( imageHeader.image_height );
  DEBUG_PRINT( "BMP Color Planes: " ); DEBUG_PRINTLN( imageHeader.color_planes, HEX );
  DEBUG_PRINT( "BMP Bits Per Pixel: " ); DEBUG_PRINTLN( imageHeader.bits_per_pixel );
  DEBUG_PRINT( "BMP Compression Method: " ); DEBUG_PRINTLN( imageHeader.compression_method, HEX );
  DEBUG_PRINT( "BMP Image Size: " ); DEBUG_PRINTLN( imageHeader.image_size );
  DEBUG_PRINT( "BMP Horizontal Resolution: " ); DEBUG_PRINTLN( imageHeader.horizontal_resolution );
  DEBUG_PRINT( "BMP Vertical Resolution: " ); DEBUG_PRINTLN( imageHeader.vertical_resolution );
  DEBUG_PRINT( "BMP Colors in Palette: " ); DEBUG_PRINTLN( imageHeader.colors_in_palette, HEX );
  DEBUG_PRINT( "BMP Important Colors: " ); DEBUG_PRINTLN( imageHeader.important_colors, HEX );

  if ( imageHeader.bits_per_pixel == 8 ) {
    //patc this
    byte palette[ imageHeader.colors_in_palette * 4 ];
    for ( int i = 0; i < ( imageHeader.colors_in_palette * 4 ); i++ ) {
      palette[ i ] = bmpImage.read();
    }
    for ( int i = 0; i < 64; i++ ) {
      byte index = bmpImage.read();
      char b = palette[ ( index * 4 )  ];
      char g = palette[ ( index * 4 ) + 1 ];
      char r = palette[ ( index * 4 ) + 2];
      panel.setPixelColor( ( 7 - ( i % 8 ) ) + ( i / 8 ) * 8, r, g, b );
    }
  }
  else if ( imageHeader.bits_per_pixel == 24 ) {
    bmpImage.seek( fileHeader.image_offset, SeekSet );
    for ( int i = 0; i < 64; i++ ) {
      char b = bmpImage.read();
      char g = bmpImage.read();
      char r = bmpImage.read();
      panel.setPixelColor( ( 7 - ( i % 8 ) ) + ( i / 8 ) * 8, r, g, b );
    }
  }
  else Serial.println( "Unknown bit depth" );
  panel.show();
}

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void setup() {
  delay( 1000 );
  Serial.begin( 115200 );
  DEBUG_PRINTLN( "Serial Online" );
  panel.begin();
  panel.show();
  DEBUG_PRINTLN( "Pixels Online" );
  if ( !SPIFFS.begin() ) DEBUG_PRINTLN( "Failed to mount file system" );
  displayBitmap( "/splash.bmp" );

  DEBUG_PRINTLN();
  DEBUG_PRINT("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  DEBUG_PRINT("AP IP address: ");
  DEBUG_PRINTLN(myIP);
  server.on("/", handleRoot);
  server.begin();
  DEBUG_PRINTLN("HTTP server started");
}

void loop() {
  for ( int i = 1; i < 5; i++ ) {
    String filename = "/sun/" + String( i ) + ".bmp";
    displayBitmap( filename );
    delay( 500 );

    server.handleClient();
  }

}
