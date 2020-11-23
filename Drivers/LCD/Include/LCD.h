/**
  ******************************************************************************
  * @file           : LCD.h
  * @author			: Alexandr Olejnik <urgorka@gmail.com>
  * @brief          : Driver for 1602 lcd 2rows screen over I2C adapter
  ******************************************************************************
  *
  * Created on: Oct 31, 2020
  *
  ******************************************************************************
  */
#ifndef LCD_LCD_H_
#define LCD_LCD_H_

#include "stm32f1xx.h"

#define DEVICE_ADDRESS (0x3F << 1)
#define PIN_RS    (1 << 0)
#define PIN_EN    (1 << 2)
#define BACKLIGHT (1 << 3)
#define LCD_DELAY_MS 5

extern I2C_HandleTypeDef hi2c1;

enum LCD_CursorState {LCD_CursorState_Off, LCD_CursorState_On};
enum LCD_CursorBlink {LCD_CursorBlink_Off, LCD_CursorBlink_On};

extern void LCD_SendInternal(uint8_t data, uint8_t flags);
extern void LCD_SendCommand(uint8_t cmd);
extern void LCD_SendData(uint8_t data);
extern void LCD_Init(void);
extern void LCD_SendString(char *str);
extern void LCD_ShiftRight(void);
extern void LCD_ShiftLeft(void);
extern void LCD_On(void);
extern void LCD_Off(void);
extern void LCD_CursorMove(uint8_t line, uint8_t position);
extern void LCD_CursorHome(void);
extern void LCD_CursorState(enum LCD_CursorState state);
extern void LCD_CursorBlink(enum LCD_CursorBlink state);
extern void LCD_Clear(void);


#endif /* LCD_LCD_H_ */
