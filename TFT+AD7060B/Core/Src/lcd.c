#include "lcd.h"

#include "main.h"
#include "tft_fsmc.h"

#include <string.h>

#define LCD_FONT_WIDTH  5U
#define LCD_FONT_HEIGHT 7U
#define LCD_CMD_SOFT_RESET   0x01U
#define LCD_CMD_SLEEP_OUT    0x11U
#define LCD_CMD_DISPLAY_OFF  0x28U
#define LCD_CMD_DISPLAY_ON   0x29U
#define LCD_CMD_COLUMN_ADDR  0x2AU
#define LCD_CMD_PAGE_ADDR    0x2BU
#define LCD_CMD_MEMORY_WRITE 0x2CU
#define LCD_CMD_MEMORY_READ  0x2EU
#define LCD_CMD_PIXEL_FORMAT 0x3AU
#define LCD_PIXEL_FORMAT_RGB565 0x55U

static const uint8_t lcd_font[][LCD_FONT_WIDTH] = {
    {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU}, /* 0 */
    {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U}, /* 1 */
    {0x42U, 0x61U, 0x51U, 0x49U, 0x46U}, /* 2 */
    {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U}, /* 3 */
    {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U}, /* 4 */
    {0x27U, 0x45U, 0x45U, 0x45U, 0x39U}, /* 5 */
    {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U}, /* 6 */
    {0x01U, 0x71U, 0x09U, 0x05U, 0x03U}, /* 7 */
    {0x36U, 0x49U, 0x49U, 0x49U, 0x36U}, /* 8 */
    {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU}, /* 9 */
    {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU}, /* A */
    {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U}, /* B */
    {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U}, /* C */
    {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU}, /* D */
    {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U}, /* E */
    {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U}, /* F */
    {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU}, /* G */
    {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU}, /* H */
    {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U}, /* I */
    {0x20U, 0x40U, 0x41U, 0x3FU, 0x01U}, /* J */
    {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U}, /* K */
    {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U}, /* L */
    {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU}, /* M */
    {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU}, /* N */
    {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU}, /* O */
    {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U}, /* P */
    {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU}, /* Q */
    {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U}, /* R */
    {0x46U, 0x49U, 0x49U, 0x49U, 0x31U}, /* S */
    {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U}, /* T */
    {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU}, /* U */
    {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU}, /* V */
    {0x3FU, 0x40U, 0x38U, 0x40U, 0x3FU}, /* W */
    {0x63U, 0x14U, 0x08U, 0x14U, 0x63U}, /* X */
    {0x07U, 0x08U, 0x70U, 0x08U, 0x07U}, /* Y */
    {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}, /* Z */
};

_lcd_dev lcddev = {
    LCD_WIDTH,
    LCD_HEIGHT,
    0x0000U,
    0U,
    0x002CU,
    0x002AU,
    0x002BU
};

uint32_t g_point_color = LCD_COLOR_RED;
uint32_t g_back_color = LCD_COLOR_WHITE;

static void LCD_WriteReg(uint16_t reg, uint16_t value)
{
    TFT_WriteCommand(reg);
    TFT_WriteData(value);
}

static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    TFT_WriteCommand(LCD_CMD_COLUMN_ADDR);
    TFT_WriteData(x0 >> 8);
    TFT_WriteData(x0 & 0xFFU);
    TFT_WriteData(x1 >> 8);
    TFT_WriteData(x1 & 0xFFU);

    TFT_WriteCommand(LCD_CMD_PAGE_ADDR);
    TFT_WriteData(y0 >> 8);
    TFT_WriteData(y0 & 0xFFU);
    TFT_WriteData(y1 >> 8);
    TFT_WriteData(y1 & 0xFFU);

    TFT_WriteCommand(LCD_CMD_MEMORY_WRITE);
}

static void LCD_SwapU16(uint16_t *a, uint16_t *b)
{
    uint16_t tmp = *a;
    *a = *b;
    *b = tmp;
}

static int32_t LCD_Abs32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static uint32_t LCD_Pow10(uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U) {
        result *= 10U;
    }

    return result;
}

static uint8_t LCD_SizeToScale(uint8_t size)
{
    if (size >= 32U) {
        return 4U;
    }
    if (size >= 24U) {
        return 3U;
    }
    if (size >= 12U) {
        return 2U;
    }
    return 1U;
}

static void LCD_DrawPixelClip(int32_t x, int32_t y, uint16_t color)
{
    if ((x >= 0) && (x < (int32_t)LCD_WIDTH) && (y >= 0) && (y < (int32_t)LCD_HEIGHT)) {
        LCD_DrawPixel((uint16_t)x, (uint16_t)y, color);
    }
}

static void LCD_DrawHLineClip(int32_t x, int32_t y, uint16_t len, uint16_t color)
{
    int32_t x0 = x;
    int32_t x1 = x + (int32_t)len - 1;

    if ((len == 0U) || (y < 0) || (y >= (int32_t)LCD_HEIGHT) ||
        (x1 < 0) || (x0 >= (int32_t)LCD_WIDTH)) {
        return;
    }

    if (x0 < 0) {
        x0 = 0;
    }
    if (x1 >= (int32_t)LCD_WIDTH) {
        x1 = (int32_t)LCD_WIDTH - 1;
    }

    LCD_FillRect((uint16_t)x0, (uint16_t)y, (uint16_t)(x1 - x0 + 1), 1U, color);
}

void LCD_Init(void)
{
    HAL_Delay(50);

    lcddev.width = LCD_WIDTH;
    lcddev.height = LCD_HEIGHT;
    lcddev.dir = 0U;
    lcddev.wramcmd = 0x002CU;
    lcddev.setxcmd = 0x002AU;
    lcddev.setycmd = 0x002BU;

    TFT_WriteCommand(LCD_CMD_SOFT_RESET);
    HAL_Delay(120);

    TFT_WriteCommand(LCD_CMD_SLEEP_OUT);
    HAL_Delay(120);

    LCD_WriteReg(LCD_CMD_PIXEL_FORMAT, LCD_PIXEL_FORMAT_RGB565);
    TFT_WriteCommand(LCD_CMD_DISPLAY_ON);
}

void LCD_Fill(uint16_t color)
{
    uint32_t i;

    LCD_SetWindow(0, 0, LCD_WIDTH - 1U, LCD_HEIGHT - 1U);

    for (i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++) {
        TFT_WriteData(color);
    }
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint32_t i;
    uint32_t count;

    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT) || (width == 0U) || (height == 0U)) {
        return;
    }

    if ((x + width) > LCD_WIDTH) {
        width = LCD_WIDTH - x;
    }

    if ((y + height) > LCD_HEIGHT) {
        height = LCD_HEIGHT - y;
    }

    LCD_SetWindow(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
    count = (uint32_t)width * height;

    for (i = 0; i < count; i++) {
        TFT_WriteData(color);
    }
}

void LCD_ColorFill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, const uint16_t *colors)
{
    uint32_t i;
    uint32_t count;

    if (colors == NULL) {
        return;
    }

    if (sx > ex) {
        LCD_SwapU16(&sx, &ex);
    }
    if (sy > ey) {
        LCD_SwapU16(&sy, &ey);
    }

    if ((sx >= LCD_WIDTH) || (sy >= LCD_HEIGHT)) {
        return;
    }
    if (ex >= LCD_WIDTH) {
        ex = LCD_WIDTH - 1U;
    }
    if (ey >= LCD_HEIGHT) {
        ey = LCD_HEIGHT - 1U;
    }

    /* Note: caller must ensure 'colors' array has at least count elements. */
    LCD_SetWindow(sx, sy, ex, ey);
    count = (uint32_t)(ex - sx + 1U) * (uint32_t)(ey - sy + 1U);

    for (i = 0U; i < count; i++) {
        TFT_WriteData(colors[i]);
    }
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return;
    }

    LCD_SetWindow(x, y, x, y);
    TFT_WriteData(color);
}

uint16_t LCD_ReadPoint(uint16_t x, uint16_t y)
{
    volatile uint16_t dummy;

    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return 0U;
    }

    LCD_SetWindow(x, y, x, y);
    TFT_WriteCommand(LCD_CMD_MEMORY_READ);
    dummy = TFT_ReadData();
    (void)dummy;
    return TFT_ReadData();
}

void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0U) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) {
        return;
    }

    LCD_FillRect(x, y, len, 1U, color);
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int32_t dx = LCD_Abs32((int32_t)x2 - (int32_t)x1);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t dy = -LCD_Abs32((int32_t)y2 - (int32_t)y1);
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx + dy;
    int32_t e2;
    int32_t x = x1;
    int32_t y = y1;

    while (1) {
        LCD_DrawPixelClip(x, y, color);
        if ((x == (int32_t)x2) && (y == (int32_t)y2)) {
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
    }
}

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}

void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int32_t x = 0;
    int32_t y = r;
    int32_t d = 3 - (2 * (int32_t)r);

    while (x <= y) {
        LCD_DrawPixelClip((int32_t)x0 + x, (int32_t)y0 + y, color);
        LCD_DrawPixelClip((int32_t)x0 + y, (int32_t)y0 + x, color);
        LCD_DrawPixelClip((int32_t)x0 - x, (int32_t)y0 + y, color);
        LCD_DrawPixelClip((int32_t)x0 - y, (int32_t)y0 + x, color);
        LCD_DrawPixelClip((int32_t)x0 + x, (int32_t)y0 - y, color);
        LCD_DrawPixelClip((int32_t)x0 + y, (int32_t)y0 - x, color);
        LCD_DrawPixelClip((int32_t)x0 - x, (int32_t)y0 - y, color);
        LCD_DrawPixelClip((int32_t)x0 - y, (int32_t)y0 - x, color);

        if (d < 0) {
            d += (4 * x) + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    int32_t xi = 0;
    int32_t yi = r;
    int32_t d = 3 - (2 * (int32_t)r);

    while (xi <= yi) {
        if (yi > 0) {
            LCD_DrawHLineClip((int32_t)x - xi, (int32_t)y + yi, (uint16_t)((2 * xi) + 1), color);
            LCD_DrawHLineClip((int32_t)x - xi, (int32_t)y - yi, (uint16_t)((2 * xi) + 1), color);
        }
        LCD_DrawHLineClip((int32_t)x - yi, (int32_t)y + xi, (uint16_t)((2 * yi) + 1), color);
        if (xi > 0) {
            LCD_DrawHLineClip((int32_t)x - yi, (int32_t)y - xi, (uint16_t)((2 * yi) + 1), color);
        }

        if (d < 0) {
            d += (4 * xi) + 6;
        } else {
            d += 4 * (xi - yi) + 10;
            yi--;
        }
        xi++;
    }
}

static const uint8_t *LCD_GetGlyph(char ch)
{
    static const uint8_t blank[LCD_FONT_WIDTH] = {0U, 0U, 0U, 0U, 0U};
    static const uint8_t colon[LCD_FONT_WIDTH] = {0x00U, 0x36U, 0x36U, 0x00U, 0x00U};
    static const uint8_t minus[LCD_FONT_WIDTH] = {0x08U, 0x08U, 0x08U, 0x08U, 0x08U};
    static const uint8_t plus[LCD_FONT_WIDTH] = {0x08U, 0x08U, 0x3EU, 0x08U, 0x08U};
    static const uint8_t dot[LCD_FONT_WIDTH] = {0x00U, 0x60U, 0x60U, 0x00U, 0x00U};
    static const uint8_t comma[LCD_FONT_WIDTH] = {0x00U, 0x40U, 0x20U, 0x00U, 0x00U};
    static const uint8_t slash[LCD_FONT_WIDTH] = {0x40U, 0x20U, 0x10U, 0x08U, 0x04U};
    static const uint8_t percent[LCD_FONT_WIDTH] = {0x23U, 0x13U, 0x08U, 0x64U, 0x62U};
    static const uint8_t equal[LCD_FONT_WIDTH] = {0x14U, 0x14U, 0x14U, 0x14U, 0x14U};
    static const uint8_t less_than[LCD_FONT_WIDTH] = {0x08U, 0x14U, 0x22U, 0x41U, 0x00U};
    static const uint8_t greater_than[LCD_FONT_WIDTH] = {0x00U, 0x41U, 0x22U, 0x14U, 0x08U};
    static const uint8_t left_paren[LCD_FONT_WIDTH] = {0x00U, 0x1CU, 0x22U, 0x41U, 0x00U};
    static const uint8_t right_paren[LCD_FONT_WIDTH] = {0x00U, 0x41U, 0x22U, 0x1CU, 0x00U};
    static const uint8_t underscore[LCD_FONT_WIDTH] = {0x40U, 0x40U, 0x40U, 0x40U, 0x40U};

    if ((ch >= '0') && (ch <= '9')) {
        return lcd_font[ch - '0'];
    }

    if ((ch >= 'A') && (ch <= 'Z')) {
        return lcd_font[10 + (ch - 'A')];
    }

    if ((ch >= 'a') && (ch <= 'z')) {
        return lcd_font[10 + (ch - 'a')];
    }

    if (ch == ':') {
        return colon;
    }

    if (ch == '-') {
        return minus;
    }

    if (ch == '+') {
        return plus;
    }

    if (ch == '.') {
        return dot;
    }

    if (ch == ',') {
        return comma;
    }

    if (ch == '/') {
        return slash;
    }

    if (ch == '%') {
        return percent;
    }

    if (ch == '=') {
        return equal;
    }

    if (ch == '<') {
        return less_than;
    }

    if (ch == '>') {
        return greater_than;
    }

    if (ch == '(') {
        return left_paren;
    }

    if (ch == ')') {
        return right_paren;
    }

    if (ch == '_') {
        return underscore;
    }

    return blank;
}

static void LCD_DrawCharInternal(uint16_t x,
                                 uint16_t y,
                                 char ch,
                                 uint16_t color,
                                 uint16_t bg_color,
                                 uint8_t scale,
                                 uint8_t transparent)
{
    const uint8_t *glyph = LCD_GetGlyph(ch);
    uint8_t col;
    uint8_t row;
    uint8_t sx;
    uint8_t sy;
    uint16_t char_w;
    uint16_t char_h;
    uint16_t pixel;

    if (scale == 0U) {
        scale = 1U;
    }

    char_w = (uint16_t)(LCD_FONT_WIDTH * scale);
    char_h = (uint16_t)(LCD_FONT_HEIGHT * scale);

    if (transparent != 0U) {
        /* Transparent: draw foreground pixels individually to avoid TFT GRAM
         * column misalignment (GRAM auto-increments only on write). */
        for (row = 0U; row < LCD_FONT_HEIGHT; row++) {
            for (col = 0U; col < LCD_FONT_WIDTH; col++) {
                if ((glyph[col] & (1U << row)) == 0U) continue;
                for (sy = 0U; sy < scale; sy++) {
                    for (sx = 0U; sx < scale; sx++) {
                        LCD_DrawPixel((uint16_t)(x + (uint16_t)col * scale + sx),
                                      (uint16_t)(y + (uint16_t)row * scale + sy),
                                      color);
                    }
                }
            }
        }
        return;
    }

    /* Opaque: set window once, stream all pixels */
    LCD_SetWindow(x, y, (uint16_t)(x + char_w - 1U), (uint16_t)(y + char_h - 1U));

    for (row = 0U; row < LCD_FONT_HEIGHT; row++) {
        for (sy = 0U; sy < scale; sy++) {
            for (col = 0U; col < LCD_FONT_WIDTH; col++) {
                pixel = ((glyph[col] & (1U << row)) != 0U) ? color : bg_color;
                for (sx = 0U; sx < scale; sx++) {
                    TFT_WriteData((uint16_t)pixel);
                }
            }
        }
    }
}

void LCD_DrawString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg_color, uint8_t scale)
{
    uint16_t len;
    uint16_t char_h;
    uint32_t total_w;
    const char *cursor;
    const uint8_t *glyph;
    uint8_t row;
    uint8_t sy;
    uint8_t col;
    uint8_t sx;
    uint16_t pixel;

    if ((text == NULL) || (*text == '\0')) {
        return;
    }

    if (scale == 0U) {
        scale = 1U;
    }

    len = (uint16_t)strlen(text);
    char_h = (uint16_t)(LCD_FONT_HEIGHT * scale);
    total_w = (uint32_t)(LCD_FONT_WIDTH + 1U) * scale * len;

    /* Clamp to screen bounds to avoid writing out-of-range TFT column registers. */
    if ((x + (uint16_t)total_w) > LCD_WIDTH) {
        total_w = (uint32_t)(LCD_WIDTH - x);
    }
    if (total_w == 0U) {
        return;
    }

    /* One window for the entire string, avoiding per-character SetWindow overhead. */
    LCD_SetWindow(x, y, (uint16_t)(x + (uint16_t)total_w - 1U), (uint16_t)(y + char_h - 1U));

    /* LCD GRAM increments row-major inside the window, so stream full text rows. */
    for (row = 0U; row < LCD_FONT_HEIGHT; row++) {
        for (sy = 0U; sy < scale; sy++) {
            cursor = text;
            while (*cursor != '\0') {
                glyph = LCD_GetGlyph(*cursor);
                for (col = 0U; col < LCD_FONT_WIDTH; col++) {
                    pixel = ((glyph[col] & (1U << row)) != 0U) ? color : bg_color;
                    for (sx = 0U; sx < scale; sx++) {
                        TFT_WriteData(pixel);
                    }
                }
                for (sx = 0U; sx < scale; sx++) {
                    TFT_WriteData(bg_color);
                }
                cursor++;
            }
        }
    }
}

void LCD_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    LCD_DrawCharInternal(x, y, chr, color, (uint16_t)g_back_color, LCD_SizeToScale(size), mode);
}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    LCD_ShowXNum(x, y, num, len, size, 0U, color);
}

void LCD_ShowXNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t i;
    uint8_t scale = LCD_SizeToScale(size);
    uint16_t step = (uint16_t)((LCD_FONT_WIDTH + 1U) * scale);
    uint8_t fill_zero = ((mode & 0x80U) != 0U) ? 1U : 0U;
    uint8_t transparent = ((mode & 0x01U) != 0U) ? 1U : 0U;
    uint8_t show_started = fill_zero;

    if (len == 0U) {
        return;
    }

    for (i = 0U; i < len; i++) {
        uint32_t divisor = LCD_Pow10((uint8_t)(len - i - 1U));
        uint8_t digit = (uint8_t)((num / divisor) % 10U);
        char chr;

        if ((digit == 0U) && (show_started == 0U) && (i < (uint8_t)(len - 1U))) {
            chr = ' ';
        } else {
            show_started = 1U;
            chr = (char)('0' + digit);
        }

        LCD_DrawCharInternal((uint16_t)(x + ((uint16_t)i * step)),
                             y,
                             chr,
                             color,
                             (uint16_t)g_back_color,
                             scale,
                             transparent);
    }
}

void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *text, uint16_t color)
{
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    uint8_t scale = LCD_SizeToScale(size);
    uint16_t step_x = (uint16_t)((LCD_FONT_WIDTH + 1U) * scale);
    uint16_t step_y = (uint16_t)((LCD_FONT_HEIGHT + 1U) * scale);
    uint16_t right = (uint16_t)(x + width);
    uint16_t bottom = (uint16_t)(y + height);

    if ((text == NULL) || (width == 0U) || (height == 0U)) {
        return;
    }

    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = x;
            cursor_y = (uint16_t)(cursor_y + step_y);
            text++;
            continue;
        }

        if ((cursor_x + step_x) > right) {
            cursor_x = x;
            cursor_y = (uint16_t)(cursor_y + step_y);
        }

        if ((cursor_y + (LCD_FONT_HEIGHT * scale)) > bottom) {
            break;
        }

        LCD_DrawCharInternal(cursor_x, cursor_y, *text, color, (uint16_t)g_back_color, scale, 0U);
        cursor_x = (uint16_t)(cursor_x + step_x);
        text++;
    }
}

#if LCD_ENABLE_COMPAT_API
void lcd_wr_data(volatile uint16_t data)
{
    TFT_WriteData((uint16_t)data);
}

void lcd_wr_regno(volatile uint16_t regno)
{
    TFT_WriteCommand((uint16_t)regno);
}

void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD_WriteReg(regno, data);
}

void lcd_init(void)
{
    LCD_Init();
}

void lcd_display_on(void)
{
    TFT_WriteCommand(LCD_CMD_DISPLAY_ON);
}

void lcd_display_off(void)
{
    TFT_WriteCommand(LCD_CMD_DISPLAY_OFF);
}

void lcd_scan_dir(uint8_t dir)
{
    /* Silently clamp invalid direction to default — caller gets no error indication. */
    if (dir > D2U_R2L) {
        dir = DFT_SCAN_DIR;
    }

    lcddev.dir = dir;
}

void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;
    lcddev.width = LCD_WIDTH;
    lcddev.height = LCD_HEIGHT;
}

void lcd_ssd_backlight_set(uint8_t pwm)
{
    (void)pwm;
}

void lcd_write_ram_prepare(void)
{
    TFT_WriteCommand(LCD_CMD_MEMORY_WRITE);
}

void lcd_set_cursor(uint16_t x, uint16_t y)
{
    LCD_SetWindow(x, y, x, y);
}

void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t ex;
    uint16_t ey;

    if ((width == 0U) || (height == 0U) || (sx >= LCD_WIDTH) || (sy >= LCD_HEIGHT)) {
        return;
    }

    ex = (uint16_t)(sx + width - 1U);
    ey = (uint16_t)(sy + height - 1U);

    if ((ex < sx) || (ex >= LCD_WIDTH)) {
        ex = LCD_WIDTH - 1U;
    }
    if ((ey < sy) || (ey >= LCD_HEIGHT)) {
        ey = LCD_HEIGHT - 1U;
    }

    LCD_SetWindow(sx, sy, ex, ey);
}

void lcd_clear(uint16_t color)
{
    g_back_color = color;
    LCD_Fill(color);
}

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    if (sx > ex) {
        LCD_SwapU16(&sx, &ex);
    }
    if (sy > ey) {
        LCD_SwapU16(&sy, &ey);
    }

    LCD_FillRect(sx, sy, (uint16_t)(ex - sx + 1U), (uint16_t)(ey - sy + 1U), (uint16_t)color);
}

void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    LCD_ColorFill(sx, sy, ex, ey, color);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    LCD_DrawPixel(x, y, (uint16_t)color);
}

uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    return LCD_ReadPoint(x, y);
}

void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    LCD_DrawHLine(x, y, len, color);
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawLine(x1, y1, x2, y2, color);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawRectangle(x1, y1, x2, y2, color);
}

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    LCD_DrawCircle(x0, y0, r, color);
}

void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    LCD_FillCircle(x, y, r, color);
}

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    LCD_ShowChar(x, y, chr, size, mode, color);
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    LCD_ShowNum(x, y, num, len, size, color);
}

void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    LCD_ShowXNum(x, y, num, len, size, mode, color);
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *text, uint16_t color)
{
    LCD_ShowString(x, y, width, height, size, text, color);
}
#endif
