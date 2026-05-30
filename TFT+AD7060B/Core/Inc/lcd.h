#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define LCD_WIDTH  240U
#define LCD_HEIGHT 320U

#ifndef LCD_ENABLE_COMPAT_API
#define LCD_ENABLE_COMPAT_API 0
#endif

#define LCD_COLOR_BLACK 0x0000U
#define LCD_COLOR_BLUE  0x001FU
#define LCD_COLOR_GREEN 0x07E0U
#define LCD_COLOR_RED   0xF800U
#define LCD_COLOR_WHITE 0xFFFFU
#define LCD_COLOR_MAGENTA 0xF81FU
#define LCD_COLOR_YELLOW  0xFFE0U
#define LCD_COLOR_CYAN    0x07FFU
#define LCD_COLOR_BROWN   0xBC40U
#define LCD_COLOR_BRRED   0xFC07U
#define LCD_COLOR_GRAY    0x8430U
#define LCD_COLOR_DARKBLUE   0x01CFU
#define LCD_COLOR_LIGHTBLUE  0x7D7CU
#define LCD_COLOR_GRAYBLUE   0x5458U
#define LCD_COLOR_LIGHTGREEN 0x841FU
#define LCD_COLOR_LGRAY      0xC618U
#define LCD_COLOR_LGRAYBLUE  0xA651U
#define LCD_COLOR_LBBLUE     0x2B12U

#define L2R_U2D 0U
#define L2R_D2U 1U
#define R2L_U2D 2U
#define R2L_D2U 3U
#define U2D_L2R 4U
#define U2D_R2L 5U
#define D2U_L2R 6U
#define D2U_R2L 7U
#define DFT_SCAN_DIR L2R_U2D

#define WHITE   LCD_COLOR_WHITE
#define BLACK   LCD_COLOR_BLACK
#define BLUE    LCD_COLOR_BLUE
#define RED     LCD_COLOR_RED
#define GREEN   LCD_COLOR_GREEN
#define MAGENTA LCD_COLOR_MAGENTA
#define YELLOW  LCD_COLOR_YELLOW
#define CYAN    LCD_COLOR_CYAN
#define BROWN   LCD_COLOR_BROWN
#define BRRED   LCD_COLOR_BRRED
#define GRAY    LCD_COLOR_GRAY
#define DARKBLUE   LCD_COLOR_DARKBLUE
#define LIGHTBLUE  LCD_COLOR_LIGHTBLUE
#define GRAYBLUE   LCD_COLOR_GRAYBLUE
#define LIGHTGREEN LCD_COLOR_LIGHTGREEN
#define LGRAY      LCD_COLOR_LGRAY
#define LGRAYBLUE  LCD_COLOR_LGRAYBLUE
#define LBBLUE     LCD_COLOR_LBBLUE

#define LCD_BL(x) do { (void)(x); } while (0)

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t id;
    uint8_t dir;
    uint16_t wramcmd;
    uint16_t setxcmd;
    uint16_t setycmd;
} _lcd_dev;

extern _lcd_dev lcddev;
extern uint32_t g_point_color;
extern uint32_t g_back_color;

void LCD_Init(void);
void LCD_Fill(uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_ColorFill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, const uint16_t *colors);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
uint16_t LCD_ReadPoint(uint16_t x, uint16_t y);
void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void LCD_DrawString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg_color, uint8_t scale);
void LCD_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void LCD_ShowXNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *text, uint16_t color);

#if LCD_ENABLE_COMPAT_API
void lcd_wr_data(volatile uint16_t data);
void lcd_wr_regno(volatile uint16_t regno);
void lcd_write_reg(uint16_t regno, uint16_t data);
void lcd_init(void);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_scan_dir(uint8_t dir);
void lcd_display_dir(uint8_t dir);
void lcd_ssd_backlight_set(uint8_t pwm);
void lcd_write_ram_prepare(void);
void lcd_set_cursor(uint16_t x, uint16_t y);
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void lcd_clear(uint16_t color);
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t lcd_read_point(uint16_t x, uint16_t y);
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *text, uint16_t color);
#endif

#endif
