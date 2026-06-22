#include "st7789.h"
#include "freertos/projdefs.h"
#include "lvgl.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_err.h"

static spi_device_handle_t st7789_spi_handle;
static SemaphoreHandle_t st7789_mutex = NULL; // 互斥锁句柄

void st7789_hardware_init(void)
{
    // 创建互斥锁
    if (st7789_mutex == NULL) {
        st7789_mutex = xSemaphoreCreateMutex();
    }
    /*GPIO初始化*/
    gpio_config_t gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = SET_IO_MSK(IO_NUM_DC) | SET_IO_MSK(IO_NUM_RST),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&gpio_cfg);

    /*SPI总线初始化*/
    spi_bus_config_t spi_bus_cfg = {
        .miso_io_num = -1,
        .mosi_io_num = IO_NUM_MOSI,
        .sclk_io_num = IO_NUM_SCLK,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .max_transfer_sz = 32*1024  
    };
    spi_bus_initialize(ST7789_SPI_HOST,&spi_bus_cfg,SPI_DMA_CH_AUTO);

    /*SPI设备配置*/
    spi_device_interface_config_t spi_dev_cfg = {
        .clock_speed_hz = 40*1000*1000,  // 降低到 40MHz 降温
        .mode = 0,
        .spics_io_num = IO_NUM_CS,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY, 
        .pre_cb = NULL,
        .queue_size = 10  
    };
    spi_bus_add_device(ST7789_SPI_HOST,&spi_dev_cfg,&st7789_spi_handle);

    /*背光PWM初始化*/
    ledc_timer_config_t ledc_timer_cfg = {
        .clk_cfg = LEDC_USE_APB_CLK,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 20000,  
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer_cfg);

    ledc_channel_config_t led_channel_cfg = {
        .channel = LEDC_CHANNEL_0,
        .duty = 128,
        .gpio_num = IO_NUM_BKL,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&led_channel_cfg);

    /*硬件复位*/
    gpio_set_level(IO_NUM_RST,0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(IO_NUM_RST,1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

//设置屏幕背光
void st7789_set_backlight(uint8_t brightness)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
}

//发送命令
void st7789_send_cmd(uint8_t cmd)
{
    gpio_set_level(IO_NUM_DC,0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    ESP_ERROR_CHECK(spi_device_transmit(st7789_spi_handle,&t));
}

//发送数据
void st7789_send_dat(uint8_t *dat,size_t len)
{
    gpio_set_level(IO_NUM_DC,1);
    spi_transaction_t t = {
        .length = 8 * len,
        .tx_buffer = dat,
    };
    ESP_ERROR_CHECK(spi_device_transmit(st7789_spi_handle,&t));
}

void st7789_send_byte(uint8_t dat)
{
    st7789_send_dat(&dat, 1);
}

//ST7789初始化
// ST7789 初始化（纯净版：不旋转、不发灰、不模糊）
void st7789_init(void)
{
    // 1. 硬件初始化（GPIO/SPI/背光/复位，保留你之前修好的）
    st7789_hardware_init();

    // 2. 软件复位
    st7789_send_cmd(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(120));

    // 3. 退出睡眠
    st7789_send_cmd(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    // 4. 色彩模式：RGB565
    st7789_send_cmd(ST7789_COLMOD);
    st7789_send_byte(0x55);

    // ====================== 关键修复1：方向还原（横屏正常） ======================
    st7789_send_cmd(ST7789_MADCTL);
    st7789_send_byte(0x00);  // 还原成你原来的方向，不再旋转！

    // 5. IPS屏必须开反转（颜色才正常）
    st7789_send_cmd(ST7789_INVON);

    // 6. 设置显示区域
    st7789_send_cmd(ST7789_CASET);
    st7789_send_byte(X_OFFSET >> 8);
    st7789_send_byte(X_OFFSET & 0xFF);
    st7789_send_byte((X_OFFSET + ST7789_WIDTH - 1) >> 8);
    st7789_send_byte((X_OFFSET + ST7789_WIDTH - 1) & 0xFF);

    st7789_send_cmd(ST7789_RASET);
    st7789_send_byte(Y_OFFSET >> 8);
    st7789_send_byte(Y_OFFSET & 0xFF);
    st7789_send_byte((Y_OFFSET + ST7789_HEIGHT - 1) >> 8);
    st7789_send_byte((Y_OFFSET + ST7789_HEIGHT - 1) & 0xFF);

    // 7. 开显示
    st7789_send_cmd(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(20));

    // 背光亮度（正常即可）
    st7789_set_backlight(255);
}

//全屏填充
void st7789_fill_color(uint16_t color)
{
    uint8_t data[2] = {color >> 8, color & 0xFF};
    st7789_send_cmd(ST7789_RAMWR);
    for (uint32_t i = 0; i < ST7789_WIDTH * ST7789_HEIGHT; i++)
    {
        st7789_send_dat(data, 2);
    }
}

//设置显示窗口
static void st7789_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    st7789_send_cmd(ST7789_CASET);
    st7789_send_byte((x1 + X_OFFSET) >> 8);
    st7789_send_byte((x1 + X_OFFSET) & 0xFF);
    st7789_send_byte((x2 + X_OFFSET) >> 8);
    st7789_send_byte((x2 + X_OFFSET) & 0xFF);

    st7789_send_cmd(ST7789_RASET);
    st7789_send_byte((y1 + Y_OFFSET) >> 8);
    st7789_send_byte((y1 + Y_OFFSET) & 0xFF);
    st7789_send_byte((y2 + Y_OFFSET) >> 8);
    st7789_send_byte((y2 + Y_OFFSET) & 0xFF);
}

//LVGL区域刷新
void st7789_fill_area(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const lv_color_t *color_p)
{
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= ST7789_WIDTH)  x2 = ST7789_WIDTH - 1;
    if(y2 >= ST7789_HEIGHT) y2 = ST7789_HEIGHT - 1;
    if(x1 > x2 || y1 > y2) return;

    if (st7789_mutex != NULL && xSemaphoreTake(st7789_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        st7789_set_window(x1, y1, x2, y2);
        st7789_send_cmd(ST7789_RAMWR);

        uint32_t pixel_count = (x2 - x1 + 1) * (y2 - y1 + 1);
        st7789_send_dat((uint8_t *)color_p, pixel_count * 2);
        
        xSemaphoreGive(st7789_mutex);
    }
}