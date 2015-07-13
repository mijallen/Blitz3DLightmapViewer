set PATH=%PATH%;C:\MinGW\bin

gcc -c Stack.c 2>compile.log

gcc -c Blitz3DFile.c 2>>compile.log

gcc -IC:/resources/SDL2-2.0.3/i686-w64-mingw32/include/SDL2 -IC:/resources/lpng1522 -c -ansi display.c 2>>compile.log

gcc -LC:/resources/SDL2-2.0.3/i686-w64-mingw32/lib -LC:/resources/lpng1522 -LC:/resources/zlib-1.2.8 -o LightmapViewer.exe display.o Stack.o Blitz3DFile.o -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglu32 -lpng -lz 2>>compile.log

type compile.log

pause
