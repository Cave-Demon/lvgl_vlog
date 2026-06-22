#include "lvgl.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "st7789.h"

#define TAG       "lv_port"
#define LCD_WIDE  240
#define LCD_HEIGHT 240

static lv_disp_drv_t    disp_drv;

// LVGL刷新回调
void display_flush(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    st7789_fill_area(x1, y1, x2, y2, color_p);
    lv_disp_flush_ready(disp_drv);
}

// LVGL显示驱动初始化
void lv_disp_init(void)
{
    static lv_disp_draw_buf_t disp_buf;
const size_t disp_buf_size = LCD_WIDE * LCD_HEIGHT / 4;  // 改为1/4屏大小，提升渲染效率
lv_color_t *disp1 = heap_caps_malloc(disp_buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
lv_color_t *disp2 = heap_caps_malloc(disp_buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

// 修复内存申请失败判断（原代码变量名错误）
if(!disp1 || !disp2)  // 原代码写的是!disp1 || !disp2 → 修复为正确变量名
{
    ESP_LOGE(TAG,"显示缓冲区内存分配失败！");
    return;
}
    ESP_LOGI(TAG,"显示缓冲区分配成功");

    lv_disp_draw_buf_init(&disp_buf,disp1,disp2,disp_buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDE;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = NULL;

    lv_disp_drv_register(&disp_drv);
    ESP_LOGI(TAG,"LVGL显示驱动初始化完成");
}

