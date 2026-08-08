pacman -Syu
pacman -Syu
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-pkg-config git
git clone --depth 1 https://git.ffmpeg.org/ffmpeg.git ffmpeg_src
rm -rf ffmpeg_src
git clone --depth 1 https://github.com/FFmpeg/FFmpeg.git ffmpeg_src
pacman -S ca-certificates
git config --global http.sslCAInfo /usr/ssl/certs/ca-bundle.crt
rm -rf ffmpeg_src
git clone --depth 1 https://github.com/FFmpeg/FFmpeg.git ffmpeg_src
rm -rf ffmpeg_src
git clone --depth 1 https://github.com/FFmpeg/FFmpeg.git ffmpeg_src
git config --global http.sslVerify false
git clone --depth 1 https://github.com/FFmpeg/FFmpeg.git ffmpeg_src
git clone --depth 1 --branch v2.4.1 https://github.com/cisco/openh264.git openh264_src
mkdir -p /opt/ffmpeg-build
mv ~/ffmpeg_src /opt/ffmpeg-build/
mv ~/openh264_src /opt/ffmpeg-build/
cd /opt/ffmpeg-build
chmod +x build_all.sh
./build_all.sh
pacman -S mingw-w64-x86_64-make
make --version
cd /opt/ffmpeg-build
./build_all.sh
ls /mingw64/bin/make.exe
pacman -Ql mingw-w64-x86_64-make
cp /mingw64/bin/mingw32-make.exe /mingw64/bin/make.exe
make --version
./build_all.sh
cd /opt/ffmpeg-build
./build_all.sh
head -n 50 build_all.sh
cd /opt/ffmpeg-build/openh264_src
ls build/platform-mingw_nt.mk
cd /opt/ffmpeg-build
./build_all.sh
pacman -S mingw-w64-x86_64-nasm
cd /opt/ffmpeg-build
./build_all.sh
cd /opt/ffmpeg-build
chmod +x build_all.sh
./build_all.sh
cd /opt/ffmpeg-build
./build_all.sh
cd /opt/ffmpeg-build 
./build_all.sh
cd /opt/ffmpeg-build
./build_all.sh
cat /opt/ffmpeg-build/openh264_install/lib/pkgconfig/openh264.pc
cd /opt/ffmpeg-build
./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build
rm -rf ffmpeg_src
cd /opt/ffmpeg-build
rm -rf ffmpeg_src
git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg_src
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build
ls -l ffmpeg_src/Makefile
file ffmpeg_src/Makefile
head -1 ffmpeg_build/Makefile
head -1 ffmpeg_build/Makefile | od -c
which make
make --version | head -1
cd ffmpeg_build
make -n -p 2>&1 | grep -i "include" | head -10
echo "include /opt/ffmpeg-build/ffmpeg_src/Makefile" > /tmp/test_makefile.mk
make -f /tmp/test_makefile.mk -n 2>&1 | head -5
test -r /opt/ffmpeg-build/ffmpeg_src/Makefile && echo "READABLE" || echo "NOT READABLE"
make -C /opt/ffmpeg-build/ffmpeg_build 2>&1 | head -10
cd /opt/ffmpeg-build && ./build_all.sh
ps aux | grep -E "gcc|g++|ld|dlltool"
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build
rm -rf ffmpeg_build ffmpeg_install
cd /opt/ffmpeg-build
rm -rf ffmpeg_build ffmpeg_install
./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
ps aux | grep -E "make|gcc|g\+\+|nasm|ld|dlltool"
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
make -n | grep avutil-61.dll
cd /opt/ffmpeg-build && ./build_all.sh
make -j1 V=1 2>&1 | tee build.log
file /mingw64/lib/libwinpthread.a
ar t /mingw64/lib/libwinpthread.a | head -5
file /mingw64/lib/libwinpthread.a
ar t /mingw64/lib/libwinpthread.a | head -5
cd /opt/ffmpeg-build && ./build_all.sh
cd /opt/ffmpeg-build && ./build_all.sh
