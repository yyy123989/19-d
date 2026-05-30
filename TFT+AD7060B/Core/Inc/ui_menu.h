#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>

typedef enum {
  UI_KEY_EVENT_NONE = 0,
  UI_KEY_EVENT_KEY1_SHORT,
  UI_KEY_EVENT_KEY0_SHORT,
  UI_KEY_EVENT_KEY1_LONG,
  UI_KEY_EVENT_KEY0_LONG
} UI_KeyEvent;

void UI_MenuKeysInit(void);
UI_KeyEvent UI_MenuScanKeys(uint32_t now_ms);

#endif
