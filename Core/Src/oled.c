#include "oled.h"

#include "i2c.h"

#include <stddef.h>
#include <string.h>

#define OLED_ADDR_DEFAULT    (0x3CU << 1)  /* SA0 = GND (most modules)  */
#define OLED_ADDR_ALT        (0x3DU << 1)  /* SA0 = VCC                 */
#define OLED_COL_OFFSET      2U            /* 0 = SSD1306, 2 = SH1106   */
#define OLED_WIDTH           128U
#define OLED_HEIGHT          64U
#define OLED_PAGE_COUNT      (OLED_HEIGHT / 8U)
#define OLED_BUFFER_SIZE     (OLED_WIDTH * OLED_PAGE_COUNT)
#define OLED_CONTROL_CMD     0x00U
#define OLED_CONTROL_DATA    0x40U

static uint8_t gOledBuffer[OLED_BUFFER_SIZE];
static uint8_t gOledI2cAddr = OLED_ADDR_DEFAULT;

static void OLED_WriteCommand(uint8_t cmd)
{
  uint8_t tx[2] = { OLED_CONTROL_CMD, cmd };
  (void)HAL_I2C_Master_Transmit(&hi2c1, gOledI2cAddr, tx, sizeof(tx), HAL_MAX_DELAY);
}

static const uint8_t* OLED_GetGlyph(char c)
{
  static const uint8_t glyphSpace[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t glyphColon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
  static const uint8_t glyphDot[5]   = {0x00, 0x40, 0x60, 0x00, 0x00};
  static const uint8_t glyphZero[5]  = {0x3E, 0x51, 0x49, 0x45, 0x3E};
  static const uint8_t glyphOne[5]   = {0x00, 0x42, 0x7F, 0x40, 0x00};
  static const uint8_t glyphTwo[5]   = {0x62, 0x51, 0x49, 0x49, 0x46};
  static const uint8_t glyphThree[5] = {0x22, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t glyphFour[5]  = {0x18, 0x14, 0x12, 0x7F, 0x10};
  static const uint8_t glyphFive[5]  = {0x2F, 0x49, 0x49, 0x49, 0x31};
  static const uint8_t glyphSix[5]   = {0x3E, 0x49, 0x49, 0x49, 0x30};
  static const uint8_t glyphSeven[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
  static const uint8_t glyphEight[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t glyphNine[5]  = {0x06, 0x49, 0x49, 0x29, 0x1E};
  static const uint8_t glyphA[5]     = {0x7E, 0x11, 0x11, 0x11, 0x7E};
  static const uint8_t glyphC[5]     = {0x3E, 0x41, 0x41, 0x41, 0x22};
  static const uint8_t glyphD[5]     = {0x7F, 0x41, 0x41, 0x22, 0x1C};
  static const uint8_t glyphM[5]     = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
  static const uint8_t glyphQ[5]     = {0x3E, 0x41, 0x51, 0x21, 0x5E};
  static const uint8_t glyphV[5]     = {0x1F, 0x20, 0x40, 0x20, 0x1F};
  static const uint8_t glyphO[5]     = {0x3E, 0x41, 0x41, 0x41, 0x3E};
  static const uint8_t glyphP[5]     = {0x7F, 0x09, 0x09, 0x09, 0x06};
  static const uint8_t glyphL[5]     = {0x7F, 0x40, 0x40, 0x40, 0x40};
  static const uint8_t glyphR[5]     = {0x7F, 0x09, 0x19, 0x29, 0x46};
  static const uint8_t glyphW[5]     = {0x3F, 0x40, 0x38, 0x40, 0x3F};
  static const uint8_t glyphN[5]     = {0x7F, 0x04, 0x08, 0x10, 0x7F};
  static const uint8_t glyphFull[5]  = {0x0E, 0x1F, 0x1F, 0x1F, 0x0E};  /* ● joined    */
  static const uint8_t glyphEmpty[5] = {0x0E, 0x11, 0x11, 0x11, 0x0E};  /* ○ not joined */

  switch (c)
  {
    case ' ': return glyphSpace;
    case ':': return glyphColon;
    case '.': return glyphDot;
    case '0': return glyphZero;
    case '1': return glyphOne;
    case '2': return glyphTwo;
    case '3': return glyphThree;
    case '4': return glyphFour;
    case '5': return glyphFive;
    case '6': return glyphSix;
    case '7': return glyphSeven;
    case '8': return glyphEight;
    case '9': return glyphNine;
    case 'A': return glyphA;
    case 'C': return glyphC;
    case 'D': return glyphD;
    case 'M': return glyphM;
    case 'Q': return glyphQ;
    case 'V': return glyphV;
    case 'O': return glyphO;
    case 'P': return glyphP;
    case 'L': return glyphL;
    case 'R': return glyphR;
    case 'W': return glyphW;
    case 'N': return glyphN;
    case '\x01': return glyphFull;
    case '\x02': return glyphEmpty;
    default:  return glyphSpace;
  }
}

void OLED_Clear(void)
{
  (void)memset(gOledBuffer, 0, sizeof(gOledBuffer));
}

void OLED_DrawText(uint8_t x, uint8_t y, const char *text)
{
  uint16_t index = (uint16_t)y * OLED_WIDTH + x;

  while ((text != NULL) && (*text != '\0'))
  {
    const uint8_t *glyph = OLED_GetGlyph(*text);
    uint8_t col = 0;

    if ((index + 5U) >= OLED_BUFFER_SIZE)
    {
      break;
    }

    for (col = 0; col < 5U; col++)
    {
      gOledBuffer[index++] = glyph[col];
    }
    gOledBuffer[index++] = 0x00;
    text++;
  }
}

void OLED_Update(void)
{
  uint8_t page = 0;

  for (page = 0; page < OLED_PAGE_COUNT; page++)
  {
    /* Always write from col 0. For SH1106 (OLED_COL_OFFSET=2) cols 0-1 are
     * hidden behind the panel border; filling them with zeros every frame
     * prevents the power-on RAM garbage from appearing as a bright streak. */
    uint8_t cmd[] = {
      OLED_CONTROL_CMD,
      (uint8_t)(0xB0U + page),
      0x00U,  /* lower  column address = 0 */
      0x10U   /* higher column address = 0 */
    };

    uint8_t data[1U + OLED_COL_OFFSET + OLED_WIDTH];
    uint8_t i;
    data[0] = OLED_CONTROL_DATA;
    for (i = 0U; i < OLED_COL_OFFSET; i++)
    {
      data[1U + i] = 0x00U;  /* zero the hidden columns */
    }
    (void)memcpy(&data[1U + OLED_COL_OFFSET], &gOledBuffer[page * OLED_WIDTH], OLED_WIDTH);

    (void)HAL_I2C_Master_Transmit(&hi2c1, gOledI2cAddr, cmd, sizeof(cmd), HAL_MAX_DELAY);
    (void)HAL_I2C_Master_Transmit(&hi2c1, gOledI2cAddr, data, sizeof(data), HAL_MAX_DELAY);
  }
}

void OLED_Init(void)
{
  static const uint8_t initCmds[] = {
    0xAE,             /* Display OFF                              */
    0x20, 0x02,       /* Memory addr mode: Page (matches Update)  */
    0x40,             /* Display start line = 0                   */
    0xA1,             /* Segment remap: col 127 -> SEG0           */
    0xC8,             /* COM output scan: reversed                */
    0x81, 0x7F,       /* Contrast                                 */
    0xA6,             /* Normal (non-inverted) display            */
    0xA8, 0x3F,       /* Multiplex ratio: 64 rows                 */
    0xD3, 0x00,       /* Display offset: 0                        */
    0xD5, 0x80,       /* Clock divide / oscillator freq           */
    0xD9, 0xF1,       /* Pre-charge period                        */
    0xDA, 0x12,       /* COM pins hardware config (alt, no remap) */
    0xDB, 0x20,       /* VCOMH deselect level: 0.77*VCC           */
    0x8D, 0x14,       /* Charge pump: enable                      */
    0xA4,             /* Entire display follows RAM               */
    0xAF              /* Display ON                               */
  };
  uint32_t i = 0;

  HAL_Delay(200); /* wait for OLED power-on stable */

  /* auto-detect I2C address: try 0x3C first, then 0x3D */
  if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_ADDR_DEFAULT, 3U, 20U) == HAL_OK)
  {
    gOledI2cAddr = OLED_ADDR_DEFAULT;
  }
  else if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_ADDR_ALT, 3U, 20U) == HAL_OK)
  {
    gOledI2cAddr = OLED_ADDR_ALT;
  }

  for (i = 0; i < (sizeof(initCmds) / sizeof(initCmds[0])); i++)
  {
    OLED_WriteCommand(initCmds[i]);
  }

  OLED_Clear();
  OLED_Update();
}
