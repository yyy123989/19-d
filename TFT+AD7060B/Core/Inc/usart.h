#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
uint8_t USART1_Write(const uint8_t *data, uint16_t size);
uint8_t USART2_Write(const uint8_t *data, uint16_t size);
void USART1_TxIRQHandler(void);
void USART2_TxIRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
