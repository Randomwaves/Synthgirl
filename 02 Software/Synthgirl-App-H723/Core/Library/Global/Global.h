#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <cmath>

#include "fatfs.h"
#include "main.h"
#include "sdmmc.h"
#include "string.h"

////////////////////////////////////////////////////////////////////////////////
/* Application Version -------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kVersionMajor = 0x01;
const uint8_t kVersionMinor = 0x01;

////////////////////////////////////////////////////////////////////////////////
/* Global Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

#define STM32_UNIQUE_ID ((uint32_t *)0x1FF1E800)

const int32_t INT24_MAX = 8388607;
const int32_t INT24_MIN = -8388608;

// (*STM32_UNIQUE_ID) == 2424897;
// (*STM32_UNIQUE_ID) == 2424884;

// volatile uint16_t* ptr = (volatile uint16_t*)(kOscBasic_RamAddress_Library[0]);
// viewRef.debug(0, *(ptr + 4), 5);

template <typename T, int N>
int size(T (&a)[N]) {
    return N - 1;
}

template <typename T>
void swap(T &a, T &b) {
    T temp = a;
    a = b;
    b = temp;
}

const int32_t kAudioSampleRate = 44100;

typedef enum {
    FILE_NONE = 0x00,
    FILE_MISSING = 0x01,
    FILE_INCOMPATIBLE = 0x02,
    FILE_INACTIVE = 0x03,
    FILE_ACTIVE = 0x04,
} FileStatus;

#define LED0_ON HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET)
#define LED1_ON HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED2_ON HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET)
#define SYNC_OUT_ON HAL_GPIO_WritePin(SYNC_OUT_GPIO_Port, SYNC_OUT_Pin, GPIO_PIN_SET)

#define LED0_OFF HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET)
#define LED1_OFF HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED2_OFF HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)
#define SYNC_OUT_OFF HAL_GPIO_WritePin(SYNC_OUT_GPIO_Port, SYNC_OUT_Pin, GPIO_PIN_RESET)

#define LED0_TOGGLE HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin)
#define LED1_TOGGLE HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin)
#define LED2_TOGGLE HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin)

////////////////////////////////////////////////////////////////////////////////
/* Ram Addresses -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

/* ITCMRAM 0x00000000 64KB ---------------------------------------------------*/

/* DTCMRAM 0x20000000 128KB --------------------------------------------------*/

/* RAM_D1 0x24000000 320KB ---------------------------------------------------*/

/* RAM_D2 0x30000000 32KB ----------------------------------------------------*/

#define RAM_ICON_SELECT_ADDRESS 0x300011B0
#define RAM_ICON_SELECT_PALETTE_ADDRESS 0x300011B0 // 128 bytes
#define RAM_ICON_SELECT_DATA_ADDRESS 0x30001230
#define RAM_ICON_SELECT_ON_DATA_ADDRESS 0x30001230  // 121 bytes
#define RAM_ICON_SELECT_OFF_DATA_ADDRESS 0x300012A9 // 121 bytes

#define RAM_ICON_ALERT_ADDRESS 0x30001322
#define RAM_ICON_ALERT_PALETTE_ADDRESS 0x30001322 // 128 bytes
#define RAM_ICON_ALERT_DATA_ADDRESS 0x300013A2
#define RAM_ICON_ALERT_L_DATA_ADDRESS 0x300013A2 // 361 bytes
#define RAM_ICON_ALERT_R_DATA_ADDRESS 0x3000150B // 361 bytes

#define RAM_ICON_PLAY_ADDRESS 0x30001674
#define RAM_ICON_PLAY_PALETTE_ADDRESS 0x30001674 // 128 bytes
#define RAM_ICON_PLAY_DATA_ADDRESS 0x300016F4
#define RAM_ICON_PLAY_RESET_ON_DATA_ADDRESS 0x300016F4   // 81 bytes
#define RAM_ICON_PLAY_STOP_ON_DATA_ADDRESS 0x30001745    // 81 bytes
#define RAM_ICON_PLAY_PLAY_ON_DATA_ADDRESS 0x30001796    // 81 bytes
#define RAM_ICON_PLAY_RECORD_ON_DATA_ADDRESS 0x300017E7  // 81 bytes
#define RAM_ICON_PLAY_RESET_OFF_DATA_ADDRESS 0x30001838  // 81 bytes
#define RAM_ICON_PLAY_STOP_OFF_DATA_ADDRESS 0x30001889   // 81 bytes
#define RAM_ICON_PLAY_PLAY_OFF_DATA_ADDRESS 0x300018DA   // 81 bytes
#define RAM_ICON_PLAY_RECORD_OFF_DATA_ADDRESS 0x3000192B // 81 bytes

#define RAM_BUTTON_KEY_ADDRESS 0x300019AB         // 99px * 26 px
#define RAM_BUTTON_KEY_PALETTE_ADDRESS 0x300019AB // 128 bytes
#define RAM_BUTTON_KEY_DATA_ADDRESS 0x30001A2B
#define RAM_BUTTON_KEY_OFF_DATA_ADDRESS 0x30001A2B // 1287 bytes
#define RAM_BUTTON_KEY_ON_DATA_ADDRESS 0x30001F32  // 1287 bytes

#define RAM_BUTTON_OSC_A_ADDRESS 0x30002439         // 208px * 39 px
#define RAM_BUTTON_OSC_A_PALETTE_ADDRESS 0x30002439 // 128 bytes
#define RAM_BUTTON_OSC_A_DATA_ADDRESS 0x300024B9
#define RAM_BUTTON_OSC_A_0_DATA_ADDRESS 0x300024B9 // 2704 bytes
#define RAM_BUTTON_OSC_A_1_DATA_ADDRESS 0x30002F49 // 2704 bytes
#define RAM_BUTTON_OSC_A_2_DATA_ADDRESS 0x300039D9 // 2704 bytes

#define RAM_BUTTON_OSC_B_ADDRESS 0x30004469         // 208px * 39 px
#define RAM_BUTTON_OSC_B_PALETTE_ADDRESS 0x30004469 // 128 bytes
#define RAM_BUTTON_OSC_B_DATA_ADDRESS 0x300044E9
#define RAM_BUTTON_OSC_B_0_DATA_ADDRESS 0x300044E9 // 2704 bytes
#define RAM_BUTTON_OSC_B_1_DATA_ADDRESS 0x30004F79 // 2704 bytes
#define RAM_BUTTON_OSC_B_2_DATA_ADDRESS 0x30005A09 // 2704 bytes

#define RAM_BUTTON_FILTER_ADDRESS 0x30006499         // 99px * 26 px
#define RAM_BUTTON_FILTER_PALETTE_ADDRESS 0x30006499 // 128
#define RAM_BUTTON_FILTER_DATA_ADDRESS 0x30006519
#define RAM_BUTTON_FILTER_OFF_DATA_ADDRESS 0x30006519 // 1287 bytes
#define RAM_BUTTON_FILTER_ON_DATA_ADDRESS 0x30006A20  // 1287 bytes

#define RAM_BUTTON_ENVELOPE_ADDRESS 0x30006F27         // 99px * 26 px
#define RAM_BUTTON_ENVELOPE_PALETTE_ADDRESS 0x30006F27 // 128 bytes
#define RAM_BUTTON_ENVELOPE_DATA_ADDRESS 0x30006FA7
#define RAM_BUTTON_ENVELOPE_OFF_DATA_ADDRESS 0x30006FA7 // 1287 bytes
#define RAM_BUTTON_ENVELOPE_ON_DATA_ADDRESS 0x300074AE  // 1287 bytes

// 0x300079B5 - 0x30008000  free space

/* RAM_D3 0x38000000 16KB ----------------------------------------------------*/

#define RAM_BUTTON_OSC_A_LFO_ADDRESS 0x38000000         // 99px * 26 px
#define RAM_BUTTON_OSC_A_LFO_PALETTE_ADDRESS 0x38000000 // 128 bytes
#define RAM_BUTTON_OSC_A_LFO_DATA_ADDRESS 0x38000080
#define RAM_BUTTON_OSC_A_LFO_OFF_DATA_ADDRESS 0x38000080 // 1287 bytes
#define RAM_BUTTON_OSC_A_LFO_ON_DATA_ADDRESS 0x38000587  // 1287 bytes

#define RAM_BUTTON_OSC_B_LFO_ADDRESS 0x38000A8E         // 99px * 26 px
#define RAM_BUTTON_OSC_B_LFO_PALETTE_ADDRESS 0x38000A8E // 128 bytes
#define RAM_BUTTON_OSC_B_LFO_DATA_ADDRESS 0x38000B0E
#define RAM_BUTTON_OSC_B_LFO_OFF_DATA_ADDRESS 0x38000B0E // 1287 bytes
#define RAM_BUTTON_OSC_B_LFO_ON_DATA_ADDRESS 0x38001015  // 1287 bytes

#define RAM_BUTTON_SONG_ADDRESS 0x3800151C         // 99px * 26 px
#define RAM_BUTTON_SONG_PALETTE_ADDRESS 0x3800151C // 128 bytes
#define RAM_BUTTON_SONG_DATA_ADDRESS 0x3800159C
#define RAM_BUTTON_SONG_OFF_DATA_ADDRESS 0x3800159C // 1287 bytes
#define RAM_BUTTON_SONG_ON_DATA_ADDRESS 0x38001AA3  // 1287 bytes

// 0x38001FAA - 0x38004000  free space

/* SDRAM 0xC00000000 128MB ---------------------------------------------------*/

#define RAM_WAVETABLE_ADDRESS 0xC0000000

const uint32_t kWavetableSize = 2048 * 256;
const uint32_t kWavetableByteSize = kWavetableSize * 2;

#define RAM_WAVETABLE_0A RAM_WAVETABLE_ADDRESS
#define RAM_WAVETABLE_0B RAM_WAVETABLE_ADDRESS + (1 * kWavetableByteSize)
#define RAM_WAVETABLE_1A RAM_WAVETABLE_ADDRESS + (2 * kWavetableByteSize)
#define RAM_WAVETABLE_1B RAM_WAVETABLE_ADDRESS + (3 * kWavetableByteSize)

const uint32_t kRamWavetableAddressLibrary[2][2] = {
    {RAM_WAVETABLE_0A, RAM_WAVETABLE_0B},
    {RAM_WAVETABLE_1A, RAM_WAVETABLE_1B}};

#define RAM_DELAY_ADDRESS 0xC0400000

const uint32_t kDelaySize = 96000;
const uint32_t kDelayByteSize = kDelaySize * 4;

#define RAM_DELAY_0 RAM_DELAY_ADDRESS
#define RAM_DELAY_1 RAM_DELAY_ADDRESS + (1 * kDelayByteSize)

#define RAM_CHORUS_ADDRESS 0xC04BB800

const uint32_t kChorusSize = 24000;
const uint32_t kChorusByteSize = kChorusSize * 4;

#define RAM_CHORUS_0 RAM_CHORUS_ADDRESS
#define RAM_CHORUS_1 RAM_CHORUS_ADDRESS + (1 * kChorusByteSize)

#define RAM_METRO_ADDRESS 0xC04EA600

const uint32_t kMetroSize = 9600; // 0.2 seconds
const uint32_t kMetroByteSize = kMetroSize * 3;

#define RAM_METRO_0A RAM_METRO_ADDRESS
#define RAM_METRO_0B RAM_METRO_ADDRESS + (1 * kMetroByteSize)
#define RAM_METRO_1A RAM_METRO_ADDRESS + (2 * kMetroByteSize)
#define RAM_METRO_1B RAM_METRO_ADDRESS + (3 * kMetroByteSize)
#define RAM_METRO_2A RAM_METRO_ADDRESS + (4 * kMetroByteSize)
#define RAM_METRO_2B RAM_METRO_ADDRESS + (5 * kMetroByteSize)
#define RAM_METRO_3A RAM_METRO_ADDRESS + (6 * kMetroByteSize)
#define RAM_METRO_3B RAM_METRO_ADDRESS + (7 * kMetroByteSize)
#define RAM_METRO_4A RAM_METRO_ADDRESS + (8 * kMetroByteSize)
#define RAM_METRO_4B RAM_METRO_ADDRESS + (9 * kMetroByteSize)

const uint32_t kRamMetronomeAddressLibrary[5][2] = {
    {RAM_METRO_0A, RAM_METRO_0B},
    {RAM_METRO_1A, RAM_METRO_1B},
    {RAM_METRO_2A, RAM_METRO_2B},
    {RAM_METRO_3A, RAM_METRO_3B},
    {RAM_METRO_4A, RAM_METRO_4B},
};

// 0xC0530B00 - 0xC0700000  free space

#define RAM_IMAGE_LOGO_ADDRESS 0xC0700000            // 600px * 100px
#define RAM_IMAGE_LOGO_PALETTE_ADDRESS 0xC0700000    //    128 bytes
#define RAM_IMAGE_LOGO_DATA_ADDRESS 0xC0700000 + 128 // 60.000 bytes

#define RAM_IMAGE_MENU_ADDRESS 0xC070EAE0            // 814px * 160px
#define RAM_IMAGE_MENU_PALETTE_ADDRESS 0xC070EAE0    //    128 bytes
#define RAM_IMAGE_MENU_DATA_ADDRESS 0xC070EAE0 + 128 // 65.120 bytes

#define RAM_IMAGE_KEY_ADDRESS 0xC072E8A0            // 119px * 244px
#define RAM_IMAGE_KEY_PALETTE_ADDRESS 0xC072E8A0    // 128 bytes
#define RAM_IMAGE_KEY_DATA_ADDRESS 0xC072E8A0 + 128 // 29.036 bytes

#define RAM_IMAGE_OSC_A_ADDRESS 0xC0735A8C            // 228px * 244px
#define RAM_IMAGE_OSC_A_PALETTE_ADDRESS 0xC0735A8C    // 128 bytes
#define RAM_IMAGE_OSC_A_DATA_ADDRESS 0xC0735A8C + 128 // 55.632 bytes

#define RAM_IMAGE_OSC_B_ADDRESS 0xC074345C            // 228px * 244px
#define RAM_IMAGE_OSC_B_PALETTE_ADDRESS 0xC074345C    // 128 bytes
#define RAM_IMAGE_OSC_B_DATA_ADDRESS 0xC074345C + 128 // 55.632 bytes

#define RAM_IMAGE_FILTER_ADDRESS 0xC0750E2C            // 119px * 244px
#define RAM_IMAGE_FILTER_PALETTE_ADDRESS 0xC0750E2C    // 128 bytes
#define RAM_IMAGE_FILTER_DATA_ADDRESS 0xC0750E2C + 128 // 29.036 bytes

#define RAM_IMAGE_ENV_ADDRESS 0xC0758018            // 119px * 244px
#define RAM_IMAGE_ENV_PALETTE_ADDRESS 0xC0758018    // 128 bytes
#define RAM_IMAGE_ENV_DATA_ADDRESS 0xC0758018 + 128 // 29.036 bytes

#define RAM_GRAPH_KEY_ADDRESS 0xC075F204
#define RAM_GRAPH_KEY_PALETTE_ADDRESS 0xC075F204    // 128 bytes
#define RAM_GRAPH_KEY_DATA_ADDRESS 0xC075F204 + 128 // 52.234 bytes

#define RAM_GRAPH_ARPEG_ADDRESS 0xC076BE8E
#define RAM_GRAPH_ARPEG_PALETTE_ADDRESS 0xC076BE8E    // 128 bytes
#define RAM_GRAPH_ARPEG_DATA_ADDRESS 0xC076BE8E + 128 // 104.468 bytes

#define RAM_GRAPH_ENVELOPE_ADDRESS 0xC0785722
#define RAM_GRAPH_ENVELOPE_PALETTE_ADDRESS 0xC0785722    // 128 bytes
#define RAM_GRAPH_ENVELOPE_DATA_ADDRESS 0xC0785722 + 128 // 16.072 bytes

#define RAM_GRAPH_FILTER_ADDRESS 0xC0789AA6
#define RAM_GRAPH_FILTER_PALETTE_ADDRESS 0xC0789AA6    // 128 bytes
#define RAM_GRAPH_FILTER_DATA_ADDRESS 0xC0789AA6 + 128 // 20.090 bytes

#define RAM_GRAPH_LFO_A_ADDRESS 0xC078E9A0
#define RAM_GRAPH_LFO_A_PALETTE_ADDRESS 0xC078E9A0    // 128 bytes
#define RAM_GRAPH_LFO_A_DATA_ADDRESS 0xC078E9A0 + 128 // 104.468 bytes

#define RAM_GRAPH_LFO_B_ADDRESS 0xC07A8234
#define RAM_GRAPH_LFO_B_PALETTE_ADDRESS 0xC07A8234    // 128 bytes
#define RAM_GRAPH_LFO_B_DATA_ADDRESS 0xC07A8234 + 128 // 104.468 bytes

// 0xC07C1AC8 - 0xC0800000  free space

////////////////////////////////////////////////////////////////////////////////
/* Sdram Constants -----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

#define SDRAM_START_ADDRESS ((uint32_t)0xC0000000)
#define SDRAM_END_ADDRESS ((uint32_t)0xC0FFFFF0)

#define SDRAM_MEMORY_WIDTH FMC_SDRAM_MEM_BUS_WIDTH_16
#define SDCLOCK_PERIOD FMC_SDRAM_CLOCK_PERIOD_2

#define SDRAM_TIMEOUT ((uint32_t)0xFFFF)
#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2 ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4 ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8 ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3 ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200)

////////////////////////////////////////////////////////////////////////////////
/* Sd Constants --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

struct Sd {
    FATFS fs;
    DIR dir;
    FIL file;
    FILINFO fileInfo;
    FRESULT fresult;

    UINT byteswritten;
    UINT bytesread;

    bool detect = false;
    bool ready = false;
    bool getLibrary = false;
    char label[32] = "";
    DWORD serial = 0;
    DWORD serialTemp = 0;

    uint16_t totalSpace = 0;
    uint16_t freeSpace = 0;
    uint16_t usedSpace = 0;

    Sd() {}
    ~Sd() {}
};

/*----------------------------------------------------------------------------*/

typedef enum {
    SD_OK = 0x00,
    SD_ERROR = 0x01,
    SD_ERROR_DETECT = 0x02,
    SD_ERROR_MOUNT = 0x03,
    SD_ERROR_SERIAL = 0x04,
    SD_ERROR_SYSTEMFOLDER = 0x05,
    SD_ERROR_WAVETABLEFOLDER = 0x06,
    SD_ERROR_FILEFOLDER = 0x07,
    SD_ERROR_SYNTHKITFOLDER = 0x08,
    SD_ERROR_SOUNDFOLDER = 0x09,
    SD_ERROR_IMAGEFOLDER = 0x0A,
    SD_ERROR_FIRMWAREFOLDER = 0x0B,
    SD_ERROR_SYSTEMFILE = 0x0C,
} SdResult;

/*----------------------------------------------------------------------------*/

const char kSdAlertTextInsert[] = "INSERT SDCARD";
const char kSdAlertTextFormat[] = "SDCARD FORMAT ERROR";
const char kSdAlertTextSerial[] = "SDCARD SERIAL ERROR";
const char kSdAlertTextSystemFolder[] = "SYSTEM FOLDER ERROR";
const char kSdAlertTextWavetableFolder[] = "WTABLE FOLDER ERROR";
const char kSdAlertTextFileFolder[] = "FILE FOLDER ERROR";
const char kSdAlertTextSynthkitFolder[] = "SYNTHKIT FOLDER ERROR";
const char kSdAlertTextSoundFolder[] = "SOUND FOLDER ERROR";
const char kSdAlertTextImageFolder[] = "IMAGE FOLDER ERROR";
const char kSdAlertTextFirmwareFolder[] = "FWARE FOLDER ERROR";
const char kSdAlertTextSystemFile[] = "SYSTEM FILE ERROR";
const char kSdAlertTextAnalyze[] = "ANALYZING SDCARD";

////////////////////////////////////////////////////////////////////////////////
/* Lcd Constants -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

#define LCD_DATA_GPIO_Port GPIOB

#define LCD_CS_HIGH LCD_CS_GPIO_Port->BSRR = LCD_CS_Pin
#define LCD_CS_LOW LCD_CS_GPIO_Port->BSRR = LCD_CS_Pin << 16U
#define LCD_RS_HIGH LCD_RS_GPIO_Port->BSRR = LCD_RS_Pin
#define LCD_RS_LOW LCD_RS_GPIO_Port->BSRR = LCD_RS_Pin << 16U
#define LCD_WR_HIGH LCD_WR_GPIO_Port->BSRR = LCD_WR_Pin
#define LCD_WR_LOW LCD_WR_GPIO_Port->BSRR = LCD_WR_Pin << 16U
#define LCD_RD_HIGH LCD_RD_GPIO_Port->BSRR = LCD_RD_Pin
#define LCD_RD_LOW LCD_RD_GPIO_Port->BSRR = LCD_RD_Pin << 16U
#define LCD_RESET_HIGH LCD_RESET_GPIO_Port->BSRR = LCD_RESET_Pin
#define LCD_RESET_LOW LCD_RESET_GPIO_Port->BSRR = LCD_RESET_Pin << 16U
#define LCD_BL_HIGH LCD_BL_GPIO_Port->BSRR = LCD_BL_Pin
#define LCD_BL_LOW LCD_BL_GPIO_Port->BSRR = LCD_BL_Pin << 16U

/*----------------------------------------------------------------------------*/

#define MADCTL_MY 0x80  // Bottom to top
#define MADCTL_MX 0x40  // Right to left
#define MADCTL_MV 0x20  // Row/Column exchange
#define MADCTL_ML 0x10  // LCD refresh Bottom to top
#define MADCTL_RGB 0x00 // Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 // Blue-Green-Red pixel order
#define MADCTL_MH 0x04  // LCD refresh right to left

/*----------------------------------------------------------------------------*/

const uint16_t kLCD_WIDTH = 480;
const uint16_t kLCD_HEIGHT = 854;

// R: 5 bit, G: 6 bit, B: 5bit
// https://lvgl.io/tools/imageconverter
// http://www.rinkydinkelectronics.com/calc_rgb565.php

typedef enum {
    RGB16 = 0x00,
    RGB24 = 0x01,
} RGBMode;

typedef uint16_t RGB16Color;
typedef uint32_t RGB24Color;

struct RGB24RawColor {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
};

/*----------------------------------------------------------------------------*/

const RGB16Color WHITE = 0xFFFF;
const RGB16Color BLACK = 0x0000;
const RGB16Color GRAY = 0x8C51;
const RGB16Color YELLOW = 0xFFE0;
const RGB16Color GREEN = 0x07E0;
const RGB16Color CYAN = 0x07FF;
const RGB16Color MAGENTA = 0xF81F;
const RGB16Color LBLUE = 0x03FF;
const RGB16Color BLUE = 0x001F;
const RGB16Color RED = 0xF800;
const RGB16Color ORANGE = 0xFD20;
const RGB16Color PINK = 0xF81F;

const RGB16Color GRAY_05 = 0x0861;
const RGB16Color GRAY_10 = 0x18C3;
const RGB16Color GRAY_15 = 0x2124;
const RGB16Color GRAY_20 = 0x3186;
const RGB16Color GRAY_25 = 0x4208;
const RGB16Color GRAY_30 = 0x4A69;
const RGB16Color GRAY_35 = 0x5ACB;
const RGB16Color GRAY_40 = 0x632C;
const RGB16Color GRAY_45 = 0x738E;
const RGB16Color GRAY_50 = 0x8410;
const RGB16Color GRAY_55 = 0x8C71;
const RGB16Color GRAY_60 = 0x9CD3;
const RGB16Color GRAY_65 = 0xA534;
const RGB16Color GRAY_70 = 0xB596;
const RGB16Color GRAY_75 = 0xBDF7;
const RGB16Color GRAY_80 = 0xCE79;
const RGB16Color GRAY_85 = 0xDEDB;
const RGB16Color GRAY_90 = 0xE73C;
const RGB16Color GRAY_95 = 0xF79E;

/*----------------------------------------------------------------------------*/

const RGB16Color kLayerColorPalette[] = {
    0xFFE0,
    0x7FE0,
    0x07E0,
    0x07EF,
    0x07FF,
    0x03FF,
    0x001F,
    0x781F,
    0xF81F,
    0xF80F};

/*----------------------------------------------------------------------------*/

typedef enum {
    PORTRAIT_0 = 0x00,
    PORTRAIT_1 = 0x02,
    LANDSCAPE_0 = 0x01,
    LANDSCAPE_1 = 0x03
} LcdRotation;

/*----------------------------------------------------------------------------*/

typedef enum {
    FONT_05x07 = 0x00,
    FONT_07x09 = 0x01,
    FONT_10x14 = 0x02,
    FONT_14x18 = 0x03
} LcdFont;

/*----------------------------------------------------------------------------*/

typedef enum {
    LEFT = 0x00,
    RIGHT = 0x01,
    CENTER = 0x02
} LcdAlign;

/*----------------------------------------------------------------------------*/

struct BmpData {
    uint16_t header;
    uint32_t fileSize;
    uint32_t width;
    uint32_t height;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t paletteSize;
    uint32_t pixelSize;
    uint32_t paletteStartByte;
    uint32_t pixelStartByte;
    uint32_t widthOffset;
};

const uint16_t kBmpHeaderByteSize = 50;

/*----------------------------------------------------------------------------*/

const uint16_t kSongWidth = 674;
const uint16_t kSongHeight = 11;
const uint16_t kSongX = 149;
const uint16_t kSongY = 129;
const uint16_t kSongOffsetY = 4;
const uint16_t kNoteHeight = 3;

const uint16_t kPlayWidth = 674;
const uint16_t kPlayHeight = 1;
const uint16_t kPlayX = 149;
const uint16_t kPlayY = 113;

const uint16_t kOctaveX[] = {31, 51, 71, 91, 111};
const uint16_t kOctaveY = 161;

const RGB16Color kPlayColor0 = BLACK;
const RGB16Color kPlayColor1 = WHITE;

const uint16_t kInfoStartX = 30;
const uint16_t kInfoStartY = 195;

const uint16_t kImageLogoWidth = 450;
const uint16_t kImageLogoHeight = 80;
const uint16_t kImageLogoX = 202;
const uint16_t kImageLogoY = 200;
const uint16_t kImageLogoPalette = 64;

const uint16_t kImageMenuWidth = 814;
const uint16_t kImageMenuHeight = 160;
const uint16_t kImageMenuX = 20;
const uint16_t kImageMenuY = 20;
const uint16_t kImageMenuPalette = 64;

const uint16_t kImageKeyWidth = 119;
const uint16_t kImageKeyHeight = 244;
const uint16_t kImageKeyX = 30;
const uint16_t kImageKeyY = kInfoStartY;
const uint16_t kImageKeyPalette = 64;

const uint16_t kImageOscAWidth = 228;
const uint16_t kImageOscAHeight = 244;
const uint16_t kImageOscAX = 149;
const uint16_t kImageOscAY = kInfoStartY;
const uint16_t kImageOscAPalette = 64;

const uint16_t kImageOscBWidth = 228;
const uint16_t kImageOscBHeight = 244;
const uint16_t kImageOscBX = 377;
const uint16_t kImageOscBY = kInfoStartY;
const uint16_t kImageOscBPalette = 64;

const uint16_t kImageFilterWidth = 119;
const uint16_t kImageFilterHeight = 244;
const uint16_t kImageFilterX = 605;
const uint16_t kImageFilterY = kInfoStartY;
const uint16_t kImageFilterPalette = 64;

const uint16_t kImageEnvWidth = 119;
const uint16_t kImageEnvHeight = 244;
const uint16_t kImageEnvX = 724;
const uint16_t kImageEnvY = kInfoStartY;
const uint16_t kImageEnvPalette = 64;

const uint16_t kImageBarWidth = 814;
const uint16_t kImageBarHeight = 13;
const uint16_t kImageBarX = 20;
const uint16_t kImageBarY = 457;
const uint16_t kImageBarPalette = 64;

const uint16_t kLimitAlertX = 103;
const uint16_t kLimitAlertY = 109;

/*----------------------------------------------------------------------------*/

const uint8_t kIconMenuWidth = 20;
const uint8_t kIconMenuHeight = 20;
const uint16_t kIconMenuX = 92;
const uint16_t kIconMenuY = 50;

const uint8_t kIconSelectWidth = 11;
const uint8_t kIconSelectHeight = 11;
const uint16_t kIconSelect4X[] = {182 + 12, 345 + 12, 508 + 12, 671 + 12};
const uint16_t kIconSelect8X[] = {182 + 12, 263 + 12, 345 + 12, 426 + 12, 508 + 12, 589 + 12, 671 + 12, 752 + 12};
const uint16_t kIconSelectY = 77;

const uint8_t kIconAlertWidth = 19;
const uint8_t kIconAlertHeight = 19;
const uint16_t kIconAlertX[] = {315, 520};
const uint16_t kIconAlertY = 139;

const uint8_t kIconPlayWidth = 9;
const uint8_t kIconPlayHeight = 9;
const uint16_t kIconPlayY = 109;
const uint16_t kIconResetX = 30;
const uint16_t kIconPlayX = 48;
const uint16_t kIconStopX = 65;
const uint16_t kIconRecordX = 81;
const uint16_t kIconPalette = 64;

/*----------------------------------------------------------------------------*/

const uint8_t kButtonKeyWidth = 99;
const uint8_t kButtonKeyHeight = 13;
const uint16_t kButtonKeyX = kInfoStartX;
const uint16_t kButtonKeyY = kInfoStartY;
const uint16_t kButtonKeyPalette = 64;

const uint8_t kButtonArpegWidth = 99;
const uint8_t kButtonArpegHeight = 13;
const uint16_t kButtonArpegX = kInfoStartX;
const uint16_t kButtonArpegY = kInfoStartY + 231;
const uint16_t kButtonArpegPalette = 64;

const uint8_t kButtonOscAWidth = 208;
const uint8_t kButtonOscAHeight = 13;
const uint16_t kButtonOscAX = kInfoStartX + 119;
const uint16_t kButtonOscAY = kInfoStartY;
const uint16_t kButtonOscAPalette = 64;

const uint8_t kButtonOscBWidth = 208;
const uint8_t kButtonOscBHeight = 13;
const uint16_t kButtonOscBX = kInfoStartX + 347;
const uint16_t kButtonOscBY = kInfoStartY;
const uint16_t kButtonOscBPalette = 64;

const uint8_t kButtonFilterWidth = 99;
const uint8_t kButtonFilterHeight = 13;
const uint16_t kButtonFilterX[2] = {kInfoStartX + 575, kInfoStartX + 575};
const uint16_t kButtonFilterY[2] = {kInfoStartY, kInfoStartY + 231};
const uint16_t kButtonFilterPalette = 64;

const uint8_t kButtonEnvelopeWidth = 99;
const uint8_t kButtonEnvelopeHeight = 13;
const uint16_t kButtonEnvelopeX = kInfoStartX + 694;
const uint16_t kButtonEnvelopeY = kInfoStartY;
const uint16_t kButtonEnvelopePalette = 64;

const uint8_t kButtonLfoWidth = 99;
const uint8_t kButtonLfoHeight = 13;
const uint16_t kButtonOscALfoX[2] = {kInfoStartX + 119, kInfoStartX + 228};
const uint16_t kButtonOscBLfoX[2] = {kInfoStartX + 347, kInfoStartX + 456};
const uint16_t kButtonLfoY = kInfoStartY + 231;
const uint16_t kButtonLfoPalette = 64;

const uint8_t kButtonSongWidth = 99;
const uint8_t kButtonSongHeight = 13;
const uint16_t kButtonSongX = 30;
const uint16_t kButtonSongY = 135;
const uint16_t kButtonSongPalette = 64;

/*----------------------------------------------------------------------------*/

const uint16_t kInfoGraphWidth = 82;
const uint16_t kInfoGraphHeight = 49;

const uint16_t kInfoGraphPalette = 64;

/*----------------------------------------------------------------------------*/

const uint16_t kBeatGraphStartX = 449;
const uint16_t kBeatGraphEndX = 730;
const uint16_t kBeatGraphWidth = 282;
const uint16_t kBeatGraphStartY = 33;
const uint16_t kBeatGraphEndY = 87;
const uint8_t kBeatGraphHeight = 55;
const uint16_t kBeatGraphStartTimeX = 370;
const uint16_t kBeatGraphEndTimeX = 755;
const uint16_t kBeatGraphTimeY = 80;

/*----------------------------------------------------------------------------*/

const uint8_t kMinMenu4Tab = 0;
const uint8_t kMaxMenu4Tab = 3;

const uint8_t kMinMenu8Tab = 0;
const uint8_t kMaxMenu8Tab = 7;

const uint16_t kMenuIconX = 83;
const uint16_t kMenuIconY = 53;

const uint16_t kMenuLine4X[] = {182, 345, 508, 671};
const uint16_t kMenuLine8X[] = {182, 263, 345, 426, 508, 589, 671, 752};
const uint16_t kMenuLineY = 30;
const uint16_t kMenuLineHeight = 60;

const uint16_t kMenuHeader4X[] = {182 + 12, 345 + 12, 508 + 12, 671 + 12};
const uint16_t kMenuHeader8X[] = {182 + 12, 263 + 12, 345 + 12, 426 + 12, 508 + 12, 589 + 12, 671 + 12, 752 + 12};

const uint16_t kMenuHeaderY = 33;

const uint16_t kMenuData4X[] = {345 - 11, 508 - 11, 671 - 11, 834 - 11};
const uint16_t kMenuData8X[] = {263 - 11, 345 - 11, 426 - 11, 508 - 11, 589 - 11, 671 - 11, 752 - 11, 834 - 11};
const uint16_t kMenuDataY = 78;

const uint16_t kMenuNumberX = 182 - 28;
const uint16_t kMenuNumberY = 33;

const uint8_t kMenuHeaderTextSize = 10;
const uint8_t kMenuDataTextSize = 10;
const uint8_t kMenuNumberTextSize = 2;

const uint8_t kMenuHeaderShortTextSize = 4;
const uint8_t kMenuDataShortTextSize = 4;

const uint8_t kMenuSignTextSize = 3;

const uint16_t kMenuBoxX[] = {345 - 20, 508 - 20, 671 - 20, 834 - 20};
const uint16_t kMenuBoxY = 33;
const uint8_t kMenuBoxWidth = 9;
const uint8_t kMenuBoxHeight = 9;

const uint16_t kMainMenuX[] = {224, 305, 387, 468, 550, 631, 713, 794};
const uint16_t kMainMenuHeaderY = 39;
const uint16_t kMainMenuDataY = 72;

const uint16_t kInfoPhaseX0[2][4] = {{kInfoStartX + 128, kInfoStartX + 128 + 20, kInfoStartX + 128 + 40, kInfoStartX + 128 + 60}, {kInfoStartX + 356, kInfoStartX + 356 + 20, kInfoStartX + 356 + 40, kInfoStartX + 356 + 60}};
const uint16_t kInfoPhaseX1[2][4] = {{kInfoStartX + 237, kInfoStartX + 237 + 20, kInfoStartX + 237 + 40, kInfoStartX + 237 + 60}, {kInfoStartX + 465, kInfoStartX + 465 + 20, kInfoStartX + 465 + 40, kInfoStartX + 465 + 60}};
const uint16_t kInfoPhaseY0 = kInfoStartY + 40;
const uint16_t kInfoPhaseY1 = kInfoStartY + 92;

const uint16_t kInfoKeyGraphX = kInfoStartX + 9;
const uint16_t kInfoKeyGraphY = kInfoStartY + 43;

const uint16_t kInfoArpegGraphX = kInfoStartX + 9;
const uint16_t kInfoArpegGraphY = kInfoStartY + 152;

const uint16_t kInfoOscGraphX[2][2] = {{kInfoStartX + 128, kInfoStartX + 237}, {kInfoStartX + 356, kInfoStartX + 465}};
const uint16_t kInfoOscGraphY = kInfoStartY + 43;

const uint16_t kInfoOscLfoGraphX[2][2] = {{kInfoStartX + 128, kInfoStartX + 237}, {kInfoStartX + 356, kInfoStartX + 465}};
const uint16_t kInfoOscLfoGraphY = kInfoStartY + 152;

const uint16_t kInfoFilterGraphX[2] = {kInfoStartX + 584, kInfoStartX + 584};
const uint16_t kInfoFilterGraphY[2] = {kInfoStartY + 43, kInfoStartY + 152};

const uint16_t kInfoEnvelopeGraphX = kInfoStartX + 703;
const uint16_t kInfoEnvelopeGraphY = kInfoStartY + 43;

/*----------------------------------------------------------------------------*/

const char kHeaderActive[] = "ACTIVE    ";
const char kHeaderArpeg[] = "ARPEG     ";
const char kHeaderAnalyze[] = "ANALYZE   ";
const char kHeaderAttack[] = "ATTACK    ";
const char kHeaderBar[] = "BAR       ";
const char kHeaderBlank[] = "          ";
const char kHeaderChord[] = "CHORD     ";
const char kHeaderClear[] = "CLEAR     ";
const char kHeaderCurve[] = "CURVE     ";
const char kHeaderCutoff[] = "CUTOFF    ";
const char kHeaderDecay[] = "DECAY     ";
const char kHeaderDepth[] = "DEPTH     ";
const char kHeaderEffectA[] = "EFFECT 1  ";
const char kHeaderEffectB[] = "EFFECT 2  ";
const char kHeaderEnd[] = "END       ";
const char kHeaderEnvelope[] = "ENVELOPE  ";
const char kHeaderEq[] = "PARAM EQ  ";
const char kHeaderFeedback[] = "FEEDBACK  ";
const char kHeaderFill[] = "FILL      ";
const char kHeaderFilterA[] = "FILTER 1  ";
const char kHeaderFilterB[] = "FILTER 2  ";
const char kHeaderFrequency[] = "FREQ      ";
const char kHeaderFlip[] = "FLIP      ";
const char kHeaderInput[] = "INPUT     ";
const char kHeaderInst[] = "INST      ";
const char kHeaderKey[] = "KEY       ";
const char kHeaderLength[] = "LENGTH    ";
const char kHeaderLevel[] = "LEVEL     ";
const char kHeaderLfo[] = "LFO       ";
const char kHeaderLfoA[] = "LFO 1     ";
const char kHeaderLfoB[] = "LFO 2     ";
const char kHeaderLimiter[] = "LIMITER   ";
const char kHeaderLoad[] = "LOAD      ";
const char kHeaderMeasure[] = "MEASURE   ";
const char kHeaderMetronome[] = "METRONOME ";
const char kHeaderMix[] = "MIX       ";
const char kHeaderNew[] = "NEW       ";
const char kHeaderNote[] = "NOTE      ";
const char kHeaderOrder[] = "ORDER     ";
const char kHeaderOctave[] = "OCTAVE    ";
const char kHeaderOscA[] = "OSC A     ";
const char kHeaderOscB[] = "OSC B     ";
const char kHeaderOsc[] = "OSC       ";
const char kHeaderPan[] = "PAN       ";
const char kHeaderPattern[] = "PATTERN   ";
const char kHeaderPhase[] = "PHASE     ";
const char kHeaderPrecount[] = "PRECOUNT  ";
const char kHeaderRate[] = "RATE      ";
const char kHeaderRelease[] = "RELEASE   ";
const char kHeaderReverb[] = "REVERB    ";
const char kHeaderQuantize[] = "QUANTIZE  ";
const char kHeaderSample[] = "SAMPLE    ";
const char kHeaderSave[] = "SAVE      ";
const char kHeaderScale[] = "SCALE     ";
const char kHeaderSize[] = "SIZE      ";
const char kHeaderStart[] = "START     ";
const char kHeaderSurround[] = "SURROUND  ";
const char kHeaderSustain[] = "SUSTAIN   ";
const char kHeaderTarget[] = "TARGET    ";
const char kHeaderTempo[] = "TEMPO     ";
const char kHeaderTime[] = "TIME      ";
const char kHeaderTune[] = "TUNE      ";
const char kHeaderType[] = "TYPE      ";
const char kHeaderVolume[] = "VOLUME    ";
const char kHeaderWavetable[] = "WTABLE    ";
const char kHeaderX[] = "X         ";
const char kHeaderY[] = "Y         ";

const char kHeaderLow[] = "LOW       ";
const char kHeaderMid[] = "MID       ";
const char kHeaderHigh[] = "HIGH      ";

const char kHeaderEqFreq[6][3] = {"LS", "P ", "P ", "P ", "P ", "HS"};

const char kDataBlank[] = "          ";
const char kDataDashL[] = "--------  ";
const char kDataDashR[] = "  --------";
const char kDataDashShortL[] = "--- ";
const char kDataDashShortR[] = " ---";
const char kDataStart[] = "S";
const char kDataEnd[] = "E";
const char kDataSelect[] = "    SELECT";
const char kDataOn[] = "        ON";
const char kDataOff[] = "       OFF";

const char kDataShortLOn[] = "ON  ";
const char kDataShortLOff[] = "OFF ";
const char kDataShortROn[] = "  ON";
const char kDataShortROff[] = " OFF";

const char kDataDot[] = ".";
const char kDataPlus[] = "+";
const char kDataTimeBlank[] = "--.--.--";

////////////////////////////////////////////////////////////////////////////////
/* Keyboard Constants --------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    PASSIVE = 0x00,
    PREWAIT = 0x01,
    READ = 0x02,
    POSTWAIT = 0x03
} ButtonState;

/*----------------------------------------------------------------------------*/

struct Keyboard {
    ButtonState leftButtonState = PASSIVE;
    ButtonState rightButtonState = PASSIVE;
    ButtonState beatButtonState = PASSIVE;
    uint16_t leftButtonCounter = 0;
    uint16_t rightButtonCounter = 0;
    uint16_t beatButtonCounter = 0;
    uint16_t longButtonCounter = 0;
    int16_t leftButtonTemp = 0;
    int16_t rightButtonTemp = 0;
    int16_t beatButtonTemp = 0;
    int16_t leftButton = -1;
    int16_t rightButton = -1;
    int16_t beatButton = -1;
    bool leftButtonPress = false;
    bool rightButtonPress = false;
    bool beatButtonPress = false;
    bool bankButtonPress = false;
};

const uint8_t kLongButtonCountLow = 5;
const uint8_t kLongButtonCountHigh = 10;

#define CT0_SCL_HIGH CT0_SCL_GPIO_Port->BSRR = CT0_SCL_Pin
#define CT0_SCL_LOW CT0_SCL_GPIO_Port->BSRR = CT0_SCL_Pin << 16U
#define CT1_SCL_HIGH CT1_SCL_GPIO_Port->BSRR = CT1_SCL_Pin
#define CT1_SCL_LOW CT1_SCL_GPIO_Port->BSRR = CT1_SCL_Pin << 16U
#define CT2_SCL_HIGH CT2_SCL_GPIO_Port->BSRR = CT2_SCL_Pin
#define CT2_SCL_LOW CT2_SCL_GPIO_Port->BSRR = CT2_SCL_Pin << 16U

#define CT0_SDO_READ CT0_SDO_GPIO_Port->IDR &CT0_SDO_Pin
#define CT1_SDO_READ CT1_SDO_GPIO_Port->IDR &CT1_SDO_Pin
#define CT2_SDO_READ CT2_SDO_GPIO_Port->IDR &CT2_SDO_Pin

/*----------------------------------------------------------------------------*/

typedef enum {
    KEY_RESET = 0x06 + 1,
    KEY_PLAYSTOP = 0x07 + 1,
    KEY_REC = 0x01 + 1,
    KEY_UP = 0x00 + 1,
    KEY_DOWN = 0x08 + 1,
    KEY_LEFT = 0x04 + 1,
    KEY_RIGHT = 0x03 + 1,
    KEY_CENTER = 0x0E + 1,
    KEY_ADD = 0x05 + 1,
    KEY_ERASE = 0x02 + 1,
    KEY_COPY = 0x0B + 1,
    KEY_PASTE = 0x0C + 1,
    KEY_POWER = 0x0A + 1,
    KEY_OCTAVE_DOWN = 0x09 + 1,
    KEY_OCTAVE_UP = 0x0D + 1,
    LEFT_RELEASE = 0x00
} KeyLeft;

typedef enum {
    KEY_FILE = 0x06 + 1,
    KEY_SYNTHKIT = 0x07 + 1,
    KEY_SYSTEM = 0x01 + 1,
    KEY_RHYTHM = 0x05 + 1,
    KEY_METRONOME = 0x00 + 1,
    KEY_EQ = 0x02 + 1,
    KEY_OSC_A = 0x04 + 1,
    KEY_OSC_B = 0x0E + 1,
    KEY_FILTER = 0x03 + 1,
    KEY_ENVELOPE = 0x0B + 1,
    KEY_EFFECT = 0x08 + 1,
    KEY_REVERB = 0x0C + 1,
    KEY_MODE_KEY = 0x0A + 1,
    KEY_MODE_SONG = 0x09 + 1,
    KEY_BANK = 0x0D + 1,
    RIGHT_RELEASE = 0x00
} KeyRight;

typedef enum {
    KEY_00 = 0x05 + 1,
    KEY_01 = 0x03 + 1,
    KEY_02 = 0x06 + 1,
    KEY_03 = 0x02 + 1,
    KEY_04 = 0x07 + 1,
    KEY_05 = 0x08 + 1,
    KEY_06 = 0x01 + 1,
    KEY_07 = 0x09 + 1,
    KEY_08 = 0x00 + 1,
    KEY_09 = 0x0A + 1,
    KEY_10 = 0x04 + 1,
    KEY_11 = 0x0B + 1,
    BEAT_RELEASE = 0x00
} KeyBeat;

////////////////////////////////////////////////////////////////////////////////
/* Controller Constants ------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    INIT_MENU = 0x00,
    MAIN_MENU = 0x01,
    FILE_MENU = 0x02,
    SYNTHKIT_MENU = 0x03,
    SYSTEM_MENU = 0x04,
    RHYTHM_MENU = 0x05,
    METRO_MENU = 0x06,
    EQ_MENU = 0x07,
    OSC_A0_MENU = 0x08,
    OSC_A1_MENU = 0x09,
    OSC_A2_MENU = 0x0A,
    OSC_A3_MENU = 0x0B,
    OSC_B0_MENU = 0x0C,
    OSC_B1_MENU = 0x0D,
    OSC_B2_MENU = 0x0E,
    OSC_B3_MENU = 0x0F,
    FILTER_0_MENU = 0x10,
    FILTER_1_MENU = 0x11,
    ENVELOPE_MENU = 0x12,
    EFFECT_0_MENU = 0x13,
    EFFECT_1_MENU = 0x14,
    REVERB_MENU = 0x15,
    KEY_MENU = 0x16,
    SONG_MENU = 0x18,
} Menu;

typedef enum {
    TRIG_MEASURE = 0x00,
    TRIG_BAR = 0x01,
    TRIG_SONG = 0x02
} TriggerMode;

typedef enum {
    ALERT_MEASUREUP = 0x00,
    ALERT_MEASUREDOWN = 0x01,
    ALERT_BARUP = 0x02,
    ALERT_BARDOWN = 0x03,
    ALERT_QUANTIZEUP = 0x04,
    ALERT_QUANTIZEDOWN = 0x05,
    ALERT_NEWFILE = 0x06,
    ALERT_LOADFILE = 0x07,
    ALERT_SAVEFILE = 0x08,
    ALERT_CLEARFILE = 0x09,
    ALERT_OVERWRITEFILE = 0x0A,
    ALERT_NEWSYNTHKIT = 0x0B,
    ALERT_LOADSYNTHKIT = 0x0C,
    ALERT_SAVESYNTHKIT = 0x0D,
    ALERT_CLEARSYNTHKIT = 0x0E,
    ALERT_OVERWRITESYNTHKIT = 0x0F,
    ALERT_MISSINGWAVETABLE = 0x10,
    ALERT_MISSINGWAVETABLES = 0x11,
    ALERT_LOADSUCCESS = 0x12,
    ALERT_LOADERROR = 0x13,
    ALERT_SAVESUCCESS = 0x14,
    ALERT_SAVEERROR = 0x15,
    ALERT_CLEARSUCCESS = 0x16,
    ALERT_CLEARERROR = 0x17,
    ALERT_SHUTDOWN = 0x18,
    ALERT_OFF = 0x20
} AlertType;

const char kAlertTextResetPlay[] = "RESET PLAY?";

const char kAlertTextNewFile[] = "NEW FILE?";
const char kAlertTextLoadFile[] = "LOAD FILE?";
const char kAlertTextSaveFile[] = "SAVE FILE?";
const char kAlertTextClearFile[] = "CLEAR FILE?";
const char kAlertTextOverwriteFile[] = "OVERWRITE FILE?";

const char kAlertTextNewSynthkit[] = "NEW SYNTHKIT?";
const char kAlertTextLoadSynthkit[] = "LOAD SYNTHKIT?";
const char kAlertTextSaveSynthkit[] = "SAVE SYNTHKIT?";
const char kAlertTextClearSynthkit[] = "CLEAR SYNTHKIT?";
const char kAlertTextOverwriteSynthkit[] = "OVERWRITE SYNTHKIT?";
const char kAlertTextMissingWavetable[] = "MISSING WTABLE";
const char kAlertTextMissingWavetables[] = "MISSING WTABLES";

const char kAlertTextLoadSuccess[] = "LOAD -> SUCCESS";
const char kAlertTextLoadError[] = "LOAD -> ERROR";
const char kAlertTextSaveSuccess[] = "SAVE -> SUCCESS";
const char kAlertTextSaveError[] = "SAVE -> ERROR";
const char kAlertTextClearSuccess[] = "CLEAR -> SUCCESS";
const char kAlertTextClearError[] = "CLEAR -> ERROR";

struct Icon {
    bool flag = false;
    bool mode = false;
};

const uint16_t kWavetableLibraryMaxSize = 10000;

const uint8_t kFileNameSize = 32;
const uint8_t kFileNameArraySize = 35;

/*----------------------------------------------------------------------------*/

struct NumberData {
    char nameShortL[5];
    char nameShortR[5];
    char nameLongR[11];
};

const struct NumberData kNumberDataLibrary[] = {
    {"00  ", "  00", "        00"},
    {"01  ", "  01", "        01"},
    {"02  ", "  02", "        02"},
    {"03  ", "  03", "        03"},
    {"04  ", "  04", "        04"},
    {"05  ", "  05", "        05"},
    {"06  ", "  06", "        06"},
    {"07  ", "  07", "        07"},
    {"08  ", "  08", "        08"},
    {"09  ", "  09", "        09"},
    {"10  ", "  10", "        10"}};

/*----------------------------------------------------------------------------*/

struct FloatData {
    char nameInt[11];
    char nameFloat[11];
    float stdMultiplier;
    float pow2Multiplier;
    float pow4Multiplier;
};

const struct FloatData kFloatDataLibrary[] = {
    {"        00", "      0.00", 0.000000, 0.000000, 0.000000},
    {"        05", "      0.05", 0.050000, 0.002500, 0.000006},
    {"        10", "      0.10", 0.100000, 0.010000, 0.000100},
    {"        15", "      0.15", 0.150000, 0.022500, 0.000506},
    {"        20", "      0.20", 0.200000, 0.040000, 0.001600},
    {"        25", "      0.25", 0.250000, 0.062500, 0.003906},
    {"        30", "      0.30", 0.300000, 0.090000, 0.008100},
    {"        35", "      0.35", 0.350000, 0.122500, 0.015006},
    {"        40", "      0.40", 0.400000, 0.160000, 0.025600},
    {"        45", "      0.45", 0.450000, 0.202500, 0.041006},
    {"        50", "      0.50", 0.500000, 0.250000, 0.062500},
    {"        55", "      0.55", 0.550000, 0.302500, 0.091506},
    {"        60", "      0.60", 0.600000, 0.360000, 0.129600},
    {"        65", "      0.65", 0.650000, 0.422500, 0.178506},
    {"        70", "      0.70", 0.700000, 0.490000, 0.240100},
    {"        75", "      0.75", 0.750000, 0.562500, 0.316406},
    {"        80", "      0.80", 0.800000, 0.640000, 0.409600},
    {"        85", "      0.85", 0.850000, 0.722500, 0.522006},
    {"        90", "      0.90", 0.900000, 0.810000, 0.656100},
    {"        95", "      0.95", 0.950000, 0.902500, 0.814506},
    {"       100", "      1.00", 1.000000, 1.000000, 1.000000}};

/*
dBvalue = 20.0 * log10 (linear);
// dB = 20*log(linear)

linear = pow (10.0, (0.05 * dBvalue));
// linear = 10^(dB/20)

000     -51.5dB     inf             0x7F        127
005     -26.0dB     -26.02dB        0x4C        76
010     -20.0dB     -20.00dB        0x40        64
015     -16.5dB     -16.48dB        0x39        57
020     -14.0dB     -13.98dB        0x34        52
025     -12.0dB     -12.04dB        0x30        48
030     -10.5dB     -10.46dB        0x2D        45
035     -9.0dB      -9.12dB         0x2A        42
040     -8.0dB      -7.96dB         0x28        40
045     -7.0dB      -6.94dB         0x26        38
050     -6.0dB      -6.02dB         0x24        36
055     -5.0dB      -5.19dB         0x22        34
060     -4.0dB      -4.44dB         0x20        32
065     -3.5dB      -3.74dB         0x1F        31
070     -3.0dB      -3.10dB         0x1E        30
075     -2.5dB      -2.50dB         0x1D        29
080     -2.0dB      -1.94dB         0x1C        28
085     -1.5dB      -1.41dB         0x1B        27
090     -1.0dB      -0.92dB         0x1A        26
095     -0.5dB      -0.45dB         0x19        25
100     0dB         0dB             0x18        24
*/

const uint8_t kHeadPhoneDecibelData[] = {
    0x7F,
    0x4C,
    0x40,
    0x39,
    0x34,
    0x30,
    0x2D,
    0x2A,
    0x28,
    0x26,
    0x24,
    0x22,
    0x20,
    0x1F,
    0x1E,
    0x1D,
    0x1C,
    0x1B,
    0x1A,
    0x19,
    0x18};

////////////////////////////////////////////////////////////////////////////////
/* Wavetable Constants -------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const char kWavetableRef[] = "RW_SYNTHGIRL_WAVETABLE   ";

////////////////////////////////////////////////////////////////////////////////
/* File Constants ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const char kFileRef[] = "RW_SYNTHGIRL_FILE        ";
const char kFileStart[] = "System/File/File_";
const char kFileEnd[] = ".rws";

const uint16_t kFileByteSize = 4788;
const uint8_t kFileLibrarySize = 100;

const uint8_t kMinFile = 0;
const uint8_t kMaxFile = 99;

////////////////////////////////////////////////////////////////////////////////
/* Synthkit Constants --------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const char kSynthkitRef[] = "RW_SYNTHGIRL_SYNTHKIT    ";
const char kSynthkitStart[] = "System/Synthkit/Synthkit_";
const char kSynthkitEnd[] = ".rws";

const uint16_t kSynthkitByteSize = 300;
const uint8_t kSynthkitLibrarySize = 100;

const uint8_t kMinSynthkit = 0;
const uint8_t kMaxSynthkit = 99;

////////////////////////////////////////////////////////////////////////////////
/* System Constants ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinSystemVolume = 0;
const uint8_t kMaxSystemVolume = 20;

const uint8_t kMinSystemPan = 0;
const uint8_t kMaxSystemPan = 20;

const uint8_t kMinSystemMidiIn = 0;
const uint8_t kMaxSystemMidiIn = 16;

const uint8_t kMinSystemMidiOut = 0;
const uint8_t kMaxSystemMidiOut = 16;

const uint8_t kMinSystemSyncIn = 0;
const uint8_t kMaxSystemSyncIn = 1;

const uint8_t kMinSystemSyncOut = 0;
const uint8_t kMaxSystemSyncOut = 2;

const uint8_t kInitialSystemVolume = 15;
const uint8_t kInitialSystemPan = 10;
const uint8_t kInitialSystemLimiter = true;
const uint8_t kInitialSystemMidiIn = 0;
const uint8_t kInitialSystemMidiOut = 0;
const uint8_t kInitialSystemSyncIn = 0;
const uint8_t kInitialSystemSyncOut = 0;

struct SystemVolumeData {
    char nameLongR[11];
    float data;
};

const struct SystemVolumeData kSystemVolumeDataLibrary[] = {
    {"        00", 0.0000}, {"        05", 0.0025}, {"        10", 0.0100}, {"        15", 0.0225}, {"        20", 0.0400}, {"        25", 0.0625}, {"        30", 0.0900}, {"        35", 0.1225}, {"        40", 0.1600}, {"        45", 0.2025}, {"        50", 0.2500}, {"        55", 0.3025}, {"        60", 0.3600}, {"        65", 0.4225}, {"        70", 0.4900}, {"        75", 0.5625}, {"        80", 0.6400}, {"        85", 0.7225}, {"        90", 0.8100}, {"        95", 0.9025}, {"       100", 1.00}};

struct SystemPanData {
    char nameShortR[5];
    uint8_t left;
    uint8_t right;
};

const struct SystemPanData kSystemPanDataLibrary[] = {
    {"L+10", 20, 0}, {" L+9", 20, 2}, {" L+8", 20, 4}, {" L+7", 20, 6}, {" L+6", 20, 8}, {" L+5", 20, 10}, {" L+4", 20, 12}, {" L+3", 20, 14}, {" L+2", 20, 16}, {" L+1", 20, 18}, {"  LR", 20, 20}, {" R+1", 18, 20}, {" R+2", 16, 20}, {" R+3", 14, 20}, {" R+4", 12, 20}, {" R+5", 10, 20}, {" R+6", 8, 20}, {" R+7", 6, 20}, {" R+8", 4, 20}, {" R+9", 2, 20}, {"R+10", 0, 20}};

struct SystemMidiData {
    char nameShortR[5];
    bool active;
    uint8_t channel;
};

const struct SystemMidiData kSystemMidiDataLibrary[]{
    {" OFF", false, 0}, {"  01", true, 0}, {"  02", true, 1}, {"  03", true, 2}, {"  04", true, 3}, {"  05", true, 4}, {"  06", true, 5}, {"  07", true, 6}, {"  08", true, 7}, {"  09", true, 8}, {"  10", true, 9}, {"  11", true, 10}, {"  12", true, 11}, {"  13", true, 12}, {"  14", true, 13}, {"  15", true, 14}, {"  16", true, 15}};

struct SystemSyncInData {
    char nameShortR[5];
    bool tempoTrigger;
    bool playTrigger;
    bool beatTrigger;
};

const struct SystemSyncInData kSystemSyncInDataLibrary[] = {
    {" OFF", false, false, false}, {"  RW", true, true, false}};

struct SystemSyncOutData {
    char nameShortR[5];
    bool tempoTrigger;
    bool playTrigger;
    bool beatTrigger;
};

const struct SystemSyncOutData kSystemSyncOutDataLibrary[] = {
    {" OFF", false, false, false}, {"  RW", true, true, false}, {" STD", false, false, true}};

typedef enum {
    SYNC_TEMPO = 0x00,
    SYNC_RESET = 0xC8,      // 200
    SYNC_TRIG_RESET = 0xC9, // 201
    SYNC_PLAY = 0xCA,       // 202
    SYNC_TRIG_STOP = 0xCB,  // 203
    SYNC_STOP = 0xCC,       // 204
    SYNC_RECORD = 0xCD,     // 205
} SyncCommand;

typedef enum {
    SYS_ACTION_NONE = 0x00,
    SYS_ACTION_UP = 0x01,
    SYS_ACTION_DOWN = 0x02
} SystemTransitionAction;

struct SystemVolumeTransition {
    bool active;

    float targetVolume;
    SystemTransitionAction actionVolume;
};

struct SystemPanTransition {
    bool active;

    float targetVolumeLeft;
    SystemTransitionAction actionVolumeLeft;

    float targetVolumeRight;
    SystemTransitionAction actionVolumeRight;
};

struct Midi {
    bool rxActive = false;
    bool txActive = false;

    uint8_t rxChannel = 0;
    uint8_t txChannel = 0;

    uint8_t rxNoteOnReadStage = 0;
    uint8_t rxNoteOnWriteFlag = 0;
    uint8_t rxNoteOnKey = 0;
    uint8_t rxNoteOnVelocity = 0;

    uint8_t rxNoteOffReadStage = 0;
    uint8_t rxNoteOffWriteFlag = 0;
    uint8_t rxNoteOffKey = 0;
    uint8_t rxNoteOffVelocity = 0;

    bool txTriggerNoteOn[2] = {false};
    bool txTriggerNoteOff[2] = {false};
    uint8_t txDataNoteOn[2];
    uint8_t txDataNoteOff[2];

    const uint8_t txData[10] = {0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32, 0x33};
};

struct Sync {
    bool syncInTempo = false;
    bool syncInPlay = false;
    bool syncInBeat = false;

    bool syncOutTempo = false;
    bool syncOutPlay = false;
    bool syncOutBeat = false;

    bool slaveMode = false;
    bool masterMode = false;
};

struct Sys {
    uint8_t volume = kInitialSystemVolume;
    uint8_t pan = kInitialSystemPan;
    bool limiter = kInitialSystemLimiter;
    uint8_t midiIn = kInitialSystemMidiIn;
    uint8_t midiOut = kInitialSystemMidiOut;
    uint8_t syncIn = kInitialSystemSyncIn;
    uint8_t syncOut = kInitialSystemSyncOut;

    Midi midi;
    Sync sync;

    float volumeFloat = kSystemVolumeDataLibrary[volume].data;
    float volumeLeftFloat = kSystemVolumeDataLibrary[kSystemPanDataLibrary[pan].left].data;
    float volumeRightFloat = kSystemVolumeDataLibrary[kSystemPanDataLibrary[pan].right].data;

    SystemVolumeTransition volumeTransition;
    SystemPanTransition panTransition;
};

////////////////////////////////////////////////////////////////////////////////
/* Rhythm Constants ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinTempo = 60;
const uint8_t kMaxTempo = 160;

const uint8_t kMinMeasure = 1;
const uint8_t kMaxMeasure = 8;

const uint8_t kMinBar = 1;
const uint8_t kMaxBar = 8;

const uint8_t kMinQuantize = 0;
const uint8_t kMaxQuantize = 6;

const uint8_t kInitialTempo = 120;
const uint8_t kInitialMeasure = 4;
const uint8_t kInitialBar = 4;
const uint8_t kInitialQuantize = 2;

const uint16_t kMeasureInterval = 800;
const uint16_t kMeasureHalfInterval = kMeasureInterval / 2;
const uint16_t kMinSongInterval = 800;
const uint16_t kMaxSongInterval = 64000;
const float kQuantizeInterval[] = {50, 100, 200, 400, 800, 1600, 3200};

const uint16_t kSecondsinOneMinute = 60;
const uint32_t kMicroSecondsinOneSecond = 1000000;
const uint32_t kMicroSecondsinOneMinute = 60000000;

/*----------------------------------------------------------------------------*/

struct QuantizeData {
    char nameShortR[3];
    char nameLongR[11];
};

const struct QuantizeData kQuantizeDataLibrary[] = {
    {"64", "      1:64"},
    {"32", "      1:32"},
    {"16", "      1:16"},
    {"08", "       1:8"},
    {"04", "       1:4"},
    {"02", "       1:2"},
    {"01", "       1:1"}};

/*----------------------------------------------------------------------------*/

struct Rhythm {
    uint8_t tempo = kInitialTempo;
    uint8_t measure = kInitialMeasure;
    uint8_t bar = kInitialBar;
    uint8_t quantize = kInitialQuantize;
    uint8_t measureTotal = kInitialMeasure * kInitialBar;

    bool measureLock = true;
    bool barLock = true;
    bool quantizeLock = true;
};

////////////////////////////////////////////////////////////////////////////////
/* Metronome Constants -------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinMetroSample = 0;
const uint8_t kMaxMetroSample = 4;

const uint8_t kMinMetroVolume = 0;
const uint8_t kMaxMetroVolume = 20;

const bool kInitialMetroActive = true;
const bool kInitialMetroPrecount = true;
const uint8_t kInitialMetroSample = 0;
const uint8_t kInitialMetroVolume = 15;

struct MetroSampleData {
    char nameLongR[11];
};

const struct MetroSampleData kMetroSampleDataLibrary[] = {
    {"     KNOCK"},
    {"     BLOCK"},
    {"    TONGUE"},
    {"     TEETH"},
    {"      CLAP"}};

struct MetronomeVolumeData {
    char nameLongR[11];
    float data;
};

const struct MetronomeVolumeData kMetronomeVolumeDataLibrary[] = {
    {"       000", 0.0000}, {"       005", 0.0025}, {"       010", 0.0100}, {"       015", 0.0225}, {"       020", 0.0400}, {"       025", 0.0625}, {"       030", 0.0900}, {"       035", 0.1225}, {"       040", 0.1600}, {"       045", 0.2025}, {"       050", 0.2500}, {"       055", 0.3025}, {"       060", 0.3600}, {"       065", 0.4225}, {"       070", 0.4900}, {"       075", 0.5625}, {"       080", 0.6400}, {"       085", 0.7225}, {"       090", 0.8100}, {"       095", 0.9025}, {"       100", 1.00}};

typedef enum {
    MET_ACTION_NONE = 0x00,
    MET_ACTION_UP = 0x01,
    MET_ACTION_DOWN = 0x02
} MetronomeTransitionAction;

struct MetronomeVolumeTransition {
    bool active;

    float targetVolume;
    MetronomeTransitionAction actionVolume;
};

struct Metronome {
    bool active = kInitialMetroActive;
    bool precount = kInitialMetroPrecount;
    uint8_t sample = kInitialMetroSample;
    uint8_t volume = kInitialMetroVolume;

    float volumeFloat = kMetronomeVolumeDataLibrary[volume].data;

    MetronomeVolumeTransition volumeTransition;

    bool precountState = false;
    uint16_t precounter = 0;
    uint16_t precounterMax = kMeasureInterval * kInitialMeasure;

    bool countDownFlag = false;
    int8_t countDown = kInitialMeasure;
};

////////////////////////////////////////////////////////////////////////////////
/* Lpf Constants -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const bool kInitialLpfActive = true;
const uint16_t kInitialLpfFreq = 5000;
const float kInitialLpfDry = 0.75;
const float kInitialLpfWet = 0.25;

struct Lpf {
    bool active = kInitialLpfActive;
    uint8_t freq = kInitialLpfFreq;

    float dry = kInitialLpfDry;
    float wet = kInitialLpfWet;

    float a0;
    float a1;
    float a2;
    float b1;
    float b2;

    int32_t dataIn[3] = {0};
    int32_t dataOut[3] = {0};

    void initialize() {
        calculateFilterCoef();
    }

    void calculateFilterCoef() {
        float Q = 0.707;
        float norm;
        float K = tan(M_PI * freq / kAudioSampleRate);

        norm = 1.0f / (1.0f + K / Q + K * K);
        a0 = K * K * norm;
        a1 = 2.0f * a0;
        a2 = a0;
        b1 = 2.0f * (K * K - 1.0f) * norm;
        b2 = (1.0f - K / Q + K * K) * norm;
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Eq Constants --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinEqQ = 0;
const uint8_t kMaxEqQ = 7;

const uint8_t kMinEqFreq = 0;
const uint8_t kMaxEqFreq = 46;

const uint8_t kMinEqGain = 0;
const uint8_t kMaxEqGain = 36;

const bool kInitialEqActive = true;

const uint8_t kEqGainZero = 24;

const uint8_t kInitialEqFreqLowShelf = 4;
const uint8_t kInitialEqGainLowShelf = 28;

const uint8_t kInitialEqFreqHighShelf = 31;
const uint8_t kInitialEqGainHighShelf = 20;

const uint8_t kInitialEqQPeak[4] = {0, 0, 0, 0};
const uint8_t kInitialEqFreqPeak[4] = {9, 12, 27, 29};
const uint8_t kInitialEqGainPeak[4] = {26, 24, 24, 22};

typedef enum {
    EQ_LOWPASS = 0x00,
    EQ_HIGHPASS = 0x01,
    EQ_BANDPASS = 0x02,
    EQ_NOTCH = 0x03,
    EQ_PEAK = 0x04,
    EQ_LOWSHELF = 0x05,
    EQ_HIGHSHELF = 0x06
} EqType;

struct EqQData {
    char nameShortR[5];
    float data;
};

const struct EqQData kEqQDataLibrary[] = {
    {"  01", 1.0f}, {"  02", 2.0f}, {"  03", 3.0f}, {"  04", 4.0f}, {"  05", 5.0f}, {"  06", 6.0f}, {"  07", 7.0f}, {"  08", 8.0f}};

struct EqFreqData {
    char nameShortR[5];
    uint16_t data;
};

const EqFreqData kEqFreqDataLibrary[] = {
    {"  10", 10}, {"  20", 20}, {"  30", 30}, {"  40", 40}, {"  50", 50}, {"  60", 60}, {"  70", 70}, {"  80", 80}, {"  90", 90}, {" 100", 100}, {" 150", 150}, {" 200", 200}, {" 250", 250}, {" 300", 300}, {" 350", 350}, {" 400", 400}, {" 450", 450}, {" 500", 500}, {" 550", 550}, {" 600", 600}, {" 650", 650}, {" 700", 700}, {" 750", 750}, {" 800", 800}, {" 850", 850}, {" 900", 900}, {" 950", 950}, {"  1K", 1000}, {"  2K", 2000}, {"  3K", 3000}, {"  4K", 4000}, {"  5K", 5000}, {"  6K", 6000}, {"  7K", 7000}, {"  8K", 8000}, {"  9K", 9000}, {" 10K", 10000}, {" 11K", 11000}, {" 12K", 12000}, {" 13K", 13000}, {" 14K", 14000}, {" 15K", 15000}, {" 16K", 16000}, {" 17K", 17000}, {" 18K", 18000}, {" 19K", 19000}, {" 20K", 20000}};

struct EqGainData {
    char nameShortR[5];
    int16_t data;
};

const struct EqGainData kEqGainDataLibrary[] = {
    {" -24", -24}, {" -23", -23}, {" -22", -22}, {" -21", -21}, {" -20", -20}, {" -19", -19}, {" -18", -18}, {" -17", -17}, {" -16", -16}, {" -15", -15}, {" -14", -14}, {" -13", -13}, {" -12", -12}, {" -11", -11}, {" -10", -10}, {" -09", -9}, {" -08", -8}, {" -07", -7}, {" -06", -6}, {" -05", -5}, {" -04", -4}, {" -03", -3}, {" -02", -2}, {" -01", -1}, {" +00", 0}, {" +01", 1}, {" +02", 2}, {" +03", 3}, {" +04", 4}, {" +05", 5}, {" +06", 6}, {" +07", 7}, {" +08", 8}, {" +09", 9}, {" +10", 10}, {" +11", 11}, {" +12", 12}};

struct EqFilter {
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float b1 = 0;
    float b2 = 0;

    int32_t dataIn[3] = {0};
    int32_t dataOut[3] = {0};
};

typedef enum {
    EQ_ACTION_NONE = 0x00,
    EQ_ACTION_UP = 0x01,
    EQ_ACTION_DOWN = 0x02
} EqTransitionAction;

typedef enum {
    EQ_MODE_NONE = 0x00,
    EQ_MODE_ACTIVE = 0x01,
} EqTransitionMode;

typedef enum {
    EQ_PHASE_NONE = 0x00,
    EQ_PHASE_A = 0x01,
    EQ_PHASE_B = 0x02
} EqTransitionPhase;

struct EqGenTransition {
    bool active;

    EqTransitionMode mode;
    EqTransitionPhase phase;

    bool activeActive;
    bool targetActive;

    float activeDry;
    float targetDry;
    EqTransitionAction actionDry;

    float activeWet;
    float targetWet;
    EqTransitionAction actionWet;
};

struct Eq {
    bool active = kInitialEqActive;

    EqGenTransition genTransition;

    uint8_t freqLowShelf;
    uint8_t gainLowShelf;
    EqFilter filterLowShelf;

    uint8_t freqHighShelf;
    uint8_t gainHighShelf;
    EqFilter filterHighShelf;

    uint8_t qPeak[4] = {kInitialEqQPeak[0], kInitialEqQPeak[1], kInitialEqQPeak[2], kInitialEqQPeak[3]};
    uint8_t freqPeak[4] = {kInitialEqFreqPeak[0], kInitialEqFreqPeak[1], kInitialEqFreqPeak[2], kInitialEqFreqPeak[3]};
    uint8_t gainPeak[4] = {kInitialEqGainPeak[0], kInitialEqGainPeak[1], kInitialEqGainPeak[2], kInitialEqGainPeak[3]};

    EqFilter filterPeak[4];

    void initialize() {
        active = kInitialEqActive;

        freqLowShelf = kInitialEqFreqLowShelf;
        gainLowShelf = kInitialEqGainLowShelf;

        freqHighShelf = kInitialEqFreqHighShelf;
        gainHighShelf = kInitialEqGainHighShelf;

        calculateCoefLowShelf();
        calculateCoefHighShelf();
        calculateCoefPeak(0);
        calculateCoefPeak(1);
        calculateCoefPeak(2);
        calculateCoefPeak(3);
    }

    void calculateCoefLowShelf() {
        calculateCoef(EQ_LOWSHELF, kAudioSampleRate, kEqFreqDataLibrary[freqLowShelf].data, 0.707, kEqGainDataLibrary[gainLowShelf].data, filterLowShelf);
    }

    void calculateCoefHighShelf() {
        calculateCoef(EQ_HIGHSHELF, kAudioSampleRate, kEqFreqDataLibrary[freqHighShelf].data, 0.707, kEqGainDataLibrary[gainHighShelf].data, filterHighShelf);
    }

    void calculateCoefPeak(uint8_t peakNum) {
        calculateCoef(EQ_PEAK, kAudioSampleRate, kEqFreqDataLibrary[freqPeak[peakNum]].data, kEqQDataLibrary[qPeak[peakNum]].data, kEqGainDataLibrary[gainPeak[peakNum]].data, filterPeak[peakNum]);
    }

    void calculateCoef(uint8_t type, uint32_t sampleRate, uint32_t freq, float Q, float gain, EqFilter &filter) {
        float a0, a1, a2, b1, b2, norm;

        float PI = 3.141592653f;
        float SQ = 1.414213562f;
        float V = pow(10, fabs(gain) / 20.0);
        float K = tan(PI * freq / sampleRate);

        switch (type) {
        case EQ_LOWPASS:
            norm = 1.0f / (1.0f + K / Q + K * K);
            a0 = K * K * norm;
            a1 = 2.0f * a0;
            a2 = a0;
            b1 = 2.0f * (K * K - 1.0f) * norm;
            b2 = (1.0f - K / Q + K * K) * norm;
            break;

        case EQ_HIGHPASS:
            norm = 1 / (1 + K / Q + K * K);
            a0 = 1 * norm;
            a1 = -2 * a0;
            a2 = a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case EQ_BANDPASS:
            norm = 1 / (1 + K / Q + K * K);
            a0 = K / Q * norm;
            a1 = 0;
            a2 = -a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case EQ_NOTCH:
            norm = 1 / (1 + K / Q + K * K);
            a0 = (1 + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = a0;
            b1 = a1;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case EQ_PEAK:
            if (gain >= 0) { // boost
                norm = 1 / (1 + 1 / Q * K + K * K);
                a0 = (1 + V / Q * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - V / Q * K + K * K) * norm;
                b1 = a1;
                b2 = (1 - 1 / Q * K + K * K) * norm;
            } else { // cut
                norm = 1 / (1 + V / Q * K + K * K);
                a0 = (1 + 1 / Q * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - 1 / Q * K + K * K) * norm;
                b1 = a1;
                b2 = (1 - V / Q * K + K * K) * norm;
            }
            break;

        case EQ_LOWSHELF:
            if (gain >= 0) { // boost
                norm = 1 / (1 + SQ * K + K * K);
                a0 = (1 + sqrt(2 * V) * K + V * K * K) * norm;
                a1 = 2 * (V * K * K - 1) * norm;
                a2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
                b1 = 2 * (K * K - 1) * norm;
                b2 = (1 - SQ * K + K * K) * norm;
            } else { // cut
                norm = 1 / (1 + sqrt(2 * V) * K + V * K * K);
                a0 = (1 + SQ * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - SQ * K + K * K) * norm;
                b1 = 2 * (V * K * K - 1) * norm;
                b2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
            }
            break;

        case EQ_HIGHSHELF:
            if (gain >= 0) { // boost
                norm = 1 / (1 + SQ * K + K * K);
                a0 = (V + sqrt(2 * V) * K + K * K) * norm;
                a1 = 2 * (K * K - V) * norm;
                a2 = (V - sqrt(2 * V) * K + K * K) * norm;
                b1 = 2 * (K * K - 1) * norm;
                b2 = (1 - SQ * K + K * K) * norm;
            } else { // cut
                norm = 1 / (V + sqrt(2 * V) * K + K * K);
                a0 = (1 + SQ * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - SQ * K + K * K) * norm;
                b1 = 2 * (K * K - V) * norm;
                b2 = (V - sqrt(2 * V) * K + K * K) * norm;
            }
            break;

        default:
            break;
        }

        filter.a0 = a0;
        filter.a1 = a1;
        filter.a2 = a2;
        filter.b1 = b1;
        filter.b2 = b2;
    }

    void cleanMemory() {
        // low shelf
        filterLowShelf.dataIn[0] = 0;
        filterLowShelf.dataIn[1] = 0;
        filterLowShelf.dataIn[2] = 0;

        filterLowShelf.dataOut[0] = 0;
        filterLowShelf.dataOut[1] = 0;
        filterLowShelf.dataOut[2] = 0;
        // high shelf
        filterHighShelf.dataIn[0] = 0;
        filterHighShelf.dataIn[1] = 0;
        filterHighShelf.dataIn[2] = 0;

        filterHighShelf.dataOut[0] = 0;
        filterHighShelf.dataOut[1] = 0;
        filterHighShelf.dataOut[2] = 0;
        // peak
        for (uint8_t i = 0; i < 4; i++) {
            filterPeak[i].dataIn[0] = 0;
            filterPeak[i].dataIn[1] = 0;
            filterPeak[i].dataIn[2] = 0;

            filterPeak[i].dataOut[0] = 0;
            filterPeak[i].dataOut[1] = 0;
            filterPeak[i].dataOut[2] = 0;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Lfo Constants -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinLfoType = 0;
const uint8_t kMaxLfoType = 24;

const uint8_t kMinLfoTarget = 0;
const uint8_t kMaxLfoTarget = 2;

const uint8_t kMinLfoRate = 0;
const uint8_t kMaxLfoRate = 10;

const uint8_t kMinLfoDepth = 0;
const uint8_t kMaxLfoDepth = 20;

const bool kInitialLfoActive = false;
const uint8_t kInitialLfoType = 0;
const uint8_t kInitialLfoTarget[2] = {0, 1};
const uint8_t kInitialLfoRate = 5;
const uint8_t kInitialLfoDepth = 20;
const bool kInitialLfoLoop = true;

struct LfoTypeData {
    char nameShortR[5];
    char nameLongR[11];
};

const struct LfoTypeData kLfoTypeDataLibrary[] = {
    {" TRI", "     TRI-1"},
    {" TRI", "     TRI-2"},
    {"RAMP", "    RAMP-1"},
    {"RAMP", "    RAMP-2"},
    {"DOME", "    DOME-1"},
    {"DOME", "    DOME-2"},
    {"SINE", "    SINE-1"},
    {"SINE", "    SINE-2"},
    {"SINE", "    SINE-3"},
    {"SINE", "    SINE-4"},
    {"PULS", "   PULSE-1"},
    {"PULS", "   PULSE-2"},
    {"PULS", "   PULSE-3"},
    {"PULS", "   PULSE-4"},
    {"SHOT", "    SHOT-1"},
    {"SHOT", "    SHOT-2"},
    {"JUNO", "    JUNO-1"},
    {"JUNO", "    JUNO-2"},
    {"JUNO", "    JUNO-3"},
    {"JUNO", "    JUNO-4"},
    {"HALF", "    HALF-1"},
    {"HALF", "    HALF-2"},
    {"HALF", "    HALF-3"},
    {"HALF", "    HALF-4"},
    {"NOIS", "     NOISE"}};

struct LfoTargetData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct LfoTargetData kLfoTargetDataLibrary[] = {
    {"WAV ", " WAV"},
    {"LVL ", " LVL"},
    {"TUN ", " TUN"}};

struct LfoRateData {
    char nameShortL[5];
    char nameShortR[5];
    float rate;
};

const struct LfoRateData kLfoRateDataLibrary[] = {
    {"1/8 ", " 1/8", 0.125000},
    {"1/6 ", " 1/6", 0.166666},
    {"1/4 ", " 1/4", 0.250000},
    {"1/3 ", " 1/3", 0.333333},
    {"1/2 ", " 1/2", 0.500000},
    {"1/1 ", " 1/1", 1.000000},
    {"2/1 ", " 2/1", 2.000000},
    {"3/1 ", " 3/1", 3.000000},
    {"4/1 ", " 4/1", 4.000000},
    {"6/1 ", " 6/1", 6.000000},
    {"8/1 ", " 8/1", 8.000000}};

struct LfoDepthData {
    char nameShortL[5];
    char nameShortR[5];
    float depth;
};

const struct LfoDepthData kLfoDepthDataLibrary[] = {
    {"00  ", "  00", 0.00},
    {"05  ", "  05", 0.05},
    {"10  ", "  10", 0.10},
    {"15  ", "  15", 0.15},
    {"20  ", "  20", 0.20},
    {"25  ", "  25", 0.25},
    {"30  ", "  30", 0.30},
    {"35  ", "  35", 0.35},
    {"40  ", "  40", 0.40},
    {"45  ", "  45", 0.45},
    {"50  ", "  50", 0.50},
    {"55  ", "  55", 0.55},
    {"60  ", "  60", 0.60},
    {"65  ", "  65", 0.65},
    {"70  ", "  70", 0.70},
    {"75  ", "  75", 0.75},
    {"80  ", "  80", 0.80},
    {"85  ", "  85", 0.85},
    {"90  ", "  90", 0.90},
    {"95  ", "  95", 0.95},
    {"100 ", " 100", 1.00}};

struct Lfo {
    uint8_t number;
    bool active;
    uint8_t type;
    uint8_t target;
    uint8_t rate;
    uint8_t depth;
    bool loop;

    Lfo(uint8_t number_) {
        number = number_;
        active = kInitialLfoActive;
        type = kInitialLfoType;
        target = kInitialLfoTarget[number];
        rate = kInitialLfoRate;
        depth = kInitialLfoDepth;
        loop = kInitialLfoLoop;
    }
    ~Lfo() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Osc Constants -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinOscLevel = 0;
const uint8_t kMaxOscLevel = 100;

const uint8_t kMinOscTune = 0;
const uint8_t kMaxOscTune = 24;

const uint8_t kMinOscPhase = 0;
const uint8_t kMaxOscPhase = 3;

const bool kInitialOscActive = false;
const int16_t kInitialOscWavetable = -1;
const uint8_t kInitialOscLevel = 75;
const uint8_t kInitialOscTune = 12;
const uint8_t kInitialOscPhase = 0;
const bool kInitialOscNormalize = true;
const bool kInitialOscXFlip = false;
const bool kInitialOscYFlip = false;

const float kOscVolumeCoef = 1.00;

struct OscTuneData {
    char nameShortL[5];
    char nameShortR[5];
    int8_t tune;
};

const struct OscTuneData kOscTuneDataLibrary[] = {
    {"-12 ", " -12", -12},
    {"-11 ", " -11", -11},
    {"-10 ", " -10", -10},
    {"-09 ", " -09", -9},
    {"-08 ", " -08", -8},
    {"-07 ", " -07", -7},
    {"-06 ", " -06", -6},
    {"-05 ", " -05", -5},
    {"-04 ", " -04", -4},
    {"-03 ", " -03", -3},
    {"-02 ", " -02", -2},
    {"-01 ", " -01", -1},
    {"+00 ", " +00", 0},
    {"+01 ", " +01", 1},
    {"+02 ", " +02", 2},
    {"+03 ", " +03", 3},
    {"+04 ", " +04", 4},
    {"+05 ", " +05", 5},
    {"+06 ", " +06", 6},
    {"+07 ", " +07", 7},
    {"+08 ", " +08", 8},
    {"+09 ", " +09", 9},
    {"+10 ", " +10", 10},
    {"+11 ", " +11", 11},
    {"+12 ", " +12", 12}};

struct OscPhaseData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct OscPhaseData kOscPhaseDataLibrary[] = {
    {"00  ", "  00"},
    {"90  ", "  90"},
    {"180 ", " 180"},
    {"270 ", " 270"}};

const char kBlankWavetableData[] = "-                                                                           ";

#define CH_NA 0x00
#define CH_MONO 0x01
#define CH_STEREO 0x02

#define BD_NA 0x00
#define BD_08 0x01
#define BD_16 0x02
#define BD_24 0x03
#define BD_32 0x04

#define ST_NA 0xFF
#define ST_001 0x00
#define ST_002 0x01
#define ST_004 0x02
#define ST_008 0x03
#define ST_016 0x04
#define ST_024 0x05
#define ST_032 0x06
#define ST_040 0x07
#define ST_048 0x08
#define ST_056 0x09
#define ST_064 0x0A
#define ST_072 0x0B
#define ST_080 0x0C
#define ST_088 0x0D
#define ST_096 0x0E
#define ST_104 0x0F
#define ST_112 0x10
#define ST_120 0x11
#define ST_128 0x12
#define ST_136 0x13
#define ST_144 0x14
#define ST_152 0x15
#define ST_160 0x16
#define ST_168 0x17
#define ST_176 0x18
#define ST_184 0x19
#define ST_192 0x1A
#define ST_200 0x1B
#define ST_208 0x1C
#define ST_216 0x1D
#define ST_224 0x1E
#define ST_232 0x1F
#define ST_240 0x20
#define ST_248 0x21
#define ST_256 0x22

struct ChannelData {
    char name[2];
    uint8_t channel;
};

const struct ChannelData kChannelDataLibrary[] = {
    {"X", 0x00},
    {"M", 0x01},
    {"S", 0x02}};

struct BitdepthData {
    char name[6];
    uint8_t bitdepth;
};

const struct BitdepthData kBitdepthDataLibrary[] = {
    {"     ", 0x00},
    {"08BIT", 0x08},
    {"16BIT", 0x10},
    {"24BIT", 0x18},
    {"32BIT", 0x20}};

const uint16_t kWavetableSampleSize = 2048;

struct StepData {
    uint16_t size;
    char name[7];
    uint8_t min;
    uint8_t max;
    uint8_t data[9];
};

const struct StepData kStepDataLibrary[] = {
    {1, "   001", 0, 0, {0}},
    {2, "   002", 0, 1, {0, 1}},
    {4, "   004", 0, 3, {0, 1, 2, 3}},
    {8, "   008", 0, 7, {0, 1, 2, 3, 4, 5, 6, 7}},
    {16, "   016", 0, 8, {0, 1, 3, 5, 7, 9, 11, 13, 15}},
    {24, "   024", 0, 8, {0, 2, 5, 8, 11, 14, 17, 20, 23}},
    {32, "   032", 0, 8, {0, 3, 7, 11, 15, 19, 23, 27, 31}},
    {40, "   040", 0, 8, {0, 4, 9, 14, 19, 24, 29, 34, 39}},
    {48, "   048", 0, 8, {0, 5, 11, 17, 23, 29, 35, 41, 47}},
    {56, "   056", 0, 8, {0, 6, 13, 20, 27, 34, 41, 48, 55}},
    {64, "   064", 0, 8, {0, 7, 15, 23, 31, 39, 47, 55, 63}},
    {72, "   072", 0, 8, {0, 8, 17, 26, 35, 44, 53, 62, 71}},
    {80, "   080", 0, 8, {0, 9, 19, 29, 39, 49, 59, 69, 79}},
    {88, "   088", 0, 8, {0, 10, 21, 32, 43, 54, 65, 76, 87}},
    {96, "   096", 0, 8, {0, 11, 23, 35, 47, 59, 71, 83, 95}},
    {104, "   104", 0, 8, {0, 12, 25, 38, 51, 64, 77, 90, 103}},
    {112, "   112", 0, 8, {0, 13, 27, 41, 55, 69, 83, 97, 111}},
    {120, "   120", 0, 8, {0, 14, 29, 44, 59, 74, 89, 104, 119}},
    {128, "   128", 0, 8, {0, 15, 31, 47, 63, 79, 95, 111, 127}},
    {136, "   136", 0, 8, {0, 16, 33, 50, 67, 84, 101, 118, 135}},
    {144, "   144", 0, 8, {0, 17, 35, 53, 71, 89, 107, 125, 143}},
    {152, "   152", 0, 8, {0, 18, 37, 56, 75, 94, 113, 132, 151}},
    {160, "   160", 0, 8, {0, 19, 39, 59, 79, 99, 119, 139, 159}},
    {168, "   168", 0, 8, {0, 20, 41, 62, 83, 104, 125, 146, 167}},
    {176, "   176", 0, 8, {0, 21, 43, 65, 87, 109, 131, 153, 175}},
    {184, "   184", 0, 8, {0, 22, 45, 68, 91, 114, 137, 160, 183}},
    {192, "   192", 0, 8, {0, 23, 47, 71, 95, 119, 143, 167, 191}},
    {200, "   200", 0, 8, {0, 24, 49, 74, 99, 124, 149, 174, 199}},
    {208, "   208", 0, 8, {0, 25, 51, 77, 103, 129, 155, 181, 207}},
    {216, "   216", 0, 8, {0, 26, 53, 80, 107, 134, 161, 188, 215}},
    {224, "   224", 0, 8, {0, 27, 55, 83, 111, 139, 167, 195, 223}},
    {232, "   232", 0, 8, {0, 28, 57, 86, 115, 144, 173, 202, 231}},
    {240, "   240", 0, 8, {0, 29, 59, 89, 119, 149, 179, 209, 239}},
    {248, "   248", 0, 8, {0, 30, 61, 92, 123, 154, 185, 216, 247}},
    {256, "   256", 0, 8, {0, 31, 63, 95, 127, 159, 191, 223, 255}}};

struct WavetableData {
    char num[4 + 1] = "----";
    char fileName[(kFileNameSize * 2) + 10] = ""; // Sample/InstName/SampleName.wav
    char nameShortR[10 + 1] = "  --------";
    char nameLongR[kFileNameSize + 1] = "";

    uint8_t channel = CH_NA;
    uint8_t bitdepth = BD_NA;
    uint8_t step = ST_NA;

    float coefSampleSize;
    uint32_t sampleByteSize;
    uint32_t sampleSize;
    uint32_t stepSize;
};

struct WavetableSector {
    uint32_t size;
    float normMultiplier = 1.0f;
};

struct riff_chunk {
    uint32_t chunkId;    // 00 --> 0x46464952
    uint32_t chunkSize;  // 04 --> --
    uint32_t fileFormat; // 08 --> 0x45564157
};

struct fmt_chunk {
    uint32_t chunkId;      // 00 --> 0x20746D66
    uint32_t chunkSize;    // 04 --> 16 | 18 | 40
    uint16_t audioFormat;  // 08 --> 0x01 | 0x03
    uint16_t nbrChannels;  // 10 --> 1 | 2
    uint32_t sampleRate;   // 12
    uint32_t byteRate;     // 16
    uint16_t blockAlign;   // 20
    uint16_t bitPerSample; // 22

    uint32_t chunkStartByte;
};

struct data_chunk {
    uint32_t chunkId;   // 00 --> 0x61746164
    uint32_t chunkSize; // 04 --> --

    uint32_t chunkStartByte;
};

struct WavData {
    struct riff_chunk riff_chunk;
    struct fmt_chunk fmt_chunk;
    struct data_chunk data_chunk;
};

const uint8_t kOscLibrarySize = 2;
const uint8_t kOscLfoLibrarySize = 2;

struct Osc {
    uint8_t number;

    uint8_t active = kInitialOscActive;
    int16_t wavetableSelected = kInitialOscWavetable;
    int16_t wavetableLoaded = kInitialOscWavetable;
    uint8_t level = kInitialOscLevel;
    uint8_t tune = kInitialOscTune;
    uint8_t phase = kInitialOscPhase;
    bool normalize = kInitialOscNormalize;
    uint8_t start;
    uint8_t end;
    uint16_t waveStart;
    uint16_t waveEnd;
    bool xFlip = kInitialOscXFlip;
    bool yFlip = kInitialOscYFlip;

    Lfo lfo[2] = {{0}, {1}};

    bool wavetableSelectedReadError = false;
    bool wavetableSelectedTypeError = false;

    struct WavetableData wavetableSelectedData;
    struct WavData wavSelectedData;

    struct WavetableData wavetableLoadedData;
    struct WavData wavLoadedData;

    uint8_t playWavetableSector = 0;
    uint8_t writeWavetableSector = 0;

    WavetableSector wavetableSector[2];

    bool wavetablePlay = false;

    Osc(uint8_t number_) : number(number_) {}
    ~Osc() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Filter Constants ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinFilterType = 1;
const uint8_t kMaxFilterType = 4;

const uint8_t kMinFilterFreq = 0;
const uint8_t kMaxFilterFreq = 46;

const uint8_t kMinFilterRes = 0;
const uint8_t kMaxFilterRes = 10;

const uint8_t kMinFilterSlope = 0;
const uint8_t kMaxFilterSlope = 1;

const uint8_t kMinFilterDry = 0;
const uint8_t kMaxFilterDry = 20;

const uint8_t kMinFilterWet = 0;
const uint8_t kMaxFilterWet = 20;

const bool kInitialFilterActive = false;
const uint8_t kInitialFilterType = 1;
const uint8_t kInitialFilterFreq = 27;
const uint8_t kInitialFilterRes = 0;
const uint8_t kInitialFilterSlope = 1;
const uint8_t kInitialFilterDry = 0;
const uint8_t kInitialFilterWet = 20;

const uint8_t kFilterLibrarySize = 2;

typedef enum {
    FIL_OFF = 0x00,
    FIL_LPF = 0x01,
    FIL_HPF = 0x02,
    FIL_BPF = 0x03,
    FIL_BSF = 0x04
} FilterType;

struct FilterTypeData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct FilterTypeData kFilterTypeDataLibrary[] = {
    {"--- ", " ---"}, {"LPF ", " LPF"}, {"HPF ", " HPF"}, {"BPF ", " BPF"}, {"BSF ", " BSF"}};

struct FilterFreqData {
    char nameShortL[5];
    char nameShortR[5];
    uint16_t data;
};

const FilterFreqData kFilterFreqDataLibrary[] = {
    {"10  ", "  10", 10}, {"20  ", "  20", 20}, {"30  ", "  30", 30}, {"40  ", "  40", 40}, {"50  ", "  50", 50}, {"60  ", "  60", 60}, {"70  ", "  70", 70}, {"80  ", "  80", 80}, {"90  ", "  90", 90}, {"100 ", " 100", 100}, {"150 ", " 150", 150}, {"200 ", " 200", 200}, {"250 ", " 250", 250}, {"300 ", " 300", 300}, {"350 ", " 350", 350}, {"400 ", " 400", 400}, {"450 ", " 450", 450}, {"500 ", " 500", 500}, {"550 ", " 550", 550}, {"600 ", " 600", 600}, {"650 ", " 650", 650}, {"700 ", " 700", 700}, {"750 ", " 750", 750}, {"800 ", " 800", 800}, {"850 ", " 850", 850}, {"900 ", " 900", 900}, {"950 ", " 950", 950}, {"1K  ", "  1K", 1000}, {"2K  ", "  2K", 2000}, {"3K  ", "  3K", 3000}, {"4K  ", "  4K", 4000}, {"5K  ", "  5K", 5000}, {"6K  ", "  6K", 6000}, {"7K  ", "  7K", 7000}, {"8K  ", "  8K", 8000}, {"9K  ", "  9K", 9000}, {"10K ", " 10K", 10000}, {"11K ", " 11K", 11000}, {"12K ", " 12K", 12000}, {"13K ", " 13K", 13000}, {"14K ", " 14K", 14000}, {"15K ", " 15K", 15000}, {"16K ", " 16K", 16000}, {"17K ", " 17K", 17000}, {"18K ", " 18K", 18000}, {"19K ", " 19K", 19000}, {"20K ", " 20K", 20000}};

struct FilterResData {
    char nameShortL[5];
    char nameShortR[5];
    float data;
};

const struct FilterResData kFilterResDataLibrary[] = {
    {"00  ", "  00", 0.7f}, {"10  ", "  10", 1.0f}, {"20  ", "  20", 1.5f}, {"30  ", "  30", 2.0f}, {"40  ", "  40", 2.5f}, {"50  ", "  50", 3.0f}, {"60  ", "  60", 3.5f}, {"70  ", "  70", 4.0f}, {"80  ", "  80", 4.5f}, {"90  ", "  90", 5.0f}, {"100 ", " 100", 5.5f}};

struct FilterSlopeData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct FilterSlopeData kFilterSlopeDataLibrary[] = {
    {"06  ", "  06"}, {"12  ", "  12"}};

struct FilterMixData {
    char nameShortL[5];
    char nameShortR[5];
    float data;
};

const struct FilterMixData kFilterMixDataLibrary[] = {
    {"00  ", "  00", 0.00}, {"05  ", "  05", 0.05}, {"10  ", "  10", 0.10}, {"15  ", "  15", 0.15}, {"20  ", "  20", 0.20}, {"25  ", "  25", 0.25}, {"30  ", "  30", 0.30}, {"35  ", "  35", 0.35}, {"40  ", "  40", 0.40}, {"45  ", "  45", 0.45}, {"50  ", "  50", 0.50}, {"55  ", "  55", 0.55}, {"60  ", "  60", 0.60}, {"65  ", "  65", 0.65}, {"70  ", "  70", 0.70}, {"75  ", "  75", 0.75}, {"80  ", "  80", 0.80}, {"85  ", "  85", 0.85}, {"90  ", "  90", 0.90}, {"95  ", "  95", 0.95}, {"100 ", " 100", 1.00}};

typedef enum {
    FIL_ACTION_NONE = 0x00,
    FIL_ACTION_UP = 0x01,
    FIL_ACTION_DOWN = 0x02
} FilterTransitionAction;

typedef enum {
    FIL_MODE_NONE = 0x00,
    FIL_MODE_ACTIVE = 0x01,
    FIL_MODE_TYPE = 0x02,
} FilterTransitionMode;

typedef enum {
    FIL_PHASE_NONE = 0x00,
    FIL_PHASE_A = 0x01,
    FIL_PHASE_B = 0x02
} FilterTransitionPhase;

struct FilterGenTransition {
    bool active;

    FilterTransitionMode mode;
    FilterTransitionPhase phase;

    bool activeActive;
    bool targetActive;

    uint8_t activeType;
    uint8_t targetType;

    float activeDry;
    float targetDry;
    FilterTransitionAction actionDry;

    float activeWet;
    float targetWet;
    FilterTransitionAction actionWet;
};

struct FilterMixTransition {
    bool active;

    float targetDry;
    FilterTransitionAction actionDry;

    float targetWet;
    FilterTransitionAction actionWet;
};

struct Filter {
    uint8_t num;
    bool active;
    uint8_t type;
    uint8_t freq;
    uint8_t res;
    uint8_t slope;
    uint8_t dry;
    uint8_t wet;

    float dryFloat;
    float wetFloat;

    FilterGenTransition genTransition;
    FilterMixTransition mixTransition;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    float a0;
    float a1;
    float a2;
    float b1;
    float b2;

    int32_t dataIn[3] = {0};
    int32_t dataOut[3] = {0};

    void initialize() {
        active = kInitialFilterActive;
        type = kInitialFilterType;
        freq = kInitialFilterFreq;
        res = kInitialFilterRes;
        slope = kInitialFilterSlope;
        dry = kInitialFilterDry;
        wet = kInitialFilterWet;

        calculateCoef();
        cleanMemory();
    }

    void calculateCoef() {
        uint32_t sampleRate = 44100;
        uint32_t frequency = kFilterFreqDataLibrary[freq].data;
        float Q = kFilterResDataLibrary[res].data;
        float norm;

        float K = tan(M_PI * frequency / sampleRate);

        switch (type) {
        case FIL_OFF:
            break;

        case FIL_LPF:
            norm = 1.0f / (1.0f + K / Q + K * K);
            a0 = K * K * norm;
            a1 = 2.0f * a0;
            a2 = a0;
            b1 = 2.0f * (K * K - 1.0f) * norm;
            b2 = (1.0f - K / Q + K * K) * norm;
            break;

        case FIL_HPF:
            norm = 1 / (1 + K / Q + K * K);
            a0 = 1 * norm;
            a1 = -2 * a0;
            a2 = a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case FIL_BPF:
            norm = 1 / (1 + K / Q + K * K);
            a0 = K / Q * norm;
            a1 = 0;
            a2 = -a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case FIL_BSF:
            norm = 1 / (1 + K / Q + K * K);
            a0 = (1 + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = a0;
            b1 = a1;
            b2 = (1 - K / Q + K * K) * norm;
            break;
        }
    }

    void cleanMemory() {
        dataIn[0] = 0;
        dataIn[1] = 0;
        dataIn[2] = 0;

        dataOut[0] = 0;
        dataOut[1] = 0;
        dataOut[2] = 0;
    }

    Filter(uint8_t num_) : num(num_) {}
    ~Filter() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Envelope Constants --------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    ENV_OFF = 0x00,
    ENV_ADSR = 0x01,
    ENV_ASR = 0x02,
    ENV_AD = 0x03,
} EnvelopeType;

#define ENV_NONE 0x00
#define ENV_ATTACK 0x01
#define ENV_DECAY 0x02
#define ENV_SUSTAIN 0x03
#define ENV_RELEASE 0x04
#define ENV_END 0x05

const uint8_t kMinEnvelopeType = 1;
const uint8_t kMaxEnvelopeType = 3;

const uint8_t kMinEnvelopeCurve = 0;
const uint8_t kMaxEnvelopeCurve = 2;

const uint8_t kMinEnvelopeTime = 0;
const uint8_t kMaxEnvelopeTime = 30;

const uint8_t kMinEnvelopeLevel = 0;
const uint8_t kMaxEnvelopeLevel = 10;

const bool kInitialEnvelopeActive = true;
const uint8_t kInitialEnvelopeType = ENV_ADSR;
const uint8_t kInitialEnvelopeCurve = 1;
const uint8_t kInitialEnvelopeAttackTime = 1;
const uint8_t kInitialEnvelopeDecayTime = 5;
const uint8_t kInitialEnvelopeSustainLevel = 8;
const uint8_t kInitialEnvelopeReleaseTime = 5;

struct EnvelopeTypeData {
    char nameShortL[5];
    char nameShortR[5];
    uint8_t step;
    bool stage[4];
};

const struct EnvelopeTypeData kEnvelopeTypeDataLibrary[] = {
    {"----", "----", 3, {1, 0, 1, 1}}, {"ADSR", "ADSR", 4, {1, 1, 1, 1}}, {"ASR ", " ASR", 3, {1, 0, 1, 1}}, {"AD  ", "  AD", 2, {1, 1, 0, 0}}};

struct EnvelopeCurve {
    char nameShortL[5];
    char nameShortR[5];
};

const struct EnvelopeCurve kEnvelopeCurveDataLibrary[] = {
    {"LOG ", " LOG"}, {"LIN ", " LIN"}, {"EXP ", " EXP"}};

struct EnvelopeLevelData {
    char nameShortL[5];
    char nameShortR[5];
    float level;
};

const struct EnvelopeLevelData kEnvelopeLevelDataLibrary[] = {
    {"00  ", "  00", 0.000000}, {"10  ", "  10", 0.010000}, {"20  ", "  20", 0.040000}, {"30  ", "  30", 0.090000}, {"40  ", "  40", 0.160000}, {"50  ", "  50", 0.250000}, {"60  ", "  60", 0.360000}, {"70  ", "  70", 0.490000}, {"80  ", "  80", 0.640000}, {"90  ", "  90", 0.810000}, {"100 ", " 100", 1.000000}};

struct EnvelopeTimeData {
    char nameShortL[5];
    char nameShortR[5];
    float time;
    uint32_t sampleSize;
    float sampleInc;
};

const struct EnvelopeTimeData kEnvelopeTimeDataLibrary[] = {
    {"0.0 ", " 0.0", 0.05, 2205, 0.000453515}, {"0.1 ", " 0.1", 0.10, 4410, 0.000226757}, {"0.2 ", " 0.2", 0.20, 8820, 0.000113379}, {"0.3 ", " 0.3", 0.30, 13230, 7.55858e-05}, {"0.4 ", " 0.4", 0.40, 17640, 5.66893e-05}, {"0.5 ", " 0.5", 0.50, 22050, 4.53515e-05}, {"0.6 ", " 0.6", 0.60, 26460, 3.77929e-05}, {"0.7 ", " 0.7", 0.70, 30870, 3.23939e-05}, {"0.8 ", " 0.8", 0.80, 35280, 2.83447e-05}, {"0.9 ", " 0.9", 0.90, 39690, 2.51953e-05}, {"1.0 ", " 1.0", 1.00, 44100, 2.26757e-05}, {"1.1 ", " 1.1", 1.10, 48510, 2.06143e-05}, {"1.2 ", " 1.2", 1.20, 52920, 1.88964e-05}, {"1.3 ", " 1.3", 1.30, 57330, 1.74429e-05}, {"1.4 ", " 1.4", 1.40, 61740, 1.6197e-05}, {"1.5 ", " 1.5", 1.50, 66150, 1.51172e-05}, {"1.6 ", " 1.6", 1.60, 70560, 1.41723e-05}, {"1.7 ", " 1.7", 1.70, 74970, 1.33387e-05}, {"1.8 ", " 1.8", 1.80, 79380, 1.25976e-05}, {"1.9 ", " 1.9", 1.90, 83790, 1.19346e-05}, {"2.0 ", " 2.0", 2.00, 88200, 1.13379e-05}, {"2.1 ", " 2.1", 2.10, 92610, 1.0798e-05}, {"2.2 ", " 2.2", 2.20, 97020, 1.03072e-05}, {"2.3 ", " 2.3", 2.30, 101430, 9.85902e-06}, {"2.4 ", " 2.4", 2.40, 105840, 9.44822e-06}, {"2.5 ", " 2.5", 2.50, 110250, 9.0703e-06}, {"2.6 ", " 2.6", 2.60, 114660, 8.72144e-06}, {"2.7 ", " 2.7", 2.70, 119070, 8.39842e-06}, {"2.8 ", " 2.8", 2.80, 123480, 8.09848e-06}, {"2.9 ", " 2.9", 2.90, 127890, 7.81922e-06}, {"3.0 ", " 3.0", 3.00, 132300, 7.55858e-06}};

struct Envelope {
    bool active;
    uint8_t type;
    uint8_t curve;
    uint8_t attackTime;
    uint8_t decayTime;
    uint8_t sustainLevel;
    uint8_t releaseTime;

    void initialize() {
        active = kInitialEnvelopeActive;
        type = kInitialEnvelopeType;
        curve = kInitialEnvelopeCurve;
        attackTime = kInitialEnvelopeAttackTime;
        decayTime = kInitialEnvelopeDecayTime;
        sustainLevel = kInitialEnvelopeSustainLevel;
        releaseTime = kInitialEnvelopeReleaseTime;
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Delay Constants ----------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint16_t kDelayBufferSize = 96000;

const uint8_t kMinDelayTime = 0;
const uint8_t kMaxDelayTime = 5;

const uint8_t kMinDelayLevel = 0;
const uint8_t kMaxDelayLevel = 16;

const uint8_t kMinDelayFeedback = 0;
const uint8_t kMaxDelayFeedback = 16;

const uint8_t kMinDelayDry = 0;
const uint8_t kMaxDelayDry = 20;

const uint8_t kMinDelayWet = 0;
const uint8_t kMaxDelayWet = 20;

const uint32_t kMinDelayInterval = 0;
const uint32_t kMaxDelayInterval = kDelaySize - 1;

const bool kInitialDelayActive = false;
const uint8_t kInitialDelayTime = 2;
const uint8_t kInitialDelayLevel = 10;
const uint8_t kInitialDelayFeedback = 5;
const uint8_t kInitialDelayDry = 20;
const uint8_t kInitialDelayWet = 10;

struct DelayTimeData {
    char nameShortR[5];
    float data;
};

const struct DelayTimeData kDelayTimeDataLibrary[] = {
    {" 1/8", 0.1250}, {" 1/6", 0.1666}, {" 1/4", 0.2500}, {" 1/3", 0.3333}, {" 1/2", 0.5000}, {" 1/1", 1.0000}};

struct DelayLevelData {
    char nameShortR[5];
    float data;
};

const struct DelayLevelData kDelayLevelDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

struct DelayFeedbackData {
    char nameShortR[5];
    float data;
};

const struct DelayFeedbackData kDelayFeedbackDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

struct Delay {
    bool active;

    uint8_t aTime;
    uint8_t bLevel;
    uint8_t cFeedback;
    uint8_t dDry;
    uint8_t eWet;

    float time;
    float level;
    float feedback;

    uint32_t lag;
    uint32_t playInterval;
    uint32_t recordInterval;
    const uint32_t kDelayBufferSize = kDelaySize;
    int32_t *delayBuffer;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialDelayActive;

        aTime = kInitialDelayTime;
        bLevel = kInitialDelayLevel;
        cFeedback = kInitialDelayFeedback;
        dDry = kInitialDelayDry;
        eWet = kInitialDelayWet;

        time = kDelayTimeDataLibrary[kInitialDelayTime].data;
        level = kDelayLevelDataLibrary[kInitialDelayLevel].data;
        feedback = kDelayFeedbackDataLibrary[kInitialDelayFeedback].data;

        lag = ((60 * kAudioSampleRate) / kInitialTempo) * kDelayTimeDataLibrary[kInitialDelayTime].data;
        playInterval = 0;
        recordInterval = lag;
    }

    void update(uint16_t tempo) {
        lag = (uint32_t)(((60 * kAudioSampleRate) / tempo) * time);
        int32_t playInterval_ = recordInterval - lag;
        if (playInterval_ < 0)
            playInterval_ += kDelayBufferSize;
        playInterval = playInterval_;
    }

    void cleanMemory() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Chorus Constants ---------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint16_t kChorusBufferSize = 5000;

const uint8_t kMinChorusTime = 0;
const uint8_t kMaxChorusTime = 8;

const uint8_t kMinChorusFeedback = 0;
const uint8_t kMaxChorusFeedback = 19;

const uint8_t kMinChorusRate = 0;
const uint8_t kMaxChorusRate = 25;

const uint8_t kMinChorusDry = 0;
const uint8_t kMaxChorusDry = 20;

const uint8_t kMinChorusWet = 0;
const uint8_t kMaxChorusWet = 20;

const bool kInitialChorusActive = false;
const uint8_t kInitialChorusTime = 0;
const uint8_t kInitialChorusFeedback = 5;
const uint8_t kInitialChorusRate = 4;
const uint8_t kInitialChorusDry = 15;
const uint8_t kInitialChorusWet = 15;

struct ChorusTimeData {
    char nameShortR[5];
    float data;
};

const struct ChorusTimeData kChorusTimeDataLibrary[] = {
    {"  10", 0.010}, {"  15", 0.015}, {"  20", 0.020}, {"  25", 0.025}, {"  30", 0.030}, {"  35", 0.035}, {"  40", 0.040}, {"  45", 0.045}, {"  50", 0.050}};

struct ChorusFeedbackData {
    char nameShortR[5];
    float data;
};

const struct ChorusFeedbackData kChorusFeedbackDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

struct ChorusRateData {
    char nameShortR[5];
    float data;
};

const struct ChorusRateData kChorusRateDataLibrary[] = {
    {" 0.1", 0.10}, {" 0.2", 0.20}, {" 0.3", 0.30}, {" 0.4", 0.40}, {" 0.5", 0.50}, {" 0.6", 0.60}, {" 0.7", 0.70}, {" 0.8", 0.80}, {" 0.9", 0.90}, {" 1.0", 1.00}, {" 1.5", 1.50}, {" 2.0", 2.00}, {" 2.5", 2.50}, {" 3.0", 3.00}, {" 3.5", 3.50}, {" 4.0", 4.00}, {" 4.5", 4.50}, {" 5.0", 5.00}, {" 5.5", 5.50}, {" 6.0", 6.00}, {" 6.5", 6.50}, {" 7.0", 7.00}, {" 7.5", 7.50}, {" 8.0", 8.00}, {" 8.5", 8.50}, {" 9.0", 9.00}};

struct ChorusDelay {
    float time;
    float depth;
    float rate;
    float mix;

    uint16_t lag;
    uint16_t playInterval;
    float chorusInterval;

    float shiftFreq;
    float shiftMax;
    float shiftMin;
    float shiftInterval;
    float shiftInc;
};

struct Chorus {
    bool active;

    uint8_t aTime;
    uint8_t bFeedback;
    uint8_t cRate;
    uint8_t dDry;
    uint8_t eWet;

    float time;
    float feedback;
    float rate;

    float depth;
    uint16_t recordInterval;
    const float delayCoef[2] = {1, 0.75};
    ChorusDelay chorusDelay[2];

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialChorusActive;

        aTime = kInitialChorusTime;
        bFeedback = kInitialChorusFeedback;
        cRate = kInitialChorusRate;
        dDry = kInitialChorusDry;
        eWet = kInitialChorusWet;

        time = kChorusTimeDataLibrary[kInitialChorusTime].data;
        feedback = kChorusFeedbackDataLibrary[kInitialChorusFeedback].data;
        rate = kChorusRateDataLibrary[kInitialChorusRate].data;

        depth = 0.80;
        recordInterval = 0;

        for (uint8_t i = 0; i < 2; i++) {
            ChorusDelay &cD = chorusDelay[i];
            cD.time = time * delayCoef[i];
            cD.depth = depth * delayCoef[i];
            cD.rate = rate * delayCoef[i];
            cD.mix = 0.50;

            cD.lag = cD.time * kAudioSampleRate;
            cD.playInterval = (kChorusBufferSize - 1) - cD.lag;

            cD.shiftFreq = cD.rate * 2;
            cD.shiftMax = (cD.depth * cD.lag) - 2;
            cD.shiftMin = -cD.shiftMax;
            cD.shiftInterval = cD.shiftMin;
            cD.shiftInc = ((cD.shiftMax - cD.shiftMin) * cD.shiftFreq) / kAudioSampleRate;
        }
    }

    void update() {
        for (uint8_t i = 0; i < 2; i++) {
            ChorusDelay &cD = chorusDelay[i];
            cD.time = time * delayCoef[i];
            cD.depth = depth * delayCoef[i];
            cD.rate = rate * delayCoef[i];
            cD.mix = 0.50;

            cD.lag = cD.time * kAudioSampleRate;
            int32_t playInterval_ = recordInterval - cD.lag;
            if (playInterval_ < 0)
                playInterval_ += kChorusBufferSize;
            cD.playInterval = playInterval_;

            cD.shiftFreq = cD.rate * 2;
            cD.shiftMax = (cD.depth * cD.lag) - 2;
            cD.shiftMin = -cD.shiftMax;
            cD.shiftInc = ((cD.shiftMax - cD.shiftMin) * cD.shiftFreq) / kAudioSampleRate;
        }
    }

    void cleanMemory() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Flanger Constants --------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint16_t kFlangerBufferSize = 250;

const uint8_t kMinFlangerTime = 0;
const uint8_t kMaxFlangerTime = 8;

const uint8_t kMinFlangerFeedback = 0;
const uint8_t kMaxFlangerFeedback = 19;

const uint8_t kMinFlangerRate = 0;
const uint8_t kMaxFlangerRate = 25;

const uint8_t kMinFlangerDry = 0;
const uint8_t kMaxFlangerDry = 20;

const uint8_t kMinFlangerWet = 0;
const uint8_t kMaxFlangerWet = 20;

const bool kInitialFlangerActive = false;
const uint8_t kInitialFlangerTime = 3;
const uint8_t kInitialFlangerFeedback = 5;
const uint8_t kInitialFlangerRate = 4;
const uint8_t kInitialFlangerDry = 15;
const uint8_t kInitialFlangerWet = 15;

struct FlangerTimeData {
    char nameShortR[5];
    float data;
};

const struct FlangerTimeData kFlangerTimeDataLibrary[] = {
    {" 1.0", 0.0010}, {" 1.5", 0.0015}, {" 2.0", 0.0020}, {" 2.5", 0.0025}, {" 3.0", 0.0030}, {" 3.5", 0.0035}, {" 4.0", 0.0040}, {" 4.5", 0.0045}, {" 5.0", 0.0050}};

struct FlangerFeedbackData {
    char nameShortR[5];
    float data;
};

const struct FlangerFeedbackData kFlangerFeedbackDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

struct FlangerRateData {
    char nameShortR[5];
    float data;
};

const struct FlangerRateData kFlangerRateDataLibrary[] = {
    {" 0.1", 0.10}, {" 0.2", 0.20}, {" 0.3", 0.30}, {" 0.4", 0.40}, {" 0.5", 0.50}, {" 0.6", 0.60}, {" 0.7", 0.70}, {" 0.8", 0.80}, {" 0.9", 0.90}, {" 1.0", 1.00}, {" 1.5", 1.50}, {" 2.0", 2.00}, {" 2.5", 2.50}, {" 3.0", 3.00}, {" 3.5", 3.50}, {" 4.0", 4.00}, {" 4.5", 4.50}, {" 5.0", 5.00}, {" 5.5", 5.50}, {" 6.0", 6.00}, {" 6.5", 6.50}, {" 7.0", 7.00}, {" 7.5", 7.50}, {" 8.0", 8.00}, {" 8.5", 8.50}, {" 9.0", 9.00}};

struct Flanger {
    bool active;

    uint8_t aTime;
    uint8_t bFeedback;
    uint8_t cRate;
    uint8_t dDry;
    uint8_t eWet;

    float time;
    float feedback;
    float rate;

    float depth;
    uint16_t lag;
    uint16_t recordInterval;
    uint16_t playInterval;
    float flangerInterval;
    float shiftInterval;
    float shiftFreq;
    float shiftMin;
    float shiftMax;
    float shiftInc;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialFlangerActive;

        aTime = kInitialFlangerTime;
        bFeedback = kInitialFlangerFeedback;
        cRate = kInitialFlangerRate;
        dDry = kInitialFlangerDry;
        eWet = kInitialFlangerWet;

        time = kFlangerTimeDataLibrary[kInitialFlangerTime].data;
        feedback = kFlangerFeedbackDataLibrary[kInitialFlangerFeedback].data;
        rate = kFlangerRateDataLibrary[kInitialFlangerRate].data;

        depth = 0.90;
        recordInterval = 0;
        lag = time * kAudioSampleRate;
        playInterval = (kFlangerBufferSize - 1) - lag;
        shiftInterval = 0;
        shiftFreq = rate * 2;
        shiftMin = 0;
        shiftMax = (depth * lag) - 2;
        shiftInc = (shiftMax * shiftFreq) / kAudioSampleRate;
    }

    void update() {
        lag = time * kAudioSampleRate;
        int32_t playInterval_ = recordInterval - lag;
        if (playInterval_ < 0)
            playInterval_ += kFlangerBufferSize;
        playInterval = playInterval_;

        shiftFreq = rate * 2;
        shiftMin = 0;
        shiftMax = (depth * lag) - 2;
        shiftInc = (shiftMax * shiftFreq) / kAudioSampleRate;
    }

    void cleanMemory() {
        recordInterval = 0;
        playInterval = (kFlangerBufferSize - 1) - lag;
        shiftInterval = 0;
        shiftInc = (shiftMax * shiftFreq) / kAudioSampleRate;
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Phaser Constants ---------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinPhaserStartFreq = 0;
const uint8_t kMaxPhaserStartFreq = 46;

const uint8_t kMinPhaserEndFreq = 0;
const uint8_t kMaxPhaserEndFreq = 46;

const uint8_t kMinPhaserRate = 0;
const uint8_t kMaxPhaserRate = 25;

const uint8_t kMinPhaserDry = 0;
const uint8_t kMaxPhaserDry = 20;

const uint8_t kMinPhaserWet = 0;
const uint8_t kMaxPhaserWet = 20;

const bool kInitialPhaserActive = false;
const uint8_t kInitialPhaserStartFreq = 27;
const uint8_t kInitialPhaserEndFreq = 31;
const uint8_t kInitialPhaserRate = 4;
const uint8_t kInitialPhaserDry = 15;
const uint8_t kInitialPhaserWet = 15;

struct PhaserFreqData {
    char nameShortR[5];
    uint16_t data;
};

const struct PhaserFreqData kPhaserFreqDataLibrary[] = {
    {"  10", 10}, {"  20", 20}, {"  30", 30}, {"  40", 40}, {"  50", 50}, {"  60", 60}, {"  70", 70}, {"  80", 80}, {"  90", 90}, {" 100", 100}, {" 150", 150}, {" 200", 200}, {" 250", 250}, {" 300", 300}, {" 350", 350}, {" 400", 400}, {" 450", 450}, {" 500", 500}, {" 550", 550}, {" 600", 600}, {" 650", 650}, {" 700", 700}, {" 750", 750}, {" 800", 800}, {" 850", 850}, {" 900", 900}, {" 950", 950}, {"  1K", 1000}, {"  2K", 2000}, {"  3K", 3000}, {"  4K", 4000}, {"  5K", 5000}, {"  6K", 6000}, {"  7K", 7000}, {"  8K", 8000}, {"  9K", 9000}, {" 10K", 10000}, {" 11K", 11000}, {" 12K", 12000}, {" 13K", 13000}, {" 14K", 14000}, {" 15K", 15000}, {" 16K", 16000}, {" 17K", 17000}, {" 18K", 18000}, {" 19K", 19000}, {" 20K", 20000}};

struct PhaserRateData {
    char nameShortR[5];
    float data;
};

const struct PhaserRateData kPhaserRateDataLibrary[] = {
    {" 0.1", 0.10}, {" 0.2", 0.20}, {" 0.3", 0.30}, {" 0.4", 0.40}, {" 0.5", 0.50}, {" 0.6", 0.60}, {" 0.7", 0.70}, {" 0.8", 0.80}, {" 0.9", 0.90}, {" 1.0", 1.00}, {" 1.5", 1.50}, {" 2.0", 2.00}, {" 2.5", 2.50}, {" 3.0", 3.00}, {" 3.5", 3.50}, {" 4.0", 4.00}, {" 4.5", 4.50}, {" 5.0", 5.00}, {" 5.5", 5.50}, {" 6.0", 6.00}, {" 6.5", 6.50}, {" 7.0", 7.00}, {" 7.5", 7.50}, {" 8.0", 8.00}, {" 8.5", 8.50}, {" 9.0", 9.00}};

struct Phaser {
    bool active;

    uint8_t aStartFreq;
    uint8_t bEndFreq;
    uint8_t cRate;
    uint8_t dDry;
    uint8_t eWet;

    uint16_t startFreq;
    uint16_t endFreq;
    float rate;

    float centerFreq;
    float depthFreq;
    float lfo;
    float dataX;
    float dataY;
    float Ts;

    float ff[2] = {0, 0};
    float fb[2] = {0, 0};
    float Q = 0.5;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialPhaserActive;

        aStartFreq = kInitialPhaserStartFreq;
        bEndFreq = kInitialPhaserEndFreq;
        cRate = kInitialPhaserRate;
        dDry = kInitialPhaserDry;
        eWet = kInitialPhaserWet;

        startFreq = kPhaserFreqDataLibrary[kInitialPhaserStartFreq].data;
        endFreq = kPhaserFreqDataLibrary[kInitialPhaserEndFreq].data;
        rate = kPhaserRateDataLibrary[kInitialPhaserRate].data;

        centerFreq = startFreq + ((endFreq - startFreq) / 2.0);
        depthFreq = (endFreq - centerFreq) * 0.9;
        lfo = 0;
        dataX = 0;
        dataY = 0;
        Ts = 1.0f / kAudioSampleRate;
    }

    void update() {
        centerFreq = startFreq + ((endFreq - startFreq) / 2.0);
        depthFreq = (endFreq - centerFreq) * 0.9;
    }

    void cleanMemory() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Compressor Constants -----------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinCompressorThreshold = 0;
const uint8_t kMaxCompressorThreshold = 24;

const uint8_t kMinCompressorRate = 0;
const uint8_t kMaxCompressorRate = 29;

const uint8_t kMinCompressorAttackTime = 0;
const uint8_t kMaxCompressorAttackTime = 18;

const uint8_t kMinCompressorReleaseTime = 0;
const uint8_t kMaxCompressorReleaseTime = 18;

const uint8_t kMinCompressorDry = 0;
const uint8_t kMaxCompressorDry = 20;

const uint8_t kMinCompressorWet = 0;
const uint8_t kMaxCompressorWet = 20;

const bool kInitialCompressorActive = false;
const uint8_t kInitialCompressorThreshold = 18;
const uint8_t kInitialCompressorRate = 3;
const uint8_t kInitialCompressorAttackTime = 0;
const uint8_t kInitialCompressorReleaseTime = 9;
const uint8_t kInitialCompressorMix = 20;

struct CompressorThresholdData {
    char nameShortR[5];
    float data;
};

const struct CompressorThresholdData kCompressorThresholdDataLibrary[] = {
    {" -24", -24.0}, {" -23", -23.0}, {" -22", -22.0}, {" -21", -21.0}, {" -20", -20.0}, {" -19", -19.0}, {" -18", -18.0}, {" -17", -17.0}, {" -16", -16.0}, {" -15", -15.0}, {" -14", -14.0}, {" -13", -13.0}, {" -12", -12.0}, {" -11", -11.0}, {" -10", -10.0}, {" -09", -9.0}, {" -08", -8.0}, {" -07", -7.0}, {" -06", -6.0}, {" -05", -5.0}, {" -04", -4.0}, {" -03", -3.0}, {" -02", -2.0}, {" -01", -1.0}, {"  00", 0.0}};

struct CompressorRateData {
    char nameShortR[5];
    float data;
};

const struct CompressorRateData kCompressorRateDataLibrary[] = {
    {"  01", 1.0}, {"  02", 2.0}, {"  03", 3.0}, {"  04", 4.0}, {"  05", 5.0}, {"  06", 6.0}, {"  07", 7.0}, {"  08", 8.0}, {"  09", 9.0}, {"  10", 10.0}, {"  11", 11.0}, {"  12", 12.0}, {"  13", 13.0}, {"  14", 14.0}, {"  15", 15.0}, {"  16", 16.0}, {"  17", 17.0}, {"  18", 18.0}, {"  19", 19.0}, {"  20", 20.0}, {"  21", 21.0}, {"  22", 22.0}, {"  23", 23.0}, {"  24", 24.0}, {"  25", 25.0}, {"  26", 26.0}, {"  27", 27.0}, {"  28", 28.0}, {"  29", 29.0}, {"  30", 30.0}};

struct CompressorAttackTimeData {
    char nameShortR[5];
    float data;
};

const struct CompressorAttackTimeData kCompressorAttackTimeDataLibrary[] = {
    {"  01", 0.001}, {"  02", 0.002}, {"  03", 0.003}, {"  04", 0.004}, {"  05", 0.005}, {"  06", 0.006}, {"  07", 0.007}, {"  08", 0.008}, {"  09", 0.009}, {"  10", 0.010}, {"  20", 0.020}, {"  30", 0.030}, {"  40", 0.040}, {"  50", 0.050}, {"  60", 0.060}, {"  70", 0.070}, {"  80", 0.080}, {"  90", 0.090}, {" 100", 0.100}};

struct CompressorReleaseTimeData {
    char nameShortR[5];
    float data;
};

const struct CompressorReleaseTimeData kCompressorReleaseTimeDataLibrary[] = {
    {"  10", 0.010}, {"  20", 0.020}, {"  30", 0.030}, {"  40", 0.040}, {"  50", 0.050}, {"  60", 0.060}, {"  70", 0.070}, {"  80", 0.080}, {"  90", 0.090}, {" 100", 0.100}, {" 200", 0.200}, {" 300", 0.300}, {" 400", 0.400}, {" 500", 0.500}, {" 600", 0.600}, {" 700", 0.700}, {" 800", 0.800}, {" 900", 0.900}, {"  1K", 1.000}};

struct Compressor {
    bool active;

    uint8_t aThreshold;
    uint8_t bRate;
    uint8_t cAttackTime;
    uint8_t dReleaseTime;
    uint8_t eMix;

    float threshold;
    float rate;
    float attackTime;
    float releaseTime;

    float attackAlpha;
    float releaseAlpha;
    float gainSmoothPrev;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialCompressorActive;

        aThreshold = kInitialCompressorThreshold;
        bRate = kInitialCompressorRate;
        cAttackTime = kInitialCompressorAttackTime;
        dReleaseTime = kInitialCompressorReleaseTime;
        eMix = kInitialCompressorMix;

        threshold = kCompressorThresholdDataLibrary[kInitialCompressorThreshold].data;
        rate = kCompressorRateDataLibrary[kInitialCompressorRate].data;
        attackTime = kCompressorAttackTimeDataLibrary[kInitialCompressorAttackTime].data;
        releaseTime = kCompressorReleaseTimeDataLibrary[kInitialCompressorReleaseTime].data;

        attackAlpha = exp(-log(9) / (kAudioSampleRate * attackTime));
        releaseAlpha = exp(-log(9) / (kAudioSampleRate * releaseTime));
        gainSmoothPrev = 0;
    }

    void update() {
        attackAlpha = exp(-log(9) / (kAudioSampleRate * attackTime));
        releaseAlpha = exp(-log(9) / (kAudioSampleRate * releaseTime));
    }

    void cleanMemory() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Expander Constants -------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinExpanderThreshold = 0;
const uint8_t kMaxExpanderThreshold = 24;

const uint8_t kMinExpanderRate = 0;
const uint8_t kMaxExpanderRate = 29;

const uint8_t kMinExpanderAttackTime = 0;
const uint8_t kMaxExpanderAttackTime = 18;

const uint8_t kMinExpanderReleaseTime = 0;
const uint8_t kMaxExpanderReleaseTime = 18;

const uint8_t kMinExpanderDry = 0;
const uint8_t kMaxExpanderDry = 20;

const uint8_t kMinExpanderWet = 0;
const uint8_t kMaxExpanderWet = 20;

const bool kInitialExpanderActive = false;
const uint8_t kInitialExpanderThreshold = 12;
const uint8_t kInitialExpanderRate = 3;
const uint8_t kInitialExpanderAttackTime = 0;
const uint8_t kInitialExpanderReleaseTime = 13;
const uint8_t kInitialExpanderMix = 20;

struct ExpanderThresholdData {
    char nameShortR[5];
    float data;
};

const struct ExpanderThresholdData kExpanderThresholdDataLibrary[] = {
    {" -24", -24.0}, {" -23", -23.0}, {" -22", -22.0}, {" -21", -21.0}, {" -20", -20.0}, {" -19", -19.0}, {" -18", -18.0}, {" -17", -17.0}, {" -16", -16.0}, {" -15", -15.0}, {" -14", -14.0}, {" -13", -13.0}, {" -12", -12.0}, {" -11", -11.0}, {" -10", -10.0}, {" -09", -9.0}, {" -08", -8.0}, {" -07", -7.0}, {" -06", -6.0}, {" -05", -5.0}, {" -04", -4.0}, {" -03", -3.0}, {" -02", -2.0}, {" -01", -1.0}, {"  00", 0.0}};

struct ExpanderRateData {
    char nameShortR[5];
    float data;
};

const struct ExpanderRateData kExpanderRateDataLibrary[] = {
    {"  01", 1.0}, {"  02", 2.0}, {"  03", 3.0}, {"  04", 4.0}, {"  05", 5.0}, {"  06", 6.0}, {"  07", 7.0}, {"  08", 8.0}, {"  09", 9.0}, {"  10", 10.0}, {"  11", 11.0}, {"  12", 12.0}, {"  13", 13.0}, {"  14", 14.0}, {"  15", 15.0}, {"  16", 16.0}, {"  17", 17.0}, {"  18", 18.0}, {"  19", 19.0}, {"  20", 20.0}, {"  21", 21.0}, {"  22", 22.0}, {"  23", 23.0}, {"  24", 24.0}, {"  25", 25.0}, {"  26", 26.0}, {"  27", 27.0}, {"  28", 28.0}, {"  29", 29.0}, {"  30", 30.0}};

struct ExpanderAttackTimeData {
    char nameShortR[5];
    float data;
};

const struct ExpanderAttackTimeData kExpanderAttackTimeDataLibrary[] = {
    {"  01", 0.001}, {"  02", 0.002}, {"  03", 0.003}, {"  04", 0.004}, {"  05", 0.005}, {"  06", 0.006}, {"  07", 0.007}, {"  08", 0.008}, {"  09", 0.009}, {"  10", 0.010}, {"  20", 0.020}, {"  30", 0.030}, {"  40", 0.040}, {"  50", 0.050}, {"  60", 0.060}, {"  70", 0.070}, {"  80", 0.080}, {"  90", 0.090}, {" 100", 0.100}};

struct ExpanderReleaseTimeData {
    char nameShortR[5];
    float data;
};

const struct ExpanderReleaseTimeData kExpanderReleaseTimeDataLibrary[] = {
    {"  10", 0.010}, {"  20", 0.020}, {"  30", 0.030}, {"  40", 0.040}, {"  50", 0.050}, {"  60", 0.060}, {"  70", 0.070}, {"  80", 0.080}, {"  90", 0.090}, {" 100", 0.100}, {" 200", 0.200}, {" 300", 0.300}, {" 400", 0.400}, {" 500", 0.500}, {" 600", 0.600}, {" 700", 0.700}, {" 800", 0.800}, {" 900", 0.900}, {"  1K", 1.000}};

struct Expander {
    bool active;

    uint8_t aThreshold;
    uint8_t bRate;
    uint8_t cAttackTime;
    uint8_t dReleaseTime;
    uint8_t eMix;

    float threshold;
    float rate;
    float attackTime;
    float releaseTime;

    float attackAlpha;
    float releaseAlpha;
    float gainSmoothPrev;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialExpanderActive;

        aThreshold = kInitialExpanderThreshold;
        bRate = kInitialExpanderRate;
        cAttackTime = kInitialExpanderAttackTime;
        dReleaseTime = kInitialExpanderReleaseTime;
        eMix = kInitialExpanderMix;

        threshold = kExpanderThresholdDataLibrary[kInitialExpanderThreshold].data;
        rate = kExpanderRateDataLibrary[kInitialExpanderRate].data;
        attackTime = kExpanderAttackTimeDataLibrary[kInitialExpanderAttackTime].data;
        releaseTime = kExpanderReleaseTimeDataLibrary[kInitialExpanderReleaseTime].data;

        attackAlpha = exp(-log(9) / (kAudioSampleRate * attackTime));
        releaseAlpha = exp(-log(9) / (kAudioSampleRate * releaseTime));
        gainSmoothPrev = -144;
    }

    void update() {
        attackAlpha = exp(-log(9) / (kAudioSampleRate * attackTime));
        releaseAlpha = exp(-log(9) / (kAudioSampleRate * releaseTime));
    }

    void cleanMemory() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Overdrive Constants ------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinOverdriveGain = 0;
const uint8_t kMaxOverdriveGain = 24;

const uint8_t kMinOverdriveThreshold = 0;
const uint8_t kMaxOverdriveThreshold = 24;

const uint8_t kMinOverdriveTone = 0;
const uint8_t kMaxOverdriveTone = 46;

const uint8_t kMinOverdriveDry = 0;
const uint8_t kMaxOverdriveDry = 20;

const uint8_t kMinOverdriveWet = 0;
const uint8_t kMaxOverdriveWet = 20;

const bool kInitialOverdriveActive = false;
const uint8_t kInitialOverdriveGain = 15;
const uint8_t kInitialOverdriveThreshold = 18;
const uint8_t kInitialOverdriveTone = 31;
const uint8_t kInitialOverdriveDry = 5;
const uint8_t kInitialOverdriveWet = 15;

struct OverdriveGainData {
    char nameShortR[5];
    float data;
};

const struct OverdriveGainData kOverdriveGainDataLibrary[] = {
    {"  00", 0.0}, {"  01", 1.0}, {"  02", 2.0}, {"  03", 3.0}, {"  04", 4.0}, {"  05", 5.0}, {"  06", 6.0}, {"  07", 7.0}, {"  08", 8.0}, {"  09", 9.0}, {"  10", 10.0}, {"  11", 11.0}, {"  12", 12.0}, {"  13", 13.0}, {"  14", 14.0}, {"  15", 15.0}, {"  16", 16.0}, {"  17", 17.0}, {"  18", 18.0}, {"  19", 19.0}, {"  20", 20.0}, {"  21", 21.0}, {"  22", 22.0}, {"  23", 23.0}, {"  24", 24.0}};

struct OverdriveThresholdData {
    char nameShortR[5];
    float data;
};

const struct OverdriveThresholdData kOverdriveThresholdDataLibrary[] = {
    {" -24", -24.0}, {" -23", -23.0}, {" -22", -22.0}, {" -21", -21.0}, {" -20", -20.0}, {" -19", -19.0}, {" -18", -18.0}, {" -17", -17.0}, {" -16", -16.0}, {" -15", -15.0}, {" -14", -14.0}, {" -13", -13.0}, {" -12", -12.0}, {" -11", -11.0}, {" -10", -10.0}, {" -09", -9.0}, {" -08", -8.0}, {" -07", -7.0}, {" -06", -6.0}, {" -05", -5.0}, {" -04", -4.0}, {" -03", -3.0}, {" -02", -2.0}, {" -01", -1.0}, {"  00", 0.0}};

struct OverdriveToneData {
    char nameShortR[5];
    float data;
};

const struct OverdriveToneData kOverdriveToneDataLibrary[] = {
    {"  10", 10}, {"  20", 20}, {"  30", 30}, {"  40", 40}, {"  50", 50}, {"  60", 60}, {"  70", 70}, {"  80", 80}, {"  90", 90}, {" 100", 100}, {" 150", 150}, {" 200", 200}, {" 250", 250}, {" 300", 300}, {" 350", 350}, {" 400", 400}, {" 450", 450}, {" 500", 500}, {" 550", 550}, {" 600", 600}, {" 650", 650}, {" 700", 700}, {" 750", 750}, {" 800", 800}, {" 850", 850}, {" 900", 900}, {" 950", 950}, {"  1K", 1000}, {"  2K", 2000}, {"  3K", 3000}, {"  4K", 4000}, {"  5K", 5000}, {"  6K", 6000}, {"  7K", 7000}, {"  8K", 8000}, {"  9K", 9000}, {" 10K", 10000}, {" 11K", 11000}, {" 12K", 12000}, {" 13K", 13000}, {" 14K", 14000}, {" 15K", 15000}, {" 16K", 16000}, {" 7K", 17000}, {" 18K", 18000}, {" 19K", 19000}, {" 20K", 20000}};

struct Overdrive {
    bool active;

    uint8_t aGain;
    uint8_t bThreshold;
    uint8_t cTone;
    uint8_t dDry;
    uint8_t eWet;

    float gaindB;
    float gain;
    float thresholddB;
    float threshold;
    float tone;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    // filter data

    float a0;
    float a1;
    float a2;
    float b1;
    float b2;

    int32_t dataIn[3] = {0};
    int32_t dataOut[3] = {0};

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialOverdriveActive;

        aGain = kInitialOverdriveGain;
        bThreshold = kInitialOverdriveThreshold;
        cTone = kInitialOverdriveTone;
        dDry = kInitialOverdriveDry;
        eWet = kInitialOverdriveWet;

        gaindB = kOverdriveGainDataLibrary[kInitialOverdriveGain].data;
        thresholddB = kOverdriveThresholdDataLibrary[kInitialOverdriveThreshold].data;
        tone = kOverdriveToneDataLibrary[kInitialOverdriveTone].data;

        gain = pow(10.0f, gaindB / 20.0f);
        threshold = pow(10.0f, thresholddB / 20.0f);

        calculateFilterCoef();
    }

    void update() {
        gain = pow(10.0f, gaindB / 20.0f);
        threshold = pow(10.0f, thresholddB / 20.0f);

        calculateFilterCoef();
    }

    void calculateFilterCoef() {
        float Q = 0.707;
        float norm;
        float K = tan(M_PI * tone / kAudioSampleRate);

        norm = 1.0f / (1.0f + K / Q + K * K);
        a0 = K * K * norm;
        a1 = 2.0f * a0;
        a2 = a0;
        b1 = 2.0f * (K * K - 1.0f) * norm;
        b2 = (1.0f - K / Q + K * K) * norm;
    }

    void cleanMemory() {
        dataIn[0] = 0;
        dataIn[1] = 0;
        dataIn[2] = 0;
        dataOut[0] = 0;
        dataOut[1] = 0;
        dataOut[2] = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Distortion Constants -----------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinDistortionGain = 0;
const uint8_t kMaxDistortionGain = 24;

const uint8_t kMinDistortionThreshold = 0;
const uint8_t kMaxDistortionThreshold = 24;

const uint8_t kMinDistortionTone = 0;
const uint8_t kMaxDistortionTone = 46;

const uint8_t kMinDistortionDry = 0;
const uint8_t kMaxDistortionDry = 20;

const uint8_t kMinDistortionWet = 0;
const uint8_t kMaxDistortionWet = 20;

const bool kInitialDistortionActive = false;
const uint8_t kInitialDistortionGain = 15;
const uint8_t kInitialDistortionThreshold = 18;
const uint8_t kInitialDistortionTone = 31;
const uint8_t kInitialDistortionDry = 5;
const uint8_t kInitialDistortionWet = 15;

struct DistortionGainData {
    char nameShortR[5];
    float data;
};

const struct DistortionGainData kDistortionGainDataLibrary[] = {
    {"  00", 0.0}, {"  01", 1.0}, {"  02", 2.0}, {"  03", 3.0}, {"  04", 4.0}, {"  05", 5.0}, {"  06", 6.0}, {"  07", 7.0}, {"  08", 8.0}, {"  09", 9.0}, {"  10", 10.0}, {"  11", 11.0}, {"  12", 12.0}, {"  13", 13.0}, {"  14", 14.0}, {"  15", 15.0}, {"  16", 16.0}, {"  17", 17.0}, {"  18", 18.0}, {"  19", 19.0}, {"  20", 20.0}, {"  21", 21.0}, {"  22", 22.0}, {"  23", 23.0}, {"  24", 24.0}};

struct DistortionThresholdData {
    char nameShortR[5];
    float data;
};

const struct DistortionThresholdData kDistortionThresholdDataLibrary[] = {
    {" -24", -24.0}, {" -23", -23.0}, {" -22", -22.0}, {" -21", -21.0}, {" -20", -20.0}, {" -19", -19.0}, {" -18", -18.0}, {" -17", -17.0}, {" -16", -16.0}, {" -15", -15.0}, {" -14", -14.0}, {" -13", -13.0}, {" -12", -12.0}, {" -11", -11.0}, {" -10", -10.0}, {" -09", -9.0}, {" -08", -8.0}, {" -07", -7.0}, {" -06", -6.0}, {" -05", -5.0}, {" -04", -4.0}, {" -03", -3.0}, {" -02", -2.0}, {" -01", -1.0}, {"  00", 0.0}};

struct DistortionToneData {
    char nameShortR[5];
    float data;
};

const struct DistortionToneData kDistortionToneDataLibrary[] = {
    {"  10", 10}, {"  20", 20}, {"  30", 30}, {"  40", 40}, {"  50", 50}, {"  60", 60}, {"  70", 70}, {"  80", 80}, {"  90", 90}, {" 100", 100}, {" 150", 150}, {" 200", 200}, {" 250", 250}, {" 300", 300}, {" 350", 350}, {" 400", 400}, {" 450", 450}, {" 500", 500}, {" 550", 550}, {" 600", 600}, {" 650", 650}, {" 700", 700}, {" 750", 750}, {" 800", 800}, {" 850", 850}, {" 900", 900}, {" 950", 950}, {"  1K", 1000}, {"  2K", 2000}, {"  3K", 3000}, {"  4K", 4000}, {"  5K", 5000}, {"  6K", 6000}, {"  7K", 7000}, {"  8K", 8000}, {"  9K", 9000}, {" 10K", 10000}, {" 11K", 11000}, {" 12K", 12000}, {" 13K", 13000}, {" 14K", 14000}, {" 15K", 15000}, {" 16K", 16000}, {" 17K", 17000}, {" 18K", 18000}, {" 19K", 19000}, {" 20K", 20000}};

struct Distortion {
    bool active;

    uint8_t aGain;
    uint8_t bThreshold;
    uint8_t cTone;
    uint8_t dDry;
    uint8_t eWet;

    float gaindB;
    float gain;
    float thresholddB;
    float threshold;
    float tone;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    // filter data

    float a0;
    float a1;
    float a2;
    float b1;
    float b2;

    int32_t dataIn[3] = {0};
    int32_t dataOut[3] = {0};

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialDistortionActive;

        aGain = kInitialDistortionGain;
        bThreshold = kInitialDistortionThreshold;
        cTone = kInitialDistortionTone;
        dDry = kInitialDistortionDry;
        eWet = kInitialDistortionWet;

        gaindB = kDistortionGainDataLibrary[kInitialDistortionGain].data;
        thresholddB = kDistortionThresholdDataLibrary[kInitialDistortionThreshold].data;
        tone = kDistortionToneDataLibrary[kInitialDistortionTone].data;

        gain = pow(10.0f, gaindB / 20.0f);
        threshold = pow(10.0f, thresholddB / 20.0f);

        calculateFilterCoef();
    }

    void update() {
        gain = pow(10.0f, gaindB / 20.0f);
        threshold = pow(10.0f, thresholddB / 20.0f);

        calculateFilterCoef();
    }

    void calculateFilterCoef() {
        float Q = 0.707;
        float norm;
        float K = tan(M_PI * tone / kAudioSampleRate);

        norm = 1.0f / (1.0f + K / Q + K * K);
        a0 = K * K * norm;
        a1 = 2.0f * a0;
        a2 = a0;
        b1 = 2.0f * (K * K - 1.0f) * norm;
        b2 = (1.0f - K / Q + K * K) * norm;
    }

    void cleanMemory() {
        dataIn[0] = 0;
        dataIn[1] = 0;
        dataIn[2] = 0;
        dataOut[0] = 0;
        dataOut[1] = 0;
        dataOut[2] = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Effect-Bitcrusher Constants -----------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinBitcrusherResolution = 0;
const uint8_t kMaxBitcrusherResolution = 23;

const uint8_t kMinBitcrusherSampleRate = 0;
const uint8_t kMaxBitcrusherSampleRate = 23;

const uint8_t kMinBitcrusherThreshold = 0;
const uint8_t kMaxBitcrusherThreshold = 24;

const uint8_t kMinBitcrusherDry = 0;
const uint8_t kMaxBitcrusherDry = 20;

const uint8_t kMinBitcrusherWet = 0;
const uint8_t kMaxBitcrusherWet = 20;

const bool kInitialBitcrusherActive = false;
const uint8_t kInitialBitcrusherResolution = 7;
const uint8_t kInitialBitcrusherSampleRate = 0;
const uint8_t kInitialBitcrusherThreshold = 24;
const uint8_t kInitialBitcrusherDry = 5;
const uint8_t kInitialBitcrusherWet = 15;

struct BitcrusherResolutionData {
    char nameShortR[5];
    uint8_t data;
};

const struct BitcrusherResolutionData kBitcrusherResolutionDataLibrary[] = {
    {"  01", 1}, {"  02", 2}, {"  03", 3}, {"  04", 4}, {"  05", 5}, {"  06", 6}, {"  07", 7}, {"  08", 8}, {"  09", 9}, {"  10", 10}, {"  11", 11}, {"  12", 12}, {"  13", 13}, {"  14", 14}, {"  15", 15}, {"  16", 16}, {"  17", 17}, {"  18", 18}, {"  19", 19}, {"  20", 20}, {"  21", 21}, {"  22", 22}, {"  23", 23}, {"  24", 24}};

struct BitcrusherSampleRateData {
    char nameShortR[5];
    uint8_t data;
};

const struct BitcrusherSampleRateData kBitcrusherSampleRateDataLibrary[] = {
    {"  01", 1}, {"  02", 2}, {"  03", 3}, {"  04", 4}, {"  05", 5}, {"  06", 6}, {"  07", 7}, {"  08", 8}, {"  09", 9}, {"  10", 10}, {"  11", 11}, {"  12", 12}, {"  13", 13}, {"  14", 14}, {"  15", 15}, {"  16", 16}, {"  17", 17}, {"  18", 18}, {"  19", 19}, {"  20", 20}, {"  21", 21}, {"  22", 22}, {"  23", 23}, {"  24", 24}};

struct BitcrusherThresholdData {
    char nameShortR[5];
    float data;
};

const struct BitcrusherThresholdData kBitcrusherThresholdDataLibrary[] = {
    {" -24", -24.0}, {" -23", -23.0}, {" -22", -22.0}, {" -21", -21.0}, {" -20", -20.0}, {" -19", -19.0}, {" -18", -18.0}, {" -17", -17.0}, {" -16", -16.0}, {" -15", -15.0}, {" -14", -14.0}, {" -13", -13.0}, {" -12", -12.0}, {" -11", -11.0}, {" -10", -10.0}, {" -09", -9.0}, {" -08", -8.0}, {" -07", -7.0}, {" -06", -6.0}, {" -05", -5.0}, {" -04", -4.0}, {" -03", -3.0}, {" -02", -2.0}, {" -01", -1.0}, {"  00", 0.0}};

typedef enum {
    BIT_CLIP = 0x00,
    BIT_FOLD = 0x01,
} BitcrusherMode;

struct Bitcrusher {
    bool active;

    uint8_t aResolution;
    uint8_t bSampleRate;
    uint8_t cThreshold;
    uint8_t dDry;
    uint8_t eWet;

    uint8_t resolution;
    uint8_t sampleRate;
    int8_t threshold;

    BitcrusherMode mode;

    uint16_t sampleCounter;
    uint16_t sampleCounterMax;
    uint32_t resModifier;
    float limitMultiplier;
    int32_t limitPos;
    int32_t limitNeg;
    int32_t sampleData;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    void initialize() {
        reset();
    }

    void reset() {
        active = kInitialBitcrusherActive;

        aResolution = kInitialBitcrusherResolution;
        bSampleRate = kInitialBitcrusherSampleRate;
        cThreshold = kInitialBitcrusherThreshold;
        dDry = kInitialBitcrusherDry;
        eWet = kInitialBitcrusherWet;

        mode = BIT_FOLD;

        resolution = kBitcrusherResolutionDataLibrary[kInitialBitcrusherResolution].data;
        sampleRate = kBitcrusherSampleRateDataLibrary[kInitialBitcrusherSampleRate].data;
        threshold = kBitcrusherThresholdDataLibrary[kInitialBitcrusherThreshold].data;

        sampleCounter = 0;
        sampleCounterMax = sampleRate;

        resModifier = 0;

        for (uint8_t i = 0; i < resolution; i++) {
            resModifier <<= 1;
            resModifier += 1;
        }

        for (uint8_t j = 0; j < (24 - resolution); j++) {
            resModifier <<= 1;
        }

        resModifier += 0xFF000000;

        limitMultiplier = pow(10.0, threshold / 20.0);
        limitPos = limitMultiplier * INT24_MAX;
        limitNeg = -limitPos;

        sampleData = 0;
    }

    void update() {
        sampleCounterMax = sampleRate;

        resModifier = 0;

        for (uint8_t i = 0; i < resolution; i++) {
            resModifier <<= 1;
            resModifier += 1;
        }

        for (uint8_t j = 0; j < (24 - resolution); j++) {
            resModifier <<= 1;
        }

        resModifier += 0xFF000000;

        limitMultiplier = pow(10.0, threshold / 20.0);
        limitPos = limitMultiplier * INT24_MAX;
        limitNeg = -limitPos;
    }

    void cleanMemory() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Effect Constants ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinEffectType = 0;
const uint8_t kMaxEffectType = 8;

const bool kInitialEffectActive = false;
const uint8_t kInitialEffectType = 0;

typedef enum {
    EF_DELAY = 0x00,
    EF_CHORUS = 0x01,
    EF_FLANGER = 0x02,
    EF_PHASER = 0x03,
    EF_COMPRESSOR = 0x04,
    EF_EXPANDER = 0x05,
    EF_OVERDRIVE = 0x06,
    EF_DISTORTION = 0x07,
    EF_BITCRUSHER = 0x08
} EffectType;

struct EffectTypeData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct EffectTypeData kEffectTypeDataLibrary[] = {
    {"DEL ", " DEL"}, {"CHO ", " CHO"}, {"FLA ", " FLA"}, {"PHA ", " PHA"}, {"COM ", " COM"}, {"EXP ", " EXP"}, {"OVE ", " OVE"}, {"DIS ", " DIS"}, {"BIT ", " BIT"}};

struct EffectMixData {
    char nameShortR[5];
    float data;
};

const struct EffectMixData kEffectMixDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

const uint8_t kEffectLibrarySize = 2;
const uint8_t kSubEffectLibrarySize = 9;

const char aHeader[kSubEffectLibrarySize][5] = {"TIME", "TIME", "TIME", "STAR", "THRE", "THRE", "GAIN", "GAIN", "RESO"};
const char bHeader[kSubEffectLibrarySize][5] = {"LEV ", "FEED", "FEED", "END ", "RATE", "RATE", "THRE", "THRE", "SAMP"};
const char cHeader[kSubEffectLibrarySize][5] = {"FEED", "RATE", "RATE", "RATE", "ATT ", "ATT ", "TONE", "TONE", "THRE"};
const char dHeader[kSubEffectLibrarySize][5] = {"DRY ", "DRY ", "DRY ", "DRY ", "REL ", "REL ", "DRY ", "DRY ", "DRY "};
const char eHeader[kSubEffectLibrarySize][5] = {"WET ", "WET ", "WET ", "WET ", "MIX ", "MIX ", "WET ", "WET ", "WET "};

const char aSign[kSubEffectLibrarySize][4] = {"  %", " MS", " MS", " HZ", " DB", " DB", " DB", " DB", " BR"};
const char bSign[kSubEffectLibrarySize][4] = {"  %", "  %", "  %", " HZ", "  X", "  X", " DB", " DB", "  %"};
const char cSign[kSubEffectLibrarySize][4] = {"  %", " HZ", " HZ", " HZ", " MS", " MS", " HZ", " HZ", " DB"};
const char dSign[kSubEffectLibrarySize][4] = {"  %", "  %", "  %", "  %", " MS", " MS", "  %", "  %", "  %"};
const char eSign[kSubEffectLibrarySize][4] = {"  %", "  %", "  %", "  %", "  %", "  %", "  %", "  %", "  %"};

struct SubEffect {
    bool activeA;
    bool activeB;
    bool activeC;
    bool activeD;
    bool activeE;

    uint8_t kMinAData;
    uint8_t kMaxAData;

    uint8_t kMinBData;
    uint8_t kMaxBData;

    uint8_t kMinCData;
    uint8_t kMaxCData;

    uint8_t kMinDData;
    uint8_t kMaxDData;

    uint8_t kMinEData;
    uint8_t kMaxEData;

    uint8_t kInitialAData;
    uint8_t kInitialBData;
    uint8_t kInitialCData;
    uint8_t kInitialDData;
    uint8_t kInitialEData;

    uint8_t aData;
    uint8_t bData;
    uint8_t cData;
    uint8_t dData;
    uint8_t eData;
};

typedef enum {
    EF_ACTION_NONE = 0x00,
    EF_ACTION_UP = 0x01,
    EF_ACTION_DOWN = 0x02
} EffectTransitionAction;

typedef enum {
    EF_MODE_NONE = 0x00,
    EF_MODE_ACTIVE = 0x01,
    EF_MODE_TYPE = 0x02,
    EF_MODE_TIME = 0x03,
    EF_MODE_FEEDBACK = 0x04
} EffectTransitionMode;

typedef enum {
    EF_PHASE_NONE = 0x00,
    EF_PHASE_A = 0x01,
    EF_PHASE_B = 0x02
} EffectTransitionPhase;

struct EffectGenTransition {
    bool active;

    EffectTransitionMode mode;
    EffectTransitionPhase phase;

    bool activeActive;
    bool targetActive;

    uint8_t activeType;
    uint8_t targetType;

    float activeDry;
    float targetDry;
    EffectTransitionAction actionDry;

    float activeWet;
    float targetWet;
    EffectTransitionAction actionWet;

    float activeRecordWet;
    float targetRecordWet;
    EffectTransitionAction actionRecordWet;
};

struct EffectMixTransition {
    bool active;

    float targetDry;
    EffectTransitionAction actionDry;

    float targetWet;
    EffectTransitionAction actionWet;
};

struct Effect {
    uint8_t num;
    bool active;
    uint8_t type;

    float dryFloat;
    float wetFloat;

    float recordWetFloat;

    SubEffect subEffect[kSubEffectLibrarySize];

    EffectGenTransition genTransition;
    EffectMixTransition mixTransition;

    Delay delay;
    Chorus chorus;
    Flanger flanger;
    Phaser phaser;
    Compressor compressor;
    Expander expander;
    Overdrive overdrive;
    Distortion distortion;
    Bitcrusher bitcrusher;

    uint32_t delayAddress;
    uint32_t chorusAddress;
    int32_t flangerBuffer[kFlangerBufferSize];

    void initialize() {
        switch (num) {
        case 0:
            delayAddress = RAM_DELAY_0;
            chorusAddress = RAM_CHORUS_0;
            break;

        case 1:
            delayAddress = RAM_DELAY_1;
            chorusAddress = RAM_CHORUS_1;
            break;
        }

        reset();
    }

    void reset() {
        active = kInitialEffectActive;
        type = kInitialEffectType;

        // delay
        subEffect[0].activeA = true;
        subEffect[0].activeB = true;
        subEffect[0].activeC = true;
        subEffect[0].activeD = true;
        subEffect[0].activeE = true;

        subEffect[0].kMinAData = kMinDelayTime;
        subEffect[0].kMaxAData = kMaxDelayTime;

        subEffect[0].kMinBData = kMinDelayLevel;
        subEffect[0].kMaxBData = kMaxDelayLevel;

        subEffect[0].kMinCData = kMinDelayFeedback;
        subEffect[0].kMaxCData = kMaxDelayFeedback;

        subEffect[0].kMinDData = kMinDelayDry;
        subEffect[0].kMaxDData = kMaxDelayDry;

        subEffect[0].kMinEData = kMinDelayWet;
        subEffect[0].kMaxEData = kMaxDelayWet;

        subEffect[0].kInitialAData = kInitialDelayTime;
        subEffect[0].kInitialBData = kInitialDelayLevel;
        subEffect[0].kInitialCData = kInitialDelayFeedback;
        subEffect[0].kInitialDData = kInitialDelayDry;
        subEffect[0].kInitialEData = kInitialDelayWet;

        subEffect[0].aData = subEffect[0].kInitialAData;
        subEffect[0].bData = subEffect[0].kInitialBData;
        subEffect[0].cData = subEffect[0].kInitialCData;
        subEffect[0].dData = subEffect[0].kInitialDData;
        subEffect[0].eData = subEffect[0].kInitialEData;

        // chorus
        subEffect[1].activeA = true;
        subEffect[1].activeB = true;
        subEffect[1].activeC = true;
        subEffect[1].activeD = true;
        subEffect[1].activeE = true;

        subEffect[1].kMinAData = kMinChorusTime;
        subEffect[1].kMaxAData = kMaxChorusTime;

        subEffect[1].kMinBData = kMinChorusFeedback;
        subEffect[1].kMaxBData = kMaxChorusFeedback;

        subEffect[1].kMinCData = kMinChorusRate;
        subEffect[1].kMaxCData = kMaxChorusRate;

        subEffect[1].kMinDData = kMinChorusDry;
        subEffect[1].kMaxDData = kMaxChorusDry;

        subEffect[1].kMinEData = kMinChorusWet;
        subEffect[1].kMaxEData = kMaxChorusWet;

        subEffect[1].kInitialAData = kInitialChorusTime;
        subEffect[1].kInitialBData = kInitialChorusFeedback;
        subEffect[1].kInitialCData = kInitialChorusRate;
        subEffect[1].kInitialDData = kInitialChorusDry;
        subEffect[1].kInitialEData = kInitialChorusWet;

        subEffect[1].aData = subEffect[1].kInitialAData;
        subEffect[1].bData = subEffect[1].kInitialBData;
        subEffect[1].cData = subEffect[1].kInitialCData;
        subEffect[1].dData = subEffect[1].kInitialDData;
        subEffect[1].eData = subEffect[1].kInitialEData;

        // flanger
        subEffect[2].activeA = true;
        subEffect[2].activeB = true;
        subEffect[2].activeC = true;
        subEffect[2].activeD = true;
        subEffect[2].activeE = true;

        subEffect[2].kMinAData = kMinFlangerTime;
        subEffect[2].kMaxAData = kMaxFlangerTime;

        subEffect[2].kMinBData = kMinFlangerFeedback;
        subEffect[2].kMaxBData = kMaxFlangerFeedback;

        subEffect[2].kMinCData = kMinFlangerRate;
        subEffect[2].kMaxCData = kMaxFlangerRate;

        subEffect[2].kMinDData = kMinFlangerDry;
        subEffect[2].kMaxDData = kMaxFlangerDry;

        subEffect[2].kMinEData = kMinFlangerWet;
        subEffect[2].kMaxEData = kMaxFlangerWet;

        subEffect[2].kInitialAData = kInitialFlangerTime;
        subEffect[2].kInitialBData = kInitialFlangerFeedback;
        subEffect[2].kInitialCData = kInitialFlangerRate;
        subEffect[2].kInitialDData = kInitialFlangerDry;
        subEffect[2].kInitialEData = kInitialFlangerWet;

        subEffect[2].aData = subEffect[2].kInitialAData;
        subEffect[2].bData = subEffect[2].kInitialBData;
        subEffect[2].cData = subEffect[2].kInitialCData;
        subEffect[2].dData = subEffect[2].kInitialDData;
        subEffect[2].eData = subEffect[2].kInitialEData;

        // phaser
        subEffect[3].activeA = true;
        subEffect[3].activeB = true;
        subEffect[3].activeC = true;
        subEffect[3].activeD = true;
        subEffect[3].activeE = true;

        subEffect[3].kMinAData = kMinPhaserStartFreq;
        subEffect[3].kMaxAData = kMaxPhaserStartFreq;

        subEffect[3].kMinBData = kMinPhaserEndFreq;
        subEffect[3].kMaxBData = kMaxPhaserEndFreq;

        subEffect[3].kMinCData = kMinPhaserRate;
        subEffect[3].kMaxCData = kMaxPhaserRate;

        subEffect[3].kMinDData = kMinPhaserDry;
        subEffect[3].kMaxDData = kMaxPhaserDry;

        subEffect[3].kMinEData = kMinPhaserWet;
        subEffect[3].kMaxEData = kMaxPhaserWet;

        subEffect[3].kInitialAData = kInitialPhaserStartFreq;
        subEffect[3].kInitialBData = kInitialPhaserEndFreq;
        subEffect[3].kInitialCData = kInitialPhaserRate;
        subEffect[3].kInitialDData = kInitialPhaserDry;
        subEffect[3].kInitialEData = kInitialPhaserWet;

        subEffect[3].aData = subEffect[3].kInitialAData;
        subEffect[3].bData = subEffect[3].kInitialBData;
        subEffect[3].cData = subEffect[3].kInitialCData;
        subEffect[3].dData = subEffect[3].kInitialDData;
        subEffect[3].eData = subEffect[3].kInitialEData;

        // compressor
        subEffect[4].activeA = true;
        subEffect[4].activeB = true;
        subEffect[4].activeC = true;
        subEffect[4].activeD = true;
        subEffect[4].activeE = true;

        subEffect[4].kMinAData = kMinCompressorThreshold;
        subEffect[4].kMaxAData = kMaxCompressorThreshold;

        subEffect[4].kMinBData = kMinCompressorRate;
        subEffect[4].kMaxBData = kMaxCompressorRate;

        subEffect[4].kMinCData = kMinCompressorAttackTime;
        subEffect[4].kMaxCData = kMaxCompressorAttackTime;

        subEffect[4].kMinDData = kMinCompressorReleaseTime;
        subEffect[4].kMaxDData = kMaxCompressorReleaseTime;

        subEffect[4].kMinEData = kMinCompressorWet;
        subEffect[4].kMaxEData = kMaxCompressorWet;

        subEffect[4].kInitialAData = kInitialCompressorThreshold;
        subEffect[4].kInitialBData = kInitialCompressorRate;
        subEffect[4].kInitialCData = kInitialCompressorAttackTime;
        subEffect[4].kInitialDData = kInitialCompressorReleaseTime;
        subEffect[4].kInitialEData = kInitialCompressorMix;

        subEffect[4].aData = subEffect[4].kInitialAData;
        subEffect[4].bData = subEffect[4].kInitialBData;
        subEffect[4].cData = subEffect[4].kInitialCData;
        subEffect[4].dData = subEffect[4].kInitialDData;
        subEffect[4].eData = subEffect[4].kInitialEData;

        // expander
        subEffect[5].activeA = true;
        subEffect[5].activeB = true;
        subEffect[5].activeC = true;
        subEffect[5].activeD = true;
        subEffect[5].activeE = true;

        subEffect[5].kMinAData = kMinExpanderThreshold;
        subEffect[5].kMaxAData = kMaxExpanderThreshold;

        subEffect[5].kMinBData = kMinExpanderRate;
        subEffect[5].kMaxBData = kMaxExpanderRate;

        subEffect[5].kMinCData = kMinExpanderAttackTime;
        subEffect[5].kMaxCData = kMaxExpanderAttackTime;

        subEffect[5].kMinDData = kMinExpanderReleaseTime;
        subEffect[5].kMaxDData = kMaxExpanderReleaseTime;

        subEffect[5].kMinEData = kMinExpanderWet;
        subEffect[5].kMaxEData = kMaxExpanderWet;

        subEffect[5].kInitialAData = kInitialExpanderThreshold;
        subEffect[5].kInitialBData = kInitialExpanderRate;
        subEffect[5].kInitialCData = kInitialExpanderAttackTime;
        subEffect[5].kInitialDData = kInitialExpanderReleaseTime;
        subEffect[5].kInitialEData = kInitialExpanderMix;

        subEffect[5].aData = subEffect[5].kInitialAData;
        subEffect[5].bData = subEffect[5].kInitialBData;
        subEffect[5].cData = subEffect[5].kInitialCData;
        subEffect[5].dData = subEffect[5].kInitialDData;
        subEffect[5].eData = subEffect[5].kInitialEData;

        // overdrive
        subEffect[6].activeA = true;
        subEffect[6].activeB = true;
        subEffect[6].activeC = true;
        subEffect[6].activeD = true;
        subEffect[6].activeE = true;

        subEffect[6].kMinAData = kMinOverdriveGain;
        subEffect[6].kMaxAData = kMaxOverdriveGain;

        subEffect[6].kMinBData = kMinOverdriveThreshold;
        subEffect[6].kMaxBData = kMaxOverdriveThreshold;

        subEffect[6].kMinCData = kMinOverdriveTone;
        subEffect[6].kMaxCData = kMaxOverdriveTone;

        subEffect[6].kMinDData = kMinOverdriveDry;
        subEffect[6].kMaxDData = kMaxOverdriveDry;

        subEffect[6].kMinEData = kMinOverdriveWet;
        subEffect[6].kMaxEData = kMaxOverdriveWet;

        subEffect[6].kInitialAData = kInitialOverdriveGain;
        subEffect[6].kInitialBData = kInitialOverdriveThreshold;
        subEffect[6].kInitialCData = kInitialOverdriveTone;
        subEffect[6].kInitialDData = kInitialOverdriveDry;
        subEffect[6].kInitialEData = kInitialOverdriveWet;

        subEffect[6].aData = subEffect[6].kInitialAData;
        subEffect[6].bData = subEffect[6].kInitialBData;
        subEffect[6].cData = subEffect[6].kInitialCData;
        subEffect[6].dData = subEffect[6].kInitialDData;
        subEffect[6].eData = subEffect[6].kInitialEData;

        // distortion
        subEffect[7].activeA = true;
        subEffect[7].activeB = true;
        subEffect[7].activeC = true;
        subEffect[7].activeD = true;
        subEffect[7].activeE = true;

        subEffect[7].kMinAData = kMinDistortionGain;
        subEffect[7].kMaxAData = kMaxDistortionGain;

        subEffect[7].kMinBData = kMinDistortionThreshold;
        subEffect[7].kMaxBData = kMaxDistortionThreshold;

        subEffect[7].kMinCData = kMinDistortionTone;
        subEffect[7].kMaxCData = kMaxDistortionTone;

        subEffect[7].kMinDData = kMinDistortionDry;
        subEffect[7].kMaxDData = kMaxDistortionDry;

        subEffect[7].kMinEData = kMinDistortionWet;
        subEffect[7].kMaxEData = kMaxDistortionWet;

        subEffect[7].kInitialAData = kInitialDistortionGain;
        subEffect[7].kInitialBData = kInitialDistortionThreshold;
        subEffect[7].kInitialCData = kInitialDistortionTone;
        subEffect[7].kInitialDData = kInitialDistortionDry;
        subEffect[7].kInitialEData = kInitialDistortionWet;

        subEffect[7].aData = subEffect[7].kInitialAData;
        subEffect[7].bData = subEffect[7].kInitialBData;
        subEffect[7].cData = subEffect[7].kInitialCData;
        subEffect[7].dData = subEffect[7].kInitialDData;
        subEffect[7].eData = subEffect[7].kInitialEData;

        // bitcrusher
        subEffect[8].activeA = true;
        subEffect[8].activeB = true;
        subEffect[8].activeC = true;
        subEffect[8].activeD = true;
        subEffect[8].activeE = true;

        subEffect[8].kMinAData = kMinBitcrusherResolution;
        subEffect[8].kMaxAData = kMaxBitcrusherResolution;

        subEffect[8].kMinBData = kMinBitcrusherSampleRate;
        subEffect[8].kMaxBData = kMaxBitcrusherSampleRate;

        subEffect[8].kMinCData = kMinBitcrusherThreshold;
        subEffect[8].kMaxCData = kMaxBitcrusherThreshold;

        subEffect[8].kMinDData = kMinBitcrusherDry;
        subEffect[8].kMaxDData = kMaxBitcrusherDry;

        subEffect[8].kMinEData = kMinBitcrusherWet;
        subEffect[8].kMaxEData = kMaxBitcrusherWet;

        subEffect[8].kInitialAData = kInitialBitcrusherResolution;
        subEffect[8].kInitialBData = kInitialBitcrusherSampleRate;
        subEffect[8].kInitialCData = kInitialBitcrusherThreshold;
        subEffect[8].kInitialDData = kInitialBitcrusherDry;
        subEffect[8].kInitialEData = kInitialBitcrusherWet;

        subEffect[8].aData = subEffect[8].kInitialAData;
        subEffect[8].bData = subEffect[8].kInitialBData;
        subEffect[8].cData = subEffect[8].kInitialCData;
        subEffect[8].dData = subEffect[8].kInitialDData;
        subEffect[8].eData = subEffect[8].kInitialEData;

        delay.reset();
        chorus.reset();
        flanger.reset();
        phaser.reset();
        compressor.reset();
        expander.reset();
        overdrive.reset();
        distortion.reset();
        bitcrusher.reset();

        memset((uint8_t *)delayAddress, 0x00, kDelayByteSize);
        memset((uint8_t *)chorusAddress, 0x00, kChorusByteSize);
        memset(flangerBuffer, 0x00, sizeof(flangerBuffer));
    }

    Effect(uint8_t num_) : num(num_) {}
    ~Effect() {}
};

////////////////////////////////////////////////////////////////////////////////
/* Reverb Constants ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinReverbSize = 0;
const uint8_t kMaxReverbSize = 20;

const uint8_t kMinReverbDecay = 0;
const uint8_t kMaxReverbDecay = 20;

const uint8_t kMinReverbPreDelay = 0;
const uint8_t kMaxReverbPreDelay = 19;

const uint8_t kMinReverbSurround = 0;
const uint8_t kMaxReverbSurround = 20;

const uint8_t kMinReverbDry = 0;
const uint8_t kMaxReverbDry = 20;

const uint8_t kMinReverbWet = 0;
const uint8_t kMaxReverbWet = 20;

const bool kInitialReverbActive = true;
const uint8_t kInitialReverbSize = 5;
const uint8_t kInitialReverbDecay = 10;
const uint8_t kInitialReverbPreDelay = 5;
const uint8_t kInitialReverbSurround = 5;
const uint8_t kInitialReverbDry = 20;
const uint8_t kInitialReverbWet = 10;

const float kInitialReverbCFeedback = 0.84;
const float kInitialReverbCDecay1 = 0.20;
const float kInitialReverbCDecay2 = 0.80;

struct ReverbSizeData {
    char nameShortR[5];
    float data;
};

const struct ReverbSizeData kReverbSizeDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

struct ReverbDecayData {
    char nameShortR[5];
    float data;
};

const struct ReverbDecayData kReverbDecayDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

struct ReverbPreDelayData {
    char nameShortR[5];
    float data;
};

const struct ReverbPreDelayData kReverbPreDelayDataLibrary[] = {
    {"  00", 0.000}, {"  01", 0.001}, {"  02", 0.002}, {"  03", 0.003}, {"  04", 0.004}, {"  05", 0.005}, {"  06", 0.006}, {"  07", 0.007}, {"  08", 0.008}, {"  09", 0.009}, {"  10", 0.010}, {"  20", 0.020}, {"  30", 0.030}, {"  40", 0.040}, {"  50", 0.050}, {"  60", 0.060}, {"  70", 0.070}, {"  80", 0.080}, {"  90", 0.090}, {" 100", 0.100}};

struct ReverbSurroundData {
    char nameShortR[5];
    float data;
};

const struct ReverbSurroundData kReverbSurroundDataLibrary[] = {
    {"  00", 0.000}, {"  05", 0.001}, {"  10", 0.002}, {"  15", 0.003}, {"  20", 0.004}, {"  25", 0.005}, {"  30", 0.006}, {"  35", 0.007}, {"  40", 0.008}, {"  45", 0.009}, {"  50", 0.010}, {"  55", 0.011}, {"  60", 0.012}, {"  65", 0.013}, {"  70", 0.014}, {"  75", 0.015}, {"  80", 0.016}, {"  85", 0.017}, {"  90", 0.018}, {"  95", 0.019}, {" 100", 0.020}};

struct ReverbMixData {
    char nameShortR[5];
    float data;
};

const struct ReverbMixData kReverbMixDataLibrary[] = {
    {"  00", 0.00}, {"  05", 0.05}, {"  10", 0.10}, {"  15", 0.15}, {"  20", 0.20}, {"  25", 0.25}, {"  30", 0.30}, {"  35", 0.35}, {"  40", 0.40}, {"  45", 0.45}, {"  50", 0.50}, {"  55", 0.55}, {"  60", 0.60}, {"  65", 0.65}, {"  70", 0.70}, {"  75", 0.75}, {"  80", 0.80}, {"  85", 0.85}, {"  90", 0.90}, {"  95", 0.95}, {" 100", 1.00}};

typedef enum {
    REV_ACTION_NONE = 0x00,
    REV_ACTION_UP = 0x01,
    REV_ACTION_DOWN = 0x02
} ReverbTransitionAction;

typedef enum {
    REV_MODE_NONE = 0x00,
    REV_MODE_ACTIVE = 0x01,
    REV_MODE_PREDELAY = 0x02,
    REV_MODE_SURROUND = 0x03
} ReverbTransitionMode;

typedef enum {
    REV_PHASE_NONE = 0x00,
    REV_PHASE_A = 0x01,
    REV_PHASE_B = 0x02
} ReverbTransitionPhase;

struct ReverbGenTransition {
    bool active;

    ReverbTransitionMode mode;
    ReverbTransitionPhase phase;

    bool activeActive;
    bool targetActive;

    uint8_t activeType;
    uint8_t targetType;

    float activeDry;
    float targetDry;
    ReverbTransitionAction actionDry;

    float activeWet;
    float targetWet;
    ReverbTransitionAction actionWet;
};

struct ReverbMixTransition {
    bool active;

    float targetDry;
    ReverbTransitionAction actionDry;

    float targetWet;
    ReverbTransitionAction actionWet;
};

struct Reverb {
    bool active;
    uint8_t size;
    uint8_t decay;
    uint8_t preDelay;
    uint8_t surround;
    uint8_t dry;
    uint8_t wet;

    float dryFloat;
    float wetFloat;

    ReverbGenTransition genTransition;
    ReverbMixTransition mixTransition;

    const bool limitMix = true;
    const uint8_t limitMixData = 30;

    uint16_t preDelay_lag;
    uint16_t preDelay_playInterval;
    uint16_t preDelay_recordInterval;

    const static uint16_t kPreDelayBufferSize = 4500;
    int32_t preDelayBuffer[kPreDelayBufferSize] = {0};

    uint16_t surround_lag;
    uint16_t surround_playInterval;
    uint16_t surround_recordInterval;

    const static uint16_t kSurroundBufferSize = 900;
    int32_t surroundBuffer[kSurroundBufferSize] = {0};

    const static uint16_t comb1Size = 1116;
    const static uint16_t comb2Size = 1188;
    const static uint16_t comb3Size = 1277;
    const static uint16_t comb4Size = 1356;
    const static uint16_t comb5Size = 1422;
    const static uint16_t comb6Size = 1491;
    const static uint16_t comb7Size = 1557;
    const static uint16_t comb8Size = 1617;

    uint16_t comb1Index = 0;
    uint16_t comb2Index = 0;
    uint16_t comb3Index = 0;
    uint16_t comb4Index = 0;
    uint16_t comb5Index = 0;
    uint16_t comb6Index = 0;
    uint16_t comb7Index = 0;
    uint16_t comb8Index = 0;

    int32_t comb1Filter = 0;
    int32_t comb2Filter = 0;
    int32_t comb3Filter = 0;
    int32_t comb4Filter = 0;
    int32_t comb5Filter = 0;
    int32_t comb6Filter = 0;
    int32_t comb7Filter = 0;
    int32_t comb8Filter = 0;

    int32_t comb1Out = 0;
    int32_t comb2Out = 0;
    int32_t comb3Out = 0;
    int32_t comb4Out = 0;
    int32_t comb5Out = 0;
    int32_t comb6Out = 0;
    int32_t comb7Out = 0;
    int32_t comb8Out = 0;

    int32_t comb1Buffer[comb1Size];
    int32_t comb2Buffer[comb2Size];
    int32_t comb3Buffer[comb3Size];
    int32_t comb4Buffer[comb4Size];
    int32_t comb5Buffer[comb5Size];
    int32_t comb6Buffer[comb6Size];
    int32_t comb7Buffer[comb7Size];
    int32_t comb8Buffer[comb8Size];

    float combFeedback; // 0.70 - 0.98
    float combDecay1;   // 0.00 - 0.40
    float combDecay2;   // 0.60 - 1.00

    const static uint16_t apass1Size = 225;
    const static uint16_t apass2Size = 556;
    const static uint16_t apass3Size = 441;
    const static uint16_t apass4Size = 341;

    uint16_t apass1Index = 0;
    uint16_t apass2Index = 0;
    uint16_t apass3Index = 0;
    uint16_t apass4Index = 0;

    int32_t apass1Out = 0;
    int32_t apass2Out = 0;
    int32_t apass3Out = 0;
    int32_t apass4Out = 0;

    int32_t apass1Buffer[apass1Size];
    int32_t apass2Buffer[apass2Size];
    int32_t apass3Buffer[apass3Size];
    int32_t apass4Buffer[apass4Size];

    float apassFeedback = 0.5;

    void initialize() {
        active = kInitialReverbActive;
        size = kInitialReverbSize;
        decay = kInitialReverbDecay;
        preDelay = kInitialReverbPreDelay;
        surround = kInitialReverbSurround;
        dry = kInitialReverbDry;
        wet = kInitialReverbWet;

        dryFloat = kReverbMixDataLibrary[dry].data;
        wetFloat = kReverbMixDataLibrary[wet].data;

        combFeedback = kInitialReverbCFeedback;
        combDecay1 = kInitialReverbCDecay1;
        combDecay2 = kInitialReverbCDecay2;

        cleanCombBuffer();
        cleanApassBuffer();
        cleanPreDelayBuffer();
        cleanWidthDelayBuffer();

        preDelay_lag = kAudioSampleRate * kReverbPreDelayDataLibrary[kInitialReverbPreDelay].data;
        preDelay_playInterval = 0;
        preDelay_recordInterval = preDelay_lag;

        surround_lag = kAudioSampleRate * kReverbSurroundDataLibrary[kInitialReverbSurround].data;
        surround_playInterval = 0;
        surround_recordInterval = surround_lag;
    }

    void cleanPreDelayBuffer() {
        memset(preDelayBuffer, 0x00, sizeof(preDelayBuffer));
    }

    void cleanWidthDelayBuffer() {
        memset(surroundBuffer, 0x00, sizeof(surroundBuffer));
    }

    void cleanCombBuffer() {
        memset(comb1Buffer, 0x00, sizeof(comb1Buffer));
        memset(comb2Buffer, 0x00, sizeof(comb2Buffer));
        memset(comb3Buffer, 0x00, sizeof(comb3Buffer));
        memset(comb4Buffer, 0x00, sizeof(comb4Buffer));
        memset(comb5Buffer, 0x00, sizeof(comb5Buffer));
        memset(comb6Buffer, 0x00, sizeof(comb6Buffer));
        memset(comb7Buffer, 0x00, sizeof(comb7Buffer));
        memset(comb8Buffer, 0x00, sizeof(comb8Buffer));
    }

    void cleanApassBuffer() {
        memset(apass1Buffer, 0x00, sizeof(apass1Buffer));
        memset(apass2Buffer, 0x00, sizeof(apass2Buffer));
        memset(apass3Buffer, 0x00, sizeof(apass3Buffer));
        memset(apass4Buffer, 0x00, sizeof(apass4Buffer));
    }

    void cleanMemory() {
        cleanPreDelayBuffer();
        cleanWidthDelayBuffer();
        cleanCombBuffer();
        cleanApassBuffer();
    }

    void setSize(float size_) {
        if ((size_ <= 1.0f) && (size >= 0.0f)) {
            combFeedback = (size_ * 0.28) + 0.70;
        }
    }

    void setDecay(float decay_) {
        if ((decay_ <= 1.0f) && (decay >= 0.0f)) {
            combDecay1 = decay_ * 0.40;
            combDecay2 = 1.0 - combDecay1;
        }
    }

    void setPreDelay(float preDelay_) {
        preDelay_lag = kAudioSampleRate * preDelay_;
        int16_t playInterval_ = preDelay_recordInterval - preDelay_lag;
        if (playInterval_ < 0)
            playInterval_ += kPreDelayBufferSize;
        preDelay_playInterval = playInterval_;
    }

    void setSurround(float surround_) {
        surround_lag = kAudioSampleRate * surround_;
        int16_t playInterval_ = surround_recordInterval - surround_lag;
        if (playInterval_ < 0)
            playInterval_ += kSurroundBufferSize;
        surround_playInterval = playInterval_;
    }
};

////////////////////////////////////////////////////////////////////////////////
/* Key Constants -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinKeyNote = 1;
const uint8_t kMaxKeyNote = 12;

const uint8_t kMinKeyRate = 0;
const uint8_t kMaxKeyRate = 7;

const uint8_t kMinKeyOsc = 0;
const uint8_t kMaxKeyOsc = 2;

const uint8_t kMinKeyChord3 = 0;
const uint8_t kMaxKeyChord3 = 3;

const uint8_t kMinKeyChord4 = 0;
const uint8_t kMaxKeyChord4 = 5;

const uint8_t kMinKeyOrder = 0;
const uint8_t kMaxKeyOrder = 4;

const uint8_t kMinKeyOctave = 0;
const uint8_t kMaxKeyOctave = 1;

const uint8_t kInitialKeyNote = 1;
const bool kInitialKeyArpeg = false;
const uint8_t kInitialKeyRate = 4;
const uint8_t kInitialKeyOsc = 0;
const uint8_t kInitialKeyChord = 0;
const uint8_t kInitialKeyOrder = 0;
const uint8_t kInitialKeyOctave = 0;

struct KeyNoteData {
    char nameShortL[5];
    char nameShortR[5];
    char nameLongR[11];
};

const struct KeyNoteData kKeyNoteDataLibrary[] = {
    {"--- ", " ---", "  --------"},
    {"C   ", "   C", "         C"},
    {"C#  ", "  C#", "        C#"},
    {"D   ", "   D", "         D"},
    {"D#  ", "  D#", "        D#"},
    {"E   ", "   E", "         E"},
    {"F   ", "   F", "         F"},
    {"F#  ", "  F#", "        F#"},
    {"G   ", "   G", "         G"},
    {"G#  ", "  G#", "        G#"},
    {"A   ", "   A", "         A"},
    {"A#  ", "  A#", "        A#"},
    {"B   ", "   B", "         B"}};

struct KeyRateData {
    char nameShortL[5];
    char nameShortR[5];
    float rate;
};

const struct KeyRateData kKeyRateDataLibrary[] = {
    {"1/16", "1/16", 0.06250},
    {"1/12", "1/12", 0.08333},
    {"1/8 ", " 1/8", 0.12500},
    {"1/6 ", " 1/6", 0.16666},
    {"1/4 ", " 1/4", 0.25000},
    {"1/3 ", " 1/3", 0.33333},
    {"1/2 ", " 1/2", 0.50000},
    {"1/1 ", " 1/1", 1.00000}};

struct KeyOscData {
    char nameShortL[5];
    char nameShortR[5];
    bool osc[2];
};

const struct KeyOscData kKeyOscDataLibrary[] = {
    {"A+B ", " A+B", {true, true}},
    {"A   ", "   A", {true, false}},
    {"B   ", "   B", {false, true}}};

struct KeyChordData {
    char nameShortL[5];
    char nameShortR[5];
};

typedef enum {
    CHORD_3 = 0x00,
    CHORD_4 = 0x01,
} ChordType;

const struct KeyChordData kKeyChord3DataLibrary[] = {
    {"MAJ ", " MAJ"},
    {"MIN ", " MIN"},
    {"AUG ", " AUG"},
    {"DIM ", " DIM"}};

const struct KeyChordData kKeyChord4DataLibrary[] = {
    {"MAJ6", "MAJ6"},
    {"MAJ7", "MAJ7"},
    {"MIN6", "MIN6"},
    {"MIN7", "MIN7"},
    {"DOM7", "DOM7"},
    {"DIM7", "DIM7"}};

const uint8_t kKeyArpeg3DataLibrary[4][4][2][16] = {
    // major
    {
        {{0, 4, 7}, {0, 4, 7, 12, 16, 19}},                                 // u
        {{7, 4, 0}, {19, 16, 12, 7, 4, 0}},                                 // d
        {{0, 4, 7, 12, 7, 4}, {0, 4, 7, 12, 16, 19, 24, 19, 16, 12, 7, 4}}, // ud
        {{12, 7, 4, 0, 4, 7}, {24, 19, 16, 12, 7, 4, 0, 4, 7, 12, 16, 19}}, // du
    },
    // minor
    {
        {{0, 3, 7}, {0, 3, 7, 12, 15, 19}},                                 // u
        {{7, 3, 0}, {19, 15, 12, 7, 3, 0}},                                 // d
        {{0, 3, 7, 12, 7, 3}, {0, 3, 7, 12, 15, 19, 24, 19, 15, 12, 7, 3}}, // ud
        {{12, 7, 3, 0, 3, 7}, {24, 19, 15, 12, 7, 3, 0, 3, 7, 12, 15, 19}}, // du
    },
    // aug
    {
        {{0, 4, 8}, {0, 4, 8, 12, 16, 20}},                                 // u
        {{8, 4, 0}, {20, 16, 12, 8, 4, 0}},                                 // d
        {{0, 4, 8, 12, 8, 4}, {0, 4, 8, 12, 16, 20, 24, 20, 16, 12, 8, 4}}, // ud
        {{12, 8, 4, 0, 4, 8}, {24, 20, 16, 12, 8, 4, 0, 4, 8, 12, 16, 20}}, // du
    },
    // dim
    {
        {{0, 3, 6}, {0, 3, 6, 12, 15, 18}},                                 // u
        {{6, 3, 0}, {18, 15, 12, 6, 3, 0}},                                 // d
        {{0, 3, 6, 12, 6, 3}, {0, 3, 6, 12, 15, 18, 24, 18, 15, 12, 6, 3}}, // ud
        {{12, 6, 3, 0, 3, 6}, {24, 18, 15, 12, 6, 3, 0, 3, 6, 12, 15, 18}}, // du
    }};

const uint8_t kKeyArpeg4DataLibrary[6][4][2][16] = {
    // major-6
    {
        {{0, 4, 7, 9}, {0, 4, 7, 9, 12, 16, 19, 21}},                                           // u
        {{9, 7, 4, 0}, {21, 19, 16, 12, 9, 7, 4, 0}},                                           // d
        {{0, 4, 7, 9, 12, 9, 7, 4}, {0, 4, 7, 9, 12, 16, 19, 21, 24, 21, 19, 16, 12, 9, 7, 4}}, // ud
        {{12, 9, 7, 4, 0, 4, 7, 9}, {24, 21, 19, 16, 12, 9, 7, 4, 0, 4, 7, 9, 12, 16, 19, 21}}, // du
    },
    // major-7
    {
        {{0, 4, 7, 11}, {0, 4, 7, 11, 12, 16, 19, 23}},                                             // u
        {{11, 7, 4, 0}, {23, 19, 16, 12, 11, 7, 4, 0}},                                             // d
        {{0, 4, 7, 11, 12, 11, 7, 4}, {0, 4, 7, 11, 12, 16, 19, 23, 24, 23, 19, 16, 12, 11, 7, 4}}, // ud
        {{12, 11, 7, 4, 0, 4, 7, 11}, {24, 23, 19, 16, 12, 11, 7, 4, 0, 4, 7, 11, 12, 16, 19, 23}}, // du
    },
    // minor-6
    {
        {{0, 3, 7, 9}, {0, 3, 7, 9, 12, 15, 19, 21}},                                           // u
        {{9, 7, 3, 0}, {21, 19, 15, 12, 9, 7, 3, 0}},                                           // d
        {{0, 3, 7, 9, 12, 9, 7, 3}, {0, 3, 7, 9, 12, 15, 19, 21, 24, 21, 19, 15, 12, 9, 7, 3}}, // ud
        {{12, 9, 7, 3, 0, 3, 7, 9}, {24, 21, 19, 15, 12, 9, 7, 3, 0, 3, 7, 9, 12, 15, 19, 21}}, // du
    },
    // minor-7
    {
        {{0, 3, 7, 10}, {0, 3, 7, 10, 12, 15, 19, 22}},                                             // u
        {{10, 7, 3, 0}, {22, 19, 15, 12, 10, 7, 3, 0}},                                             // d
        {{0, 3, 7, 10, 12, 10, 7, 3}, {0, 3, 7, 10, 12, 15, 19, 22, 24, 22, 19, 15, 12, 10, 7, 3}}, // ud
        {{12, 10, 7, 3, 0, 3, 7, 10}, {24, 22, 19, 15, 12, 10, 7, 3, 0, 3, 7, 10, 12, 15, 19, 22}}, // du
    },
    // dom-7
    {
        {{0, 4, 7, 10}, {0, 4, 7, 10, 12, 16, 19, 22}},                                             // u
        {{10, 7, 4, 0}, {22, 19, 16, 12, 10, 7, 4, 0}},                                             // d
        {{0, 4, 7, 10, 12, 10, 7, 4}, {0, 4, 7, 10, 12, 16, 19, 22, 24, 22, 19, 16, 12, 10, 7, 4}}, // ud
        {{12, 10, 7, 4, 0, 4, 7, 10}, {24, 22, 19, 16, 12, 10, 7, 4, 0, 4, 7, 10, 12, 16, 19, 22}}, // du
    },
    // dim-7
    {
        {{0, 3, 6, 9}, {0, 3, 6, 9, 12, 15, 18, 21}},                                           // u
        {{9, 6, 3, 0}, {21, 18, 15, 12, 9, 6, 3, 0}},                                           // d
        {{0, 3, 6, 9, 12, 9, 6, 3}, {0, 3, 6, 9, 12, 15, 18, 21, 24, 21, 18, 15, 12, 9, 6, 3}}, // ud
        {{12, 9, 6, 3, 0, 3, 6, 9}, {24, 21, 18, 15, 12, 9, 6, 3, 0, 3, 6, 9, 12, 15, 18, 21}}, // du
    }};

struct KeyOrderData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct KeyOrderData kKeyOrderDataLibrary[] = {
    {"UP  ", "  UP"},
    {"DOWN", "DOWN"},
    {"UPDO", "UPDO"},
    {"DOUP", "DOUP"},
    {"RAND", "RAND"}};

struct KeyOctaveData {
    char nameShortL[5];
    char nameShortR[5];
};

const struct KeyOctaveData kKeyOctaveDataLibrary[] = {
    {"SIN ", " SIN"},
    {"DOU ", " DOU"},
};

struct Key {
    uint8_t note = kInitialKeyNote;
    bool arpeg = kInitialKeyArpeg;
    uint8_t rate = kInitialKeyRate;
    uint8_t osc = kInitialKeyOsc;
    uint8_t chord = kInitialKeyChord;
    uint8_t order = kInitialKeyOrder;
    uint8_t octave = kInitialKeyOctave;

    bool oscActive[2] = {true, true};
    ChordType chordType = CHORD_4;
    uint8_t stepSize = 4;
    uint8_t *chordPointer = NULL;
    uint8_t chordPattern = 0;
};

////////////////////////////////////////////////////////////////////////////////
/* Octave Constants ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const uint8_t kMinOctave = 2;
const uint8_t kMaxOctave = 6;

const uint8_t kInitialOctave = 4;

////////////////////////////////////////////////////////////////////////////////
/* Note Constants ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

const char kNoteTextDataLibrary[][10] = {
    "        C",
    "       C#",
    "        D",
    "       D#",
    "        E",
    "        F",
    "       F#",
    "        G",
    "       G#",
    "        A",
    "       A#",
    "        B"};

const char kOctaveTextDataLibrary[][2] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7"};

struct NoteData {
    float frequency;
    int16_t sampleSize; // 44100 / frequency
    float sampleInc;    // 2048 / sampleSize
};

const struct NoteData kNoteDataLibrary[] = {
    {8.18, 5391, 0.379892},  // C-1     // 000
    {8.66, 5092, 0.402199},  // C#-1    // 001
    {9.18, 4803, 0.426400},  // D-1     // 002
    {9.72, 4537, 0.451399},  // D#-1    // 003
    {10.30, 4281, 0.478392}, // E-1     // 004
    {10.91, 4042, 0.506679}, // F-1     // 005
    {11.56, 3814, 0.536969}, // F#-1    // 006
    {12.25, 3600, 0.568888}, // G-1     // 007
    {12.98, 3397, 0.602884}, // G#-1    // 008
    {13.75, 3207, 0.638603}, // A-1     // 009
    {14.57, 3026, 0.676801}, // A#-1    // 010
    {15.43, 2858, 0.716585}, // B-1     // 011

    {16.35, 2697, 0.759362}, // C0      // 012
    {17.32, 2546, 0.804399}, // C#0     // 013
    {18.35, 2403, 0.852267}, // D0      // 014
    {19.45, 2267, 0.903396}, // D#0     // 015
    {20.60, 2140, 0.957009}, // E0      // 016
    {21.83, 2020, 1.013861}, // F0      // 017
    {23.12, 1907, 1.073938}, // F#0     // 018
    {24.50, 1800, 1.137777}, // G0      // 019
    {25.96, 1698, 1.206124}, // G#0     // 020
    {27.50, 1603, 1.277604}, // A0      // 021
    {29.14, 1513, 1.353602}, // A#0     // 022
    {30.87, 1428, 1.434173}, // B0      // 023

    {32.70, 1348, 1.519287}, // C1      // 024
    {34.65, 1272, 1.610062}, // C#1     // 025
    {36.71, 1201, 1.705245}, // D1      // 026
    {38.89, 1133, 1.807590}, // D#1     // 027
    {41.20, 1070, 1.914018}, // E1      // 028
    {43.65, 1010, 2.027722}, // F1      // 029
    {46.25, 953, 2.149003},  // F#1     // 030
    {49.00, 900, 2.275555},  // G1      // 031
    {51.91, 849, 2.412249},  // G#1     // 032
    {55.00, 801, 2.556803},  // A1      // 033
    {58.27, 756, 2.708994},  // A#1     // 034
    {61.74, 714, 2.868347},  // B1      // 035

    {65.41, 674, 3.038575},  // C2      // 036
    {69.30, 636, 3.220125},  // C#2     // 037
    {73.42, 600, 3.413333},  // D2      // 038
    {77.78, 566, 3.618374},  // D#2     // 039
    {82.41, 535, 3.828037},  // E2      // 040
    {87.31, 505, 4.055445},  // F2      // 041
    {92.50, 476, 4.302512},  // F#2     // 042
    {98.00, 450, 4.551111},  // G2      // 043
    {103.83, 424, 4.830188}, // G#2     // 044
    {110.00, 400, 5.120000}, // A2      // 045
    {116.54, 378, 5.417989}, // A#2     // 046
    {123.47, 357, 5.736694}, // B2      // 047

    {130.81, 337, 6.077151},  // C3     // 048
    {138.59, 318, 6.440251},  // C#3    // 049
    {146.83, 300, 6.826666},  // D3     // 050
    {155.56, 283, 7.236749},  // D#3    // 051
    {164.81, 267, 7.670411},  // E3     // 052
    {174.61, 252, 8.126984},  // F3     // 053
    {185.00, 238, 8.605042},  // F#3    // 054
    {196.00, 225, 9.102222},  // G3     // 055
    {207.65, 212, 9.660377},  // G#3    // 056
    {220.00, 200, 10.240000}, // A3     // 057
    {233.08, 189, 10.835978}, // A#3    // 058
    {246.94, 178, 11.505617}, // B3     // 059

    {261.63, 168, 12.190476}, // C4     // 060
    {277.18, 159, 12.880503}, // C#4    // 061
    {293.66, 150, 13.653333}, // D4     // 062
    {311.13, 141, 14.524822}, // D#4    // 063
    {329.63, 133, 15.398496}, // E4     // 064
    {349.23, 126, 16.253968}, // F4     // 065
    {369.99, 119, 17.210084}, // F#4    // 066
    {392.00, 112, 18.285714}, // G4     // 067
    {415.30, 106, 19.320754}, // G#4    // 068
    {440.00, 100, 20.480000}, // A4     // 069
    {466.16, 94, 21.787234},  // A#4    // 070
    {493.88, 89, 23.011235},  // B4     // 071

    {523.25, 84, 24.380952}, // C5      // 072
    {554.37, 79, 25.924050}, // C#5     // 073
    {587.33, 75, 27.306666}, // D5      // 074
    {622.25, 70, 29.257142}, // D#5     // 075
    {659.25, 66, 31.030303}, // E5      // 076
    {698.46, 63, 32.507936}, // F5      // 077
    {739.99, 59, 34.711864}, // F#5     // 078
    {783.99, 56, 36.571428}, // G5      // 079
    {830.61, 53, 38.641509}, // G#5     // 080
    {880.00, 50, 40.960000}, // A5      // 081
    {932.33, 47, 43.574468}, // A#5     // 082
    {987.77, 44, 46.545454}, // B5      // 083

    {1046.50, 42, 48.761904}, // C6     // 084
    {1108.73, 39, 52.512820}, // C#6    // 085
    {1174.66, 37, 55.351351}, // D6     // 086
    {1244.51, 35, 58.514285}, // D#6    // 087
    {1318.51, 33, 62.060606}, // E6     // 088
    {1396.91, 31, 66.064516}, // F6     // 089
    {1479.98, 29, 70.620689}, // F#6    // 090
    {1567.98, 28, 73.142857}, // G6     // 091
    {1661.22, 26, 78.769230}, // G#6    // 092
    {1760.00, 25, 81.920000}, // A6     // 093
    {1864.66, 23, 89.043478}, // A#6    // 094
    {1975.53, 22, 93.090909}, // B6     // 095

    {2093.00, 21, 97.523809},  // C7    // 096
    {2217.46, 19, 107.789473}, // C#7   // 097
    {2349.32, 18, 113.777777}, // D7    // 098
    {2489.02, 17, 120.470588}, // D#7   // 099
    {2637.02, 16, 128.000000}, // E7    // 100
    {2793.83, 15, 136.533333}, // F7    // 101
    {2959.96, 14, 146.285714}, // F#7   // 102
    {3135.96, 14, 146.285714}, // G7    // 103
    {3322.44, 13, 157.538461}, // G#7   // 104
    {3520.00, 12, 170.666666}, // A7    // 105
    {3729.31, 11, 186.181818}, // A#7   // 106
    {3951.07, 11, 186.181818}, // B7    // 107

    {4186.01, 10, 204.800000}, // C8    // 108
    {4434.92, 9, 227.555556},  // C#8   // 109
    {4698.63, 9, 227.555556},  // D8    // 110
    {4978.03, 8, 256.000000},  // D#8   // 111
    {5274.04, 8, 256.000000},  // E8    // 112
    {5587.65, 7, 292.571428},  // F8    // 113
    {5919.91, 7, 292.571428},  // F#8   // 114
    {6271.93, 7, 292.571428},  // G8    // 115
    {6644.88, 6, 341.333333},  // G#8   // 116
    {7040.00, 6, 341.333333},  // A8    // 117
    {7458.62, 5, 409.600000},  // A#8   // 118
    {7902.13, 5, 409.600000},  // B8    // 119

    {8372.02, 5, 409.600000},   // C9   // 120
    {8869.84, 4, 512.000000},   // C#9  // 121
    {9397.27, 4, 512.000000},   // D9   // 122
    {9956.06, 4, 512.000000},   // D#9  // 123
    {10548.08, 4, 512.000000},  // E9   // 124
    {11175.30, 3, 682.666666},  // F9   // 125
    {11839.82, 3, 682.666666},  // F#9  // 126
    {12543.85, 3, 682.666666}}; // G9   // 127

/*----------------------------------------------------------------------------*/

struct Beat {
    bool active = false;
    bool recordActive = false;
    uint8_t octave = 0;
    uint8_t note = 0;
    uint16_t startInterval = 0x00;
    uint16_t endInterval = 0x00;

    Beat() {}
    ~Beat() {}

    void start(uint16_t startInterval_, uint8_t octave_, uint8_t note_) {
        active = true;
        recordActive = true;
        octave = octave_;
        note = note_;
        startInterval = startInterval_;
    }
    void end(uint16_t endInterval_) {
        recordActive = false;
        endInterval = endInterval_;
    }
    void set(uint16_t startInterval_, uint16_t endInterval_, uint8_t octave_, uint8_t note_) {
        active = true;
        octave = octave_;
        note = note_;
        startInterval = startInterval_;
        endInterval = endInterval_;
    }
    void reset() {
        active = false;
        recordActive = false;
        octave = 0;
        note = 0;
        startInterval = 0x00;
        endInterval = 0x00;
    }
};

/*----------------------------------------------------------------------------*/

const uint8_t kBeatLibrarySize = 80;
const uint8_t kBankLibrarySize = 7;

const uint8_t kInitialBank = 0;

struct Bank {
    Beat beatLibrary[kBeatLibrarySize];

    int8_t lastActiveBeatNum = -1;
    int8_t playBeatNum = -1;
    int8_t nextBeatNum = -1;
};

/*----------------------------------------------------------------------------*/

struct Song {
    Bank bankLibrary[kBankLibrarySize];

    Song() {}
    ~Song() {}
};

////////////////////////////////////////////////////////////////////////////////
/* PlayData Constants --------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

struct MetroPlayData {
    bool active;
    uint32_t counter;
    uint32_t counterMax;
    uint32_t ramAddress;
    float volumeMultiplier;
};

struct ArpegPlayData {
    bool active;
    bool trigger;

    float freq;

    uint8_t stepSize;
    uint8_t stepCounter;

    uint8_t *chordPointer;
    bool random;

    uint32_t sampleSize;    // 44100 / freq
    uint32_t sampleCounter; // 0
    uint32_t sampleInc = 1; // 1
};

struct OscLfoPlayData {
    bool active;
    uint8_t type;
    uint8_t target;
    float freq;
    float depth;
    bool loop;

    uint32_t sampleSize;    // 44100 / freq
    uint32_t sampleCounter; // 0
    uint32_t sampleInc = 1; // 1

    double arrayCounter; // 0
    double arrayInc;     // 2048 / sampleSize

    float data;
};

struct OscPlayData {
    bool active;
    uint32_t address;
    uint16_t phaseShift;
    bool normalize;
    bool xFlip;
    bool yFlip;

    float normMultiplier;

    bool endFlag;
    bool endReadMode;
    bool endLoopMode;

    bool sdramReadActive;

    uint16_t noteBase;

    float freqBase;
    float freqCurrent;

    float levelBase;
    float levelRange;
    float levelGround;
    float levelCurrent;
    float levelInc; // levelSmooth == true

    uint16_t waveBase;
    uint16_t waveRange;
    float waveCurrent;
    float waveInc; // waveSmooth == true

    uint32_t sampleSize;    // 44100 / freq
    uint32_t sampleCounter; // 0
    uint32_t sampleInc = 1; // 1

    float arrayCounter; // 0
    float arrayInc;     // 2048 / sampleSize

    bool lfoTrigger;

    bool freqLfoActive;
    bool levelLfoActive;
    bool waveLfoActive;

    int8_t freqLfoNum;
    int8_t levelLfoNum;
    int8_t waveLfoNum;

    bool levelSmooth;
    bool waveSmooth;

    int16_t data[2750];     // 2048
    int32_t outputData;
};

struct EnvelopeStage {
    bool active;
    uint32_t sampleSize;
    uint32_t sampleCounter;
    uint32_t sampleInc = 1;
    uint8_t curve;
    float dataStart;
    float dataEnd;
    float dataCurrent;
    float dataInc;
};

struct EnvelopePlayData {
    bool active;
    uint8_t type;
    uint8_t curve;
    float level;

    int32_t inputData;
    int32_t outputData;

    uint8_t activeStage;

    EnvelopeStage attack;
    EnvelopeStage decay;
    EnvelopeStage sustain;
    EnvelopeStage release;
    EnvelopeStage end;
};

struct TriggerData {
    bool active;
    uint8_t beatNum;
    uint8_t noteData;
};

struct BeatPlayData {
    bool active;

    bool sdramReadActive;

    uint8_t noteData;

    OscPlayData oscPlayData[2];
    OscLfoPlayData oscLfoPlayData[2][2];
    ArpegPlayData arpegPlayData[2];
    EnvelopePlayData envelopePlayData;

    int32_t data;
};

struct SongPlayData {
    bool active;

    TriggerData startData;
    TriggerData releaseData;
    TriggerData endData;

    int8_t activeBeatNum = -1;

    BeatPlayData beatPlayData[2];

    int32_t data;
};

const uint8_t wavIntro[44] = {0x52, 0x49, 0x46, 0x46, 0xAC, 0x58, 0x01, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00, 0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x10, 0xB1, 0x02, 0x00};

#endif
