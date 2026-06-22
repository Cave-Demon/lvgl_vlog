#include "lvgl.h"
#include "esp_log.h"

LV_IMG_DECLARE(cxk1)

static lv_obj_t* s_cxk_image;
static lv_obj_t* s_ip_label;

void ui_home_create(void)
{
    //创建cxk图片
    s_cxk_image = lv_img_create(lv_scr_act());
    lv_img_set_src(s_cxk_image,&cxk1);
    lv_obj_set_pos(s_cxk_image,15,16);

    //创建IP标签
    s_ip_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_ip_label, "Connecting Wi-Fi...");
    lv_obj_set_style_text_color(s_ip_label, lv_color_hex(0xFFFFFF), 0); // 白色文字
    lv_obj_align(s_ip_label, LV_ALIGN_BOTTOM_MID, 0, -10); // 底部居中
}

void ui_set_ip_label(const char* ip_str)
{
    if (s_ip_label) {
        lv_label_set_text_fmt(s_ip_label, "IP: %s", ip_str);
    }
}