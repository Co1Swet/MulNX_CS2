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
[ ! -d "ffmpeg_src" ] || [ ! -f "ffmpeg_src/Makefile" ] && { echo "错误：ffmpeg_src 缺失"; exit 1; }
[ ! -d "openh264_src" ] && { echo "错误：openh264_src 缺失"; exit 1; }
echo "通过"

echo "==================================="
echo " 3. 编译 OpenH264 静态库（带完整的静态依赖声明）"
echo "==================================="
INSTALL_OPENH264="/opt/openh264"
rm -rf "$INSTALL_OPENH264"
mkdir -p "$INSTALL_OPENH264"

cd "$BASE/openh264_src"
make clean 2>/dev/null || true
make ENABLE64BIT=Yes OS=mingw_nt ARCH=x86_64

mkdir -p "$INSTALL_OPENH264/lib/pkgconfig" "$INSTALL_OPENH264/include/wels"
cp libopenh264.a "$INSTALL_OPENH264/lib/"
cp codec/api/wels/*.h "$INSTALL_OPENH264/include/wels/"

# 关键：pkg-config 文件声明 Libs.private 为完整的静态链接参数
cat > "$INSTALL_OPENH264/lib/pkgconfig/openh264.pc" << 'EOF'
prefix=/opt/openh264
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include/wels

Name: OpenH264
Description: Cisco OpenH264 Codec (static)
Version: 2.4.1
Libs: -L${libdir} -lopenh264
Libs.private: -lstdc++ -lgcc -lgcc_eh -lwinpthread
Cflags: -I${includedir}
EOF

# 安装到 mingw64 系统路径
mkdir -p /mingw64/include/wels /mingw64/lib/pkgconfig
cp -r "$INSTALL_OPENH264/include/wels/"* /mingw64/include/wels/
cp "$INSTALL_OPENH264/lib/libopenh264.a" /mingw64/lib/
cp "$INSTALL_OPENH264/lib/pkgconfig/openh264.pc" /mingw64/lib/pkgconfig/

echo "OpenH264 静态库安装完成"
pkg-config --modversion openh264
# 调试：显示 pkg-config 返回的静态链接标志（供检查）
echo "静态链接标志：$(pkg-config --static --libs openh264)"

cd "$BASE"

echo "==================================="
echo " 4. 构建 FFmpeg（全局静态链接，一劳永逸）"
echo "==================================="
INSTALL_FFMPEG="$BASE/ffmpeg_install"
BUILD_FFMPEG="$BASE/ffmpeg_src/build"

rm -rf "$INSTALL_FFMPEG" "$BUILD_FFMPEG"
mkdir -p "$BUILD_FFMPEG" "$INSTALL_FFMPEG"
cd "$BUILD_FFMPEG"

# 使用 -static 标志强制所有非系统库静态链接
# 无需 --enable-w32threads / --disable-pthreads，因为 -static 会静态链接 pthread 本身
../configure \
    --prefix="$INSTALL_FFMPEG" \
    --enable-shared --disable-static \
    --disable-doc --disable-programs \
    --enable-avdevice --enable-indev=dshow \
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
    --extra-ldflags="-static" \
    --pkg-config-flags="--static" || {
        echo "===== configure 失败，显示日志尾部 ====="
        tail -80 ffbuild/config.log
        exit 1
    }

echo "=== 清除绝对源路径 ==="
find . -type f \( -name 'Makefile' -o -name '*.mak' -o -name '*.d' -o -name '*.pc' -o -name 'config.*' \) \
    -exec sed -i 's|/opt/ffmpeg-build/ffmpeg_src|..|g' {} +
[ -f ffbuild/config.mak ] && sed -i 's|^SRC_PATH=.*|SRC_PATH=..|' ffbuild/config.mak
sed -i '1s|include .*|include ../Makefile|' Makefile
mkdir -p ../tools
touch ../tools/Makefile

echo "开始编译 FFmpeg（单线程，输出日志到 build.log）..."
make -j1 V=1 2>&1 | tee build.log
make install

echo ""
echo "==================================="
echo " 5. 最终验证"
echo "==================================="
cd "$INSTALL_FFMPEG/bin"
FAIL=0
for dll in *.dll; do
    echo -n "检查 $dll ... "
    DEPS=$(ldd "$dll" 2>/dev/null)
    # 检查是否依赖任何 MinGW 运行时 DLL
    if echo "$DEPS" | grep -qE "libwinpthread|libpthread|libstdc\+\+|libgcc_s|libiconv|libz|libopenh264"; then
        echo "失败！依赖了非系统库："
        echo "$DEPS" | grep -E "lib(winpthread|pthread|stdc\+\+|gcc_s|iconv|z|openh264)"
        FAIL=1
    else
        echo "通过"
    fi
done

if [ $FAIL -ne 0 ]; then
    echo ""
    echo "=============================================="
    echo "  验证失败，请查看 build.log 分析链接命令。"
    echo "=============================================="
    exit 1
fi

cd "$BASE"
echo ""
echo "=============================================="
echo "  完美成功！所有 DLL 仅依赖 Windows 系统 DLL。"
echo "  产物目录：$INSTALL_FFMPEG"
echo "=============================================="