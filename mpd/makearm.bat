if not %POCKETBOOKSDK%.==. goto C1
echo Environment variable POCKETBOOKSDK is not set
pause
:C1
set PATH=%POCKETBOOKSDK%\arm-linux\bin;%POCKETBOOKSDK%\bin;%PATH%

set INCLUDE=-I/arm-linux/include
set LIBS=-L./lib -lmad
set OUTPUT=mpd.app

rm -f %OUTPUT%

gcc -Wall -O2 -fomit-frame-pointer %INCLUDE% -o %OUTPUT% src/*.c %LIBS%
if errorlevel 1 goto L_ER
strip %OUTPUT%

exit 0

:L_ER
exit 1

