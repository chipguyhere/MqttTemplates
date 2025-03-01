// This per-project file needs to be COPIED to libraries/TFT_eSPI/tft_setup.h before compiling.
// This configures the library.  The library doesn't come with any tft_setup.h.
// It is OK to overwrite an existing file, which would just be a different project's file.  
// TFT_eSPI library contains preprocessor logic that adopts tft_setup.h as the setup file if it exists,
// ignoring all of the User_Setup*.h files.
// As a way of confirming that User_Setup.h was excluded, consider adding an #error directive to the top of it.



#define USER_SETUP_ID 12

//Setup file for the M5Stack Basic Core

#define tft_setup_for_sketch_2024jul28_tryTFT

#define ILI9341_DRIVER

#define M5STACK

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   14  // Chip select control pin
#define TFT_DC   27  // Data Command control pin
#define TFT_RST  33  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   32  // LED back-light

#define TFT_INVERSION_ON


#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT


#define SPI_FREQUENCY  27000000

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  5000000