#!/bin/bash
# 收集 ui/ 源码字符串里的非 ASCII 字符，生成 12px / 24px 像素字库 assets/generated/font_ui_*.c
# 字符没变就跳过。用法: ./gen_fonts.sh [--force]

set -e
cd "$(dirname "$0")"

CONV="tools/node_modules/.bin/lv_font_conv"
FONT="tools/fonts/fusion-pixel-12px-proportional-zh_hans.ttf"
OUT="assets/generated"
STAMP="$OUT/font_chars.txt"

if [ ! -x "$CONV" ]; then
    echo "安装字体转换工具 lv_font_conv ..."
    (cd tools && npm install --silent)
fi

# printf 是终端日志，console.c 是终端帮助，都不上屏；lv_snprintf 是上屏文字，要收
CHARS="$(python3 - <<'EOF'
import pathlib, re
STRING_RE = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
chars = set()
for f in sorted(pathlib.Path("ui").rglob("*.[ch]")):
    if f.name == "console.c":
        continue
    for line in f.read_text(encoding="utf-8").splitlines():
        if re.search(r'(?<![\w])printf\(', line):
            continue
        for s in STRING_RE.findall(line):
            chars.update(ch for ch in s if ord(ch) > 0x7E)
print("".join(sorted(chars)), end="")
EOF
)"

# 无尽版独立字库；不替换其他小游戏的共用字库。
SAUSAGE_FONT="tools/fonts/Cubic_11.ttf"
SAUSAGE_STAMP="$OUT/font_sausage_stamp.txt"
SAUSAGE_HASH="$(shasum -a 256 "$SAUSAGE_FONT" | cut -d ' ' -f 1)"
SAUSAGE_KEY="$SAUSAGE_HASH:$CHARS"
mkdir -p "$OUT"
if [ "$1" == "--force" ] || [ ! -f "$SAUSAGE_STAMP" ] || [ "$(cat "$SAUSAGE_STAMP")" != "$SAUSAGE_KEY" ] \
    || [ ! -f "$OUT/font_sausage_12.c" ] || [ ! -f "$OUT/font_sausage_24.c" ]; then
    for SIZE in 12 24; do
        "$CONV" --font "$SAUSAGE_FONT" --size "$SIZE" --bpp 1 --format lvgl --no-compress \
            -r 0x20-0x7E --lv-font-name "font_sausage_$SIZE" -o "$OUT/font_sausage_$SIZE.c" \
            ${CHARS:+--symbols "$CHARS"} > /dev/null
    done
    printf '%s' "$SAUSAGE_KEY" > "$SAUSAGE_STAMP"
    echo "无尽版俐方体字库已生成"
fi

if [ "$1" != "--force" ] && [ -f "$STAMP" ] && [ "$(cat "$STAMP")" == "$CHARS" ] \
    && [ -f "$OUT/font_ui_12.c" ] && [ -f "$OUT/font_ui_24.c" ]; then
    exit 0
fi

mkdir -p "$OUT"
for SIZE in 12 24; do
    "$CONV" --font "$FONT" --size "$SIZE" --bpp 1 --format lvgl --no-compress \
        -r 0x20-0x7E --lv-font-name "font_ui_$SIZE" -o "$OUT/font_ui_$SIZE.c" \
        ${CHARS:+--symbols "$CHARS"} > /dev/null
done
printf '%s' "$CHARS" > "$STAMP"
echo "字库已生成: ASCII + ${#CHARS} 个字符"
