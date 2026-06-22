# LVGL VLOG - ESP32 无线投屏项目

基于 ESP32 的实时视频/图像无线投屏系统，使用 LVGL 图形库和 ST7789 LCD 屏幕。

## 功能特性

- 🌐 **Wi-Fi 连接**：支持连接 Wi-Fi 网络
- 📺 **实时投屏**：通过 UDP 协议接收视频流数据
- 🎨 **图形界面**：使用 LVGL 创建美观的用户界面
- 🔌 **硬件驱动**：支持 ST7789 240x240 LCD 屏幕
- ⚡ **双核优化**：利用 ESP32 双核特性提高性能

## 硬件配置

### 屏幕引脚

| 功能 | ESP32 引脚 |
|------|-----------|
| MOSI | GPIO 23 |
| SCLK | GPIO 18 |
| CS | GPIO 15 |
| DC | GPIO 2 |
| RST | GPIO 4 |
| BKL | GPIO 25 |

### 屏幕规格

- 型号：ST7789
- 分辨率：240x240
- 接口：SPI
- 色彩模式：RGB565

## 软件架构

### 任务分配

| 任务 | 核心 | 优先级 | 功能 |
|------|------|--------|------|
| lvgl_main_task | Core 1 | 4 | LVGL 界面渲染 |
| udp_server_task | Core 1 | 5 | UDP 数据接收与屏幕刷新 |

### 数据流程

1. 电脑/设备通过 Wi-Fi 发送视频数据包到 ESP32
2. ESP32 在 8080 端口接收 UDP 数据包
3. 解析数据包（包含区域坐标和像素数据）
4. 直接更新 ST7789 屏幕指定区域

## 快速开始

### 1. 配置 Wi-Fi

修改 `main/lvgl_display.c` 中的 Wi-Fi 配置：

```c
#define WIFI_SSID      "你的WiFi名称"
#define WIFI_PASS      "你的WiFi密码"
```

### 2. 编译与烧录

使用 ESP-IDF 工具链编译和烧录：

```bash
idf.py build
idf.py flash
```

### 3. 查看串口日志

```bash
idf.py monitor
```

### 4. 发送视频流

在电脑上运行 UDP 发送程序，将屏幕数据发送到 ESP32 的 8080 端口。

## UDP 数据包格式

```
+------------------+------------------+
| 数据包头 (8 字节)| 像素数据 (n 字节)|
+------------------+------------------+
```

### 数据包头结构

```c
typedef struct {
    uint16_t x1;  // 区域起始 X 坐标
    uint16_t y1;  // 区域起始 Y 坐标
    uint16_t x2;  // 区域结束 X 坐标
    uint16_t y2;  // 区域结束 Y 坐标
} udp_packet_header_t;
```

### 像素数据

每个像素使用 RGB565 格式（2 字节）。

## 项目结构

```
lvgl_vlog/
├── components/
│   ├── bsp/
│   │   ├── st7789.c          # ST7789 驱动
│   │   └── st7789.h
│   └── lvgl/                 # LVGL 图形库
├── main/
│   ├── lvgl_display.c        # 主程序
│   ├── lv_port.c             # LVGL 显示驱动
│   ├── ui_cxk.c              # 自定义 UI
│   ├── img/
│   │   └── cxk1.c            # 图片资源
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig
└── partitions.csv
```

## 性能优化

- ✅ 使用互斥锁保护 SPI 访问
- ✅ 增大 UDP 接收缓冲区（32KB）
- ✅ 静态分配接收缓冲区避免栈溢出
- ✅ 双核调度提高系统响应
- ✅ SPI 时钟 40MHz 降低发热
- ✅ Wi-Fi 发射功率优化

## 代码优化建议

1. **Wi-Fi 配置**：考虑使用 NVS 存储 Wi-Fi 凭证
2. **错误处理**：增加更完善的错误恢复机制
3. **配置分离**：将硬件配置与代码分离
4. **缓冲区管理**：优化像素数据缓冲区大小

## 技术栈

- **芯片**：ESP32
- **框架**：ESP-IDF
- **图形库**：LVGL
- **协议**：Wi-Fi + UDP
- **RTOS**：FreeRTOS

## 许可证

本项目仅供学习和研究使用。

---

**注意**：请根据自己的硬件配置修改引脚定义！
