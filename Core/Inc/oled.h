#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Status icon characters for OLED_DrawText */
#define OLED_CHAR_JOINED     '\x01'  /* ● filled  circle – LoRaWAN joined     */
#define OLED_CHAR_NOT_JOINED '\x02'  /* ○ hollow circle – LoRaWAN not joined */

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_DrawText(uint8_t x, uint8_t y, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
