#!/bin/bash
# 烧录小游戏固件到 Pico 2W。覆盖程序区；数据分区（存档）不在 UF2 里，不会丢。

cd "$(dirname "$0")"
UF2_FILE="build_firmware/game_firmware.uf2"

if [ ! -f "$UF2_FILE" ]; then
    echo "找不到 $UF2_FILE，请先运行 ./build_firmware.sh"
    exit 1
fi

echo "固件: $UF2_FILE"

SERIAL_PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -n 1)
if [ -n "$SERIAL_PORT" ] && ! ls /Volumes/ 2>/dev/null | grep -qiE "^(RP2350|RPI-RP2)$"; then
    echo "通过串口 $SERIAL_PORT 进入 BOOTSEL..."
    stty -f "$SERIAL_PORT" 1200 2>/dev/null
else
    echo "若长时间没反应：按住 BOOTSEL 再插 USB"
fi

echo "等待 Pico..."
while true; do
    PICO_MOUNT=$(ls /Volumes/ 2>/dev/null | grep -iE "^(RP2350|RPI-RP2)$" | head -n 1)
    if [ -n "$PICO_MOUNT" ]; then
        echo "检测到 /Volumes/$PICO_MOUNT"
        break
    fi
    sleep 0.5
done

echo "正在烧录..."
cp "$UF2_FILE" "/Volumes/$PICO_MOUNT/"
if [ $? -eq 0 ]; then
    echo "烧录成功，板子会自动重启"
else
    echo "烧录失败"
    exit 1
fi
