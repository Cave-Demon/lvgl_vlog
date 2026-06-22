#ifndef __LV_PORT_DISP_H__
#define __LV_PORT_DISP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 * 包含头文件
 *********************/
#include "lvgl.h"

/*********************
 * 屏幕宏定义
 *********************/
#define LCD_WIDE      240    // LCD 宽度
#define LCD_HEIGHT    240    // LCD 高度

/*********************
 * 函数声明
 *********************/

/**
 * @brief  LVGL 显示驱动初始化
 * @note   需在 st7789_init() 之后调用
 * @return 无
 */
void lv_disp_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LV_PORT_DISP_H__ */