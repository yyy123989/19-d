#include "ui_menu.h"

#include "main.h"

#define UI_KEY_DEBOUNCE_MS 30U
#define UI_KEY_LONG_MS     500U

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState last_raw;
  GPIO_PinState stable;
  uint32_t changed_ms;
  uint32_t pressed_ms;
  uint8_t pressed;
  uint8_t long_sent;
  UI_KeyEvent short_event;
  UI_KeyEvent long_event;
} UI_KeyState;

static UI_KeyState ui_keys[] = {
  {KEY1_GPIO_Port, KEY1_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0U, 0U, 0U, 0U,
   UI_KEY_EVENT_KEY1_SHORT, UI_KEY_EVENT_KEY1_LONG},
  {KEY0_GPIO_Port, KEY0_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0U, 0U, 0U, 0U,
   UI_KEY_EVENT_KEY0_SHORT, UI_KEY_EVENT_KEY0_LONG},
};

void UI_MenuKeysInit(void)
{
  uint8_t i;

  for (i = 0U; i < (uint8_t)(sizeof(ui_keys) / sizeof(ui_keys[0])); i++) {
    ui_keys[i].last_raw = HAL_GPIO_ReadPin(ui_keys[i].port, ui_keys[i].pin);
    ui_keys[i].stable = ui_keys[i].last_raw;
    ui_keys[i].changed_ms = HAL_GetTick();
    ui_keys[i].pressed_ms = ui_keys[i].changed_ms;
    ui_keys[i].pressed = (ui_keys[i].stable == GPIO_PIN_RESET) ? 1U : 0U;
    ui_keys[i].long_sent = 0U;
  }
}

UI_KeyEvent UI_MenuScanKeys(uint32_t now_ms)
{
  uint8_t i;
  UI_KeyEvent best = UI_KEY_EVENT_NONE;

  for (i = 0U; i < (uint8_t)(sizeof(ui_keys) / sizeof(ui_keys[0])); i++) {
    UI_KeyState *key = &ui_keys[i];
    GPIO_PinState raw = HAL_GPIO_ReadPin(key->port, key->pin);

    if (raw != key->last_raw) {
      key->last_raw = raw;
      key->changed_ms = now_ms;
    }

    if (((uint32_t)(now_ms - key->changed_ms) >= UI_KEY_DEBOUNCE_MS) &&
        (key->stable != key->last_raw)) {
      key->stable = key->last_raw;
      if (key->stable == GPIO_PIN_RESET) {
        key->pressed = 1U;
        key->long_sent = 0U;
        key->pressed_ms = now_ms;
      } else {
        key->pressed = 0U;
        if (key->long_sent == 0U) {
          /* Prefer KEY0 short over KEY1 short when both fire. */
          if ((best == UI_KEY_EVENT_NONE) || (key->short_event < best)) {
            best = key->short_event;
          }
        }
      }
    }

    if ((key->pressed != 0U) && (key->long_sent == 0U) &&
        ((uint32_t)(now_ms - key->pressed_ms) >= UI_KEY_LONG_MS)) {
      key->long_sent = 1U;
      /* Long-press always wins over short press. */
      best = key->long_event;
    }
  }

  return best;
}
