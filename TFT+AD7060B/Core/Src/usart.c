#include "usart.h"

#define USART1_BAUDRATE 115200UL
#define USART2_BAUDRATE 115200UL
#define USART1_TX_BUFFER_SIZE 1024U
#define USART2_TX_BUFFER_SIZE 2048U
#define USART1_RX_BUFFER_SIZE 128U

typedef struct {
  USART_TypeDef *instance;
  volatile uint16_t head;
  volatile uint16_t tail;
  uint16_t size;
  uint8_t *buffer;
} USART_TxQueue;

static uint8_t usart1_tx_buffer[USART1_TX_BUFFER_SIZE];
static uint8_t usart2_tx_buffer[USART2_TX_BUFFER_SIZE];
static uint8_t usart1_rx_buffer[USART1_RX_BUFFER_SIZE];
static USART_TxQueue usart1_tx = {USART1, 0U, 0U, USART1_TX_BUFFER_SIZE, usart1_tx_buffer};
static USART_TxQueue usart2_tx = {USART2, 0U, 0U, USART2_TX_BUFFER_SIZE, usart2_tx_buffer};
static volatile uint16_t usart1_rx_head;
static volatile uint16_t usart1_rx_tail;

static void USART_TxQueueReset(USART_TxQueue *queue)
{
  queue->head = 0U;
  queue->tail = 0U;
}

static void USART1_RxQueueReset(void)
{
  usart1_rx_head = 0U;
  usart1_rx_tail = 0U;
}

static uint16_t USART_TxQueueNext(const USART_TxQueue *queue, uint16_t index)
{
  index++;
  if (index >= queue->size) {
    index = 0U;
  }
  return index;
}

static uint16_t USART1_RxQueueNext(uint16_t index)
{
  index++;
  if (index >= USART1_RX_BUFFER_SIZE) {
    index = 0U;
  }
  return index;
}

static void USART1_RxPush(uint8_t data)
{
  uint16_t next = USART1_RxQueueNext(usart1_rx_head);

  if (next == usart1_rx_tail) {
    return;
  }

  usart1_rx_buffer[usart1_rx_head] = data;
  usart1_rx_head = next;
}

static uint16_t USART_TxQueueFree(const USART_TxQueue *queue)
{
  uint16_t used;

  if (queue->head >= queue->tail) {
    used = (uint16_t)(queue->head - queue->tail);
  } else {
    used = (uint16_t)(queue->size - queue->tail + queue->head);
  }

  return (uint16_t)(queue->size - used - 1U);
}

static uint8_t USART_WriteQueued(USART_TxQueue *queue, const uint8_t *data, uint16_t size)
{
  uint16_t i;
  uint16_t next;
  uint32_t primask;

  if ((queue == 0) || (data == 0) || (size == 0U)) {
    return 0U;
  }

  primask = __get_PRIMASK();
  __disable_irq();

  if (USART_TxQueueFree(queue) < size) {
    if (primask == 0U) {
      __enable_irq();
    }
    return 0U;
  }

  for (i = 0U; i < size; i++) {
    next = USART_TxQueueNext(queue, queue->head);
    queue->buffer[queue->head] = data[i];
    queue->head = next;
  }

  queue->instance->CR1 |= USART_CR1_TXEIE;

  if (primask == 0U) {
    __enable_irq();
  }

  return 1U;
}

static void USART_TxIRQ(USART_TxQueue *queue)
{
  if ((queue->instance->SR & USART_SR_TXE) == 0U) {
    return;
  }

  if (queue->head == queue->tail) {
    queue->instance->CR1 &= ~USART_CR1_TXEIE;
    return;
  }

  queue->instance->DR = queue->buffer[queue->tail];
  queue->tail = USART_TxQueueNext(queue, queue->tail);
}

void MX_USART1_UART_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t pclk;
  uint32_t brr;

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  GPIO_InitStruct.Pin = USART1_TX_Pin | USART1_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  USART1->CR1 = 0U;
  USART1->CR2 = 0U;
  USART1->CR3 = 0U;
  USART_TxQueueReset(&usart1_tx);
  USART1_RxQueueReset();

  pclk = HAL_RCC_GetPCLK2Freq();
  brr = (pclk + (USART1_BAUDRATE / 2UL)) / USART1_BAUDRATE;
  if (brr == 0UL) {
    brr = 1UL;
  }
  USART1->BRR = (uint16_t)brr;

  USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

  HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void MX_USART2_UART_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t pclk;
  uint32_t brr;

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();

  GPIO_InitStruct.Pin = USART2_SCREEN_TX_Pin | USART2_SCREEN_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  USART2->CR1 = 0U;
  USART2->CR2 = 0U;
  USART2->CR3 = 0U;
  USART_TxQueueReset(&usart2_tx);

  pclk = HAL_RCC_GetPCLK1Freq();
  brr = (pclk + (USART2_BAUDRATE / 2UL)) / USART2_BAUDRATE;
  if (brr == 0UL) {
    brr = 1UL;
  }
  USART2->BRR = (uint16_t)brr;

  USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

  HAL_NVIC_SetPriority(USART2_IRQn, 3, 1);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

uint8_t USART1_Write(const uint8_t *data, uint16_t size)
{
  return USART_WriteQueued(&usart1_tx, data, size);
}

uint8_t USART2_Write(const uint8_t *data, uint16_t size)
{
  return USART_WriteQueued(&usart2_tx, data, size);
}

uint8_t USART1_ReadByte(uint8_t *data)
{
  uint32_t primask;

  if (data == 0) {
    return 0U;
  }

  primask = __get_PRIMASK();
  __disable_irq();

  if (usart1_rx_head == usart1_rx_tail) {
    if (primask == 0U) {
      __enable_irq();
    }
    return 0U;
  }

  *data = usart1_rx_buffer[usart1_rx_tail];
  usart1_rx_tail = USART1_RxQueueNext(usart1_rx_tail);

  if (primask == 0U) {
    __enable_irq();
  }

  return 1U;
}

void USART1_TxIRQHandler(void)
{
  uint32_t sr = USART1->SR;

  if ((sr & (USART_SR_RXNE | USART_SR_ORE)) != 0U) {
    uint8_t data = (uint8_t)USART1->DR;
    if ((sr & USART_SR_RXNE) != 0U) {
      USART1_RxPush(data);
    }
  }

  USART_TxIRQ(&usart1_tx);
}

void USART2_TxIRQHandler(void)
{
  USART_TxIRQ(&usart2_tx);
}
