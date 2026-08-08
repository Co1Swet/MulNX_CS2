#!/bin/bash
set -e

BASE="/opt/ffmpeg-build"
cd "$BASE"

echo "==================================="
echo " 1. 环境检查与自动修复"
echo "==================================="

if ! command -v make &>/dev/null; then
    if [ -f /mingw64/bin/mingw32-make.exe ]; then
        cp /mingw64/bin/mingw32-make.exe /mingw64/bin/make.exe
    else
        pacman -S --noconfirm mingw-w64-x86_64-make
        [ ! -f /mingw64/bin/make.exe ] && cp /mingw64/bin/mingw32-make.exe /mingw64/bin/make.exe
    fi
fi

for pkg in mingw-w64-x86_64-gcc mingw-w64-x86_64-pkg-config mingw-w64-x86_64-nasm git diffutils; do
    if ! pacman -Q $pkg &>/dev/null; then
        echo "正在安装 $pkg ..."
        pacman -S --noconfirm $pkg
    fi
done

echo "==================================="
echo " 2. 源码完整性检查"
echo "==================================="

if [ ! -d "ffmpeg_src" ] || [ ! -f "ffmpeg_src/Makefile" ]; then
    echo "错误：ffmpeg_src 缺失或不完整。请手动克隆。"
    exit 1
fi
echo "ffmpeg_src 检查通过"

if [ ! -d "openh264_src" ]; then
    echo "错误：openh264_src 缺失。请手动克隆。"
    exit 1
fi
echo "openh264_src 检查通过"

echo "==================================="
echo " 3. 编译 OpenH264 并部署到默认路径"
echo "==================================="
INSTALL_OPENH264="/opt/openh264"
rm -rf "$INSTALL_OPENH264"
mkdir -p "$INSTALL_OPENH264"

cd "$BASE/openh264_src"
make clean 2>/dev/null || true
make ENABLE64BIT=Yes OS=mingw_nt ARCH=x86_64

if make install PREFIX="$INSTALL_OPENH264" OS=mingw_nt ARCH=x86_64 2>/dev/null; then
    echo "OpenH264 make install 成功"
else
    mkdir -p "$INSTALL_OPENH264/bin" "$INSTALL_OPENH264/lib/pkgconfig" "$INSTALL_OPENH264/include/wels"
    cp libopenh264.dll "$INSTALL_OPENH264/bin/"
    cp libopenh264.dll.a "$INSTALL_OPENH264/lib/"
    cp codec/api/wels/*.h "$INSTALL_OPENH264/include/wels/"
fi

mkdir -p /mingw64/include/wels /mingw64/lib/pkgconfig /mingw64/bin
cp -r "$INSTALL_OPENH264/include/wels/"* /mingw64/include/wels/
cp "$INSTALL_OPENH264/lib/libopenh264.dll.a" /mingw64/lib/
cp "$INSTALL_OPENH264/bin/libopenh264.dll" /mingw64/bin/

cat > /mingw64/lib/pkgconfig/openh264.pc << 'EOF'
prefix=/mingw64
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include/wels

Name: OpenH264
Description: Cisco OpenH264 Codec
Version: 2.4.1
Libs: -L${libdir} -lopenh264 -lstdc++ -lpthread
Cflags: -I${includedir}
EOF

echo "OpenH264 安装完成，pkg-config 测试："
pkg-config --modversion openh264

cd "$BASE"

echo "==================================="
echo " 4. 源内构建 FFmpeg（绝对路径彻底清除）"
echo "==================================="
INSTALL_FFMPEG="$BASE/ffmpeg_install"
BUILD_FFMPEG="$BASE/ffmpeg_src/build"

rm -rf "$INSTALL_FFMPEG" "$BUILD_FFMPEG"
mkdir -p "$BUILD_FFMPEG" "$INSTALL_FFMPEG"
cd "$BUILD_FFMPEG"

LDFLAGS_EXTRA="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic"

../configure \
    --prefix="$INSTALL_FFMPEG" \
    --enable-shared --disable-static \
    --disable-doc --disable-programs \
    --enable-avdevice \
    --enable-indev=dshow \
    --enable-avcodec --enable-avformat --enable-avutil \
    --enable-swscale --enable-swresample --enable-avfilter \
    --disable-everything \
    --enable-decoder=h264,aac,hevc,mp3 \
    --enable-encoder=aac,libopenh264 \
    --enable-demuxer=mov,mp4,matroska \
    --enable-muxer=mp4,mov \
    --enable-parser=h264,aac,hevc \
    --enable-protocol=file,http \
    --enable-filter=scale,format,trim,overlay \
    --enable-libopenh264 \
    --extra-ldflags="$LDFLAGS_EXTRA" || {
        tail -80 ffbuild/config.log
        exit 1
    }

echo "=== 清除所有绝对源路径，替换为相对路径 .. ==="
# 递归替换所有生成文件中的 /opt/ffmpeg-build/ffmpeg_src 为 ..
find . -type f \( -name 'Makefile' -o -name '*.mak' -o -name '*.d' -o -name '*.pc' -o -name 'config.*' \) \
    -exec sed -i 's|/opt/ffmpeg-build/ffmpeg_src|..|g' {} +

# 确认 SRC_PATH 正确
if [ -f ffbuild/config.mak ]; then
    sed -i 's|^SRC_PATH=.*|SRC_PATH=..|' ffbuild/config.mak
fi

# 修复顶层 Makefile 的 include
sed -i '1s|include .*|include ../Makefile|' Makefile

# 创建缺失的 tools/Makefile
mkdir -p ../tools
touch ../tools/Makefile

echo "开始编译 FFmpeg（单线程，稳定模式）..."
make -j1 V=1
make install

cd "$BASE"
echo ""
echo "=============================================="
echo "  编译成功！产物：$INSTALL_FFMPEG"
echo "  请将 bin/ include/ lib/ 复制到项目的"
echo "  ThirdPartyBuild/ffmpeg-master-lgpl-shared/"
echo "=============================================="