/**
  ******************************************************************************
  * @file           : LCD.c
  * @author			: Alexandr Olejnik <urgorka@gmail.com>
  * @brief          : Driver for 1602 lcd 2rows screen over I2C adapter
  ******************************************************************************
  *
  * Created on: Oct 31, 2020
  *
  ******************************************************************************
  */
#include "LCD.h"
#include "stm32f1xx_hal_def.h"

uint8_t LCD_DISP_CURS  = 0b00001000;
uint8_t LCD_Disp_OnOff = 0b00000000;
uint8_t LCD_Curs_OnOff = 0b00000000;
uint8_t LCD_Curs_Blink = 0b00000000;

void LCD_SendInternal(uint8_t data, uint8_t flags) {
	HAL_StatusTypeDef res;
	// бесконечный цикл
	for (;;) {
		// проверяем, готово ли устройство для связи
		res = HAL_I2C_IsDeviceReady(&hi2c1, DEVICE_ADDRESS, 1, HAL_MAX_DELAY);
		if (res == HAL_OK)
			break;
	}
	uint8_t up = data & 0xF0;
	uint8_t lo = (data << 4) & 0xF0;

	uint8_t data_arr[4];
	// 4-7 биты содержат информацию, биты 0-3 настраивают работу дисплея
	data_arr[0] = up | flags | BACKLIGHT | PIN_EN;
	// дублирование сигнала, на выводе Е в этот раз 0
	data_arr[1] = up | flags | BACKLIGHT;
	data_arr[2] = lo | flags | BACKLIGHT | PIN_EN;
	data_arr[3] = lo | flags | BACKLIGHT;

	HAL_I2C_Master_Transmit(&hi2c1, DEVICE_ADDRESS, data_arr, sizeof(data_arr), HAL_MAX_DELAY);
	HAL_Delay(LCD_DELAY_MS);
}
void LCD_SendCommand(uint8_t cmd) {
    LCD_SendInternal(cmd, 0);
}
void LCD_SendData(uint8_t data) {
    LCD_SendInternal(data, PIN_RS);
}
void LCD_Init(void) {
    LCD_SendCommand(0b00110000);// 4-bit mode, 2 lines, 5x7 format
    LCD_CursorHome();
    LCD_On(); // display on
    LCD_Clear(); // clear display (optional here)
}
void LCD_SendString(char *str) {
    while(*str) {
        LCD_SendData((uint8_t)(*str));
        str++;
    }
}
void LCD_ShiftRight(void) {
	LCD_SendCommand(0b00011100);
}
void LCD_ShiftLeft(void) {
	LCD_SendCommand(0b00011000);
}
/*
 * Turn on lcd crystals (not backlight!)
 */
void LCD_On(void) {
	LCD_Disp_OnOff = 0b00000100;
	LCD_SendCommand(LCD_DISP_CURS | LCD_Disp_OnOff | LCD_Curs_OnOff | LCD_Curs_Blink);
}
/*
 * Turn off lcd crystals (not backlight!)
 */
void LCD_Off(void) {
	LCD_Disp_OnOff = 0b00000000;
	LCD_SendCommand(LCD_DISP_CURS | LCD_Disp_OnOff | LCD_Curs_OnOff | LCD_Curs_Blink);
}
void LCD_CursorMove(uint8_t line, uint8_t position){
	if (0 != line) line--;
	if (0 != position) position--;
	if (15 < position) position = 15;
	uint8_t byte = 0b10000000;
	byte = byte | (line << 6);
	byte += position;
	// 0b10000000 // 1st line
	// 0b11000000 // 2nd line
	LCD_SendCommand(byte);
}
void LCD_CursorHome(void) {
	LCD_SendCommand(0b00000010);
}
void LCD_CursorState(enum LCD_CursorState state) {
	LCD_Curs_OnOff = state << 1;
	LCD_SendCommand(LCD_DISP_CURS | LCD_Disp_OnOff | LCD_Curs_OnOff | LCD_Curs_Blink);
}
void LCD_CursorBlink(enum LCD_CursorBlink state) {
	LCD_Curs_Blink = state;
	LCD_SendCommand(LCD_DISP_CURS | LCD_Disp_OnOff | LCD_Curs_OnOff | LCD_Curs_Blink);
}
void LCD_Clear(void) {
	LCD_SendCommand(0b00000001);
}
