#include "lvgl.h"
#include <stdio.h>
#include "st7789.h"
#include "esp_log.h"
#include "lv_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_cxk.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/sockets.h" // 添加缺失的套接字头文件

// 定义数据包头结构体
typedef struct {
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
} udp_packet_header_t;

// 请在这里替换您的Wi-Fi SSID和密码
#define WIFI_SSID      "METAMECHBOOK_9656"
#define WIFI_PASS      "13146666"

#define UDP_PORT 8080

// --- 函数前向声明 ---
static void udp_server_task(void *pvParameters);
static void lvgl_main_task(void *pvParameters); // 新增 LVGL 任务
static void wifi_init_sta(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);


// 声明外部图片变量
LV_IMG_DECLARE(cxk1);  

#define TAG "main"

void app_main(void)
{
    // 1. NVS初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Wi-Fi初始化 (必须最先初始化网络栈，否则 socket() 会崩溃)
    wifi_init_sta();

    // 3. 启动用户业务逻辑（全放在 Core 1）
    // LVGL 渲染任务
    xTaskCreatePinnedToCore(lvgl_main_task, "lvgl_main", 8192, NULL, 4, NULL, 1);
    
    // UDP 接收任务 (优先级设为 5，确保不丢包)
    xTaskCreatePinnedToCore(udp_server_task, "udp_server", 8192, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "System cores initialized. Network on Core 0, App on Core 1.");
}

// 独立的 LVGL 刷新任务
static void lvgl_main_task(void *pvParameters)
{
    lv_init();          
    st7789_init();      
    lv_disp_init();    
    ui_home_create();

    while (1) {
        lv_task_handler();  
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// --- Wi-Fi 事件处理函数 ---
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ip_str[16];
        sprintf(ip_str, IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "got ip: %s", ip_str);
        ui_set_ip_label(ip_str);
    }
}

// --- Wi-Fi 初始化函数 ---
static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    // 优化：开启 Modem Sleep 并限制 TX 功率，有助于降温
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); 
    esp_wifi_set_max_tx_power(40); // 10dBm 左右，对于室内投屏足够了限制发射频率

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

// --- UDP 服务器任务 ---
static void udp_server_task(void *pvParameters)
{
    // 等待网络栈和 Wi-Fi 完全启动
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 将 rx_buffer 改为静态变量（位于 DRAM），防止栈溢出
    static char rx_buffer[10240];     //10KB 的缓冲区如果放在函数栈里，会瞬间导致栈溢出（ESP32 任务栈通常只有 4-8KB）。使用 static 后，这块内存分配在 DRAM（静态内存）中，安全且稳定。
    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
    ip_protocol = IPPROTO_IP;

    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    
    // 增加 UDP 接收缓冲区大小，防止高频丢包
    int rcv_buf_size = 32 * 1024; // 32KB
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf_size, sizeof(rcv_buf_size));
    
    ESP_LOGI(TAG, "Socket created");

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", UDP_PORT);

    while (1) {
        struct sockaddr_in source_addr; 
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        } 
        
        if (len > sizeof(udp_packet_header_t)) {
            udp_packet_header_t* header = (udp_packet_header_t*)rx_buffer;
            lv_color_t* color_p = (lv_color_t*)(rx_buffer + sizeof(udp_packet_header_t));
            
            // 调试：每 50 个包打一条日志，证明数据正在进入
            static int packet_count = 0;
            if (++packet_count % 50 == 0) {
                ESP_LOGI(TAG, "Streaming: Receiving packets... (Last size: %d)", len);
            }

            st7789_fill_area(header->x1, header->y1, header->x2, header->y2, color_p);
            
            // 关键优化：不再使用 vTaskDelay(1)，因为 1 tick = 10ms 太慢了
            // 只有在收到一帧的最后一个包时才进行一次短暂延时，让出 CPU
            if (header->y2 >= 239) {
                vTaskDelay(1); 
            }
        }
    }

    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}