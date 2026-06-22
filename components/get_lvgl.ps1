# 下载 LVGL 库的 PowerShell 脚本
Write-Host "正在下载 LVGL 库..." -ForegroundColor Green

# 如果目录已存在，先删除
if (Test-Path "lvgl") {
    Write-Host "删除现有 lvgl 目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "lvgl"
}

# 克隆 LVGL 仓库
git clone --depth 1 https://github.com/lvgl/lvgl.git lvgl

Write-Host "LVGL 库下载完成！" -ForegroundColor Green
Write-Host "请确保 lvgl 文件夹在 components/ 目录下" -ForegroundColor Cyan
