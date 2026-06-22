#!/bin/bash
# 下载 LVGL 库的 Shell 脚本

echo "正在下载 LVGL 库..."

# 如果目录已存在，先删除
if [ -d "lvgl" ]; then
    echo "删除现有 lvgl 目录..."
    rm -rf lvgl
fi

# 克隆 LVGL 仓库
git clone --depth 1 https://github.com/lvgl/lvgl.git lvgl

echo "LVGL 库下载完成！"
echo "请确保 lvgl 文件夹在 components/ 目录下"
