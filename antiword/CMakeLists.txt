# Для выбора целевой архитектуры переменной TARGET_TYPE надо присвоить нужное
# значение:
# 	- Linux (по-умолчанию)
#	- ARM
#
# Например: cmake -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=Linux -DPLATFORM=FC

PROJECT (antiword)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.0)

IF (NOT PLATFORM)
	SET (PLATFORM FC)
ENDIF (NOT PLATFORM)

IF (PLATFORM STREQUAL "NX")
	SET (TOOLCHAIN_PATH "../../PBSDK")
	SET (TOOLCHAIN_PREFIX "arm-linux")
	SET (TOOLCHAIN_INSTALL "")
	ADD_DEFINITIONS(-DPLATFORM_NX)
ELSE ()
	SET (TOOLCHAIN_PATH "../../FRSCSDK")
	SET (TOOLCHAIN_PREFIX "arm-none-linux-gnueabi")
	SET (TOOLCHAIN_INSTALL "sysroot/usr")
	ADD_DEFINITIONS(-DPLATFORM_FC)
ENDIF (PLATFORM STREQUAL "NX")
#SET (CMAKE_VERBOSE_MAKEFILE ON)

# по-умолчанию сборка для Linux
IF (NOT TARGET_TYPE)
	SET (TARGET_TYPE Linux)
ENDIF (NOT TARGET_TYPE)

IF (NOT CMAKE_BUILD_TYPE)
	SET (CMAKE_BUILD_TYPE Debug)
ENDIF (NOT CMAKE_BUILD_TYPE)

# Изменить префикс установки
IF (TARGET_TYPE STREQUAL "ARM")
	SET(CMAKE_INSTALL_PREFIX "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}/${TOOLCHAIN_INSTALL}" CACHE PATH "Install path prefix" FORCE)

	# добавим компилятор для АРМ-а
	SET (CMAKE_C_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-gcc)
	SET (CMAKE_CXX_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-g++)
	SET (CMAKE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-g++)
	SET (CMAKE_ARR ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-arr)
	SET (CMAKE_STRIP ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-strip)

	SET (TARGET_INCLUDE "")
	SET (TARGET_LIB z)
ELSE()
	SET(CMAKE_INSTALL_PREFIX "${TOOLCHAIN_PATH}" CACHE PATH "Install path prefix" FORCE)

	FIND_PACKAGE (ZLIB REQUIRED)
	SET (TARGET_INCLUDE ${CMAKE_INSTALL_PREFIX}/include ${ZLIB_INCLUDE_DIRS})
	SET (TARGET_LIB ${ZLIB_LIBRARIES})

	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/${CMAKE_INSTALL_PREFIX}/lib)
ENDIF(TARGET_TYPE STREQUAL "ARM")

# установить нужные флаги компиляции
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")

	IF (TARGET_TYPE STREQUAL "Linux")
		MESSAGE (STATUS "Build for Linux Debug")
		SET (CMAKE_C_FLAGS_DEBUG "-DDEBUG -W -Wall -Wextra -O0 -g3 -DIVSAPP")
		SET (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -W -Wall -Wextra -O0 -g3 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Linux")

	IF (TARGET_TYPE STREQUAL "Windows")
		MESSAGE (STATUS "Build for Windows Debug")
		SET (CMAKE_C_FLAGS_DEBUG "-DDEBUG -W -Wall -Wextra -O0 -g3 -DIVSAPP")
		SET (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -W -Wall -Wextra -O0 -g3 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Windows")

	IF (TARGET_TYPE STREQUAL "ARM")
		MESSAGE (STATUS "Build for ARM Debug")
		SET (CMAKE_C_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -Wno-format-y2k -fomit-frame-pointer -O0 -g3")
		SET (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -Wno-format-y2k -fomit-frame-pointer -O0 -g3")
	ENDIF (TARGET_TYPE STREQUAL "ARM")

ELSE ()

	IF (TARGET_TYPE STREQUAL "Linux")
		MESSAGE (STATUS "Build for Linux Release")
		SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -s -Wall -O2 -DIVSAPP")
		SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -s -Wall -O2 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Linux")

	IF (TARGET_TYPE STREQUAL "Windows")
		MESSAGE (STATUS "Build for Windows Release")
		SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -s -Wall -O2 -DIVSAPP")
		SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -s -Wall -O2 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Windows")

	IF (TARGET_TYPE STREQUAL "ARM")
		MESSAGE (STATUS "Build for ARM Release")
		SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -Wno-format-y2k -fomit-frame-pointer")
		SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -Wno-format-y2k -fomit-frame-pointer")
	ENDIF (TARGET_TYPE STREQUAL "ARM")

ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

SET (SRC_LIST
	${CMAKE_SOURCE_DIR}/src/main_u.c
	${CMAKE_SOURCE_DIR}/src/asc85enc.c
	${CMAKE_SOURCE_DIR}/src/blocklist.c
	${CMAKE_SOURCE_DIR}/src/chartrans.c
	${CMAKE_SOURCE_DIR}/src/datalist.c
	${CMAKE_SOURCE_DIR}/src/depot.c
	${CMAKE_SOURCE_DIR}/src/dib2eps.c
	${CMAKE_SOURCE_DIR}/src/doclist.c
	${CMAKE_SOURCE_DIR}/src/fail.c
	${CMAKE_SOURCE_DIR}/src/finddata.c
	${CMAKE_SOURCE_DIR}/src/findtext.c
	${CMAKE_SOURCE_DIR}/src/fmt_text.c
	${CMAKE_SOURCE_DIR}/src/fontlist.c
	${CMAKE_SOURCE_DIR}/src/fonts.c
	${CMAKE_SOURCE_DIR}/src/fonts_u.c
	${CMAKE_SOURCE_DIR}/src/hdrftrlist.c
	${CMAKE_SOURCE_DIR}/src/imgexam.c
	${CMAKE_SOURCE_DIR}/src/imgtrans.c
	${CMAKE_SOURCE_DIR}/src/jpeg2eps.c
	${CMAKE_SOURCE_DIR}/src/listlist.c
	${CMAKE_SOURCE_DIR}/src/misc.c
	${CMAKE_SOURCE_DIR}/src/notes.c
	${CMAKE_SOURCE_DIR}/src/options.c
	${CMAKE_SOURCE_DIR}/src/out2window.c
	${CMAKE_SOURCE_DIR}/src/output.c
	${CMAKE_SOURCE_DIR}/src/pdf.c
	${CMAKE_SOURCE_DIR}/src/pictlist.c
	${CMAKE_SOURCE_DIR}/src/png2eps.c
	${CMAKE_SOURCE_DIR}/src/postscript.c
	${CMAKE_SOURCE_DIR}/src/prop0.c
	${CMAKE_SOURCE_DIR}/src/prop2.c
	${CMAKE_SOURCE_DIR}/src/prop6.c
	${CMAKE_SOURCE_DIR}/src/prop8.c
	${CMAKE_SOURCE_DIR}/src/properties.c
	${CMAKE_SOURCE_DIR}/src/propmod.c
	${CMAKE_SOURCE_DIR}/src/rowlist.c
	${CMAKE_SOURCE_DIR}/src/sectlist.c
	${CMAKE_SOURCE_DIR}/src/stylelist.c
	${CMAKE_SOURCE_DIR}/src/stylesheet.c
	${CMAKE_SOURCE_DIR}/src/summary.c
	${CMAKE_SOURCE_DIR}/src/tabstop.c
	${CMAKE_SOURCE_DIR}/src/text.c
	${CMAKE_SOURCE_DIR}/src/unix.c
	${CMAKE_SOURCE_DIR}/src/utf8.c
	${CMAKE_SOURCE_DIR}/src/word2text.c
	${CMAKE_SOURCE_DIR}/src/worddos.c
	${CMAKE_SOURCE_DIR}/src/wordlib.c
	${CMAKE_SOURCE_DIR}/src/wordmac.c
	${CMAKE_SOURCE_DIR}/src/wordole.c
	${CMAKE_SOURCE_DIR}/src/wordwin.c
	${CMAKE_SOURCE_DIR}/src/xmalloc.c
	${CMAKE_SOURCE_DIR}/src/xml.c
	${CMAKE_SOURCE_DIR}/src/fontinfo.h
	${CMAKE_SOURCE_DIR}/src/version.h
)	
ADD_CUSTOM_COMMAND(
 	OUTPUT ${CMAKE_SOURCE_DIR}/src/fontinfo.h
	COMMAND perl ${CMAKE_SOURCE_DIR}/src/Unix-only/fontinfo.pl > ${CMAKE_SOURCE_DIR}/src/fontinfo.h)

ADD_EXECUTABLE (antiword 
		${SRC_LIST}) 

INCLUDE_DIRECTORIES(${TARGET_INCLUDE})
TARGET_LINK_LIBRARIES (antiword ${TARGET_LIB})

INSTALL (TARGETS antiword DESTINATION bin)

