if not %POCKETBOOKSDK%.==. goto C1
echo Environment variable POCKETBOOKSDK is not set
pause
:C1
set PATH=%POCKETBOOKSDK%\arm-linux\bin;%POCKETBOOKSDK%\bin;%PATH%

set INCLUDE=-I/arm-linux/include
set LIBS=-linkview -lfreetype -ljpeg -lz
set OUTPUT=Chess.app

rm -f %OUTPUT%

set IMAGES=
if not exist images\*.bmp goto NOIMG
set IMAGES=%TEMP%\images.temp.c
pbres -c %IMAGES% images/*.bmp
if errorlevel 1 goto L_ER
:NOIMG

gcc -Wall -O2 -fomit-frame-pointer %INCLUDE% -I./src -o %OUTPUT% src/*.c %IMAGES% %LIBS%
if errorlevel 1 goto L_ER
strip %OUTPUT%

exit 0

:L_ER
exit 1

