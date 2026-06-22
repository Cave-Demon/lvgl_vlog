#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

/*************************** 引脚定义（你的硬件配置，完全保留） ***************************/
#define IO_NUM_MOSI   GPIO_NUM_23
#define IO_NUM_SCLK   GPIO_NUM_18
#define IO_NUM_CS     GPIO_NUM_15
#define IO_NUM_DC     GPIO_NUM_2
#define IO_NUM_RST    GPIO_NUM_4
#define IO_NUM_BKL    GPIO_NUM_25

/*************************** ST7789 指令宏定义 ***************************/
#define ST7789_NOP        0x00  // 无操作
#define ST7789_SWRESET    0x01  // 软件复位
#define ST7789_SLPOUT     0x11  // 退出睡眠模式
#define ST7789_COLMOD     0x3A  // 色彩模式设置
#define ST7789_MADCTL     0x36  // 内存访问控制（显示方向）
#define ST7789_CASET      0x2A  // 列地址设置
#define ST7789_RASET      0x2B  // 行地址设置
#define ST7789_RAMWR      0x2C  // 写显存数据
#define ST7789_INVON      0x21  // 颜色反转（IPS屏必需）
#define ST7789_DISPON     0x29  // 开启显示

/*************************** 屏幕参数配置 ***************************/
#define ST7789_WIDTH      240
#define ST7789_HEIGHT     240
#define X_OFFSET          0   // 240×240标准屏偏移量
#define Y_OFFSET          0

/*************************** 工具宏定义 ***************************/
// 修复：ESP32 GPIO为64位掩码，必须使用 1ULL 防止溢出
#define SET_IO_MSK(index) (1ULL << (index))

/*************************** SPI主机配置 ***************************/
#define ST7789_SPI_HOST   SPI2_HOST 

/*************************** 函数声明（补齐所有对外接口） ***************************/
void st7789_init(void);                        // 屏幕初始化
void st7789_set_backlight(uint8_t brightness);// 设置背光亮度
void st7789_fill_color(uint16_t color);        // 全屏填充颜色
// LVGL区域刷新接口
void st7789_fill_area(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const lv_color_t *color_p);