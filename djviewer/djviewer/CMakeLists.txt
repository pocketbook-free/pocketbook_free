# Для выбора целевой архитектуры переменной TARGET_TYPE надо присвоить нужное
# значение:
# 	- Linux (по-умолчанию)
#	- ARM
#
# Например: cmake -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=Linux -DPLATFORM=FC -DSYNOPSIS=no -DPBPRJ=no

PROJECT (djviewer)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.0 FATAL_ERROR)

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

IF (NOT PBPRJ)
	SET (PBPRJ no)
ENDIF (NOT PBPRJ) 

IF (NOT SYNOPSIS)
	SET (SYNOPSIS no)
ENDIF (NOT SYNOPSIS) 

IF (NOT SYNOPSIS_LIB)
	SET (SYNOPSIS_LIB "")
ENDIF (NOT SYNOPSIS_LIB)

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

	SET (TARGET_LIB pthread inkview freetype z jpeg tiff iconv)
	SET (TARGET_INCLUDE "")

	# добавим компилятор для АРМ-а
	SET (CMAKE_C_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-gcc)
	SET (CMAKE_CXX_COMPILER ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-g++)
	SET (CMAKE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-g++)
	SET (CMAKE_ARR ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-arr)
	SET (CMAKE_STRIP ${CMAKE_CURRENT_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-strip)

ELSE()
	SET(CMAKE_INSTALL_PREFIX "${TOOLCHAIN_PATH}" CACHE PATH "Install path prefix" FORCE)

	# проверить наличие необходимых пакетов
	FIND_PACKAGE (Freetype REQUIRED)
	FIND_PACKAGE (JPEG REQUIRED)
	FIND_PACKAGE (TIFF REQUIRED)
	FIND_PACKAGE (ZLIB REQUIRED)
	FIND_PACKAGE (GTK2 REQUIRED)

	SET (TARGET_INCLUDE ${CMAKE_INSTALL_PREFIX}/include ${FREETYPE_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS} ${TIFF_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
	SET (TARGET_LIB pthread inkview curl iconv ${FREETYPE_LIBRARIES} ${JPEG_LIBRARIES} ${TIFF_LIBRARIES} ${ZLIB_LIBRARIES} ${GTK2_LIBRARIES})

	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/${CMAKE_INSTALL_PREFIX}/lib)

	ADD_DEFINITIONS(-DEMULATOR=1)
ENDIF(TARGET_TYPE STREQUAL "ARM")

ADD_DEFINITIONS(-DHAVE_CONFIG_H=1)

# установить нужные флаги компиляции
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")

	IF (TARGET_TYPE STREQUAL "Linux")
		MESSAGE (STATUS "Build for Linux Debug")
                SET (CMAKE_C_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -O0 -g3 -DIVSAPP")
                SET (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -O0 -g3 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Linux")

	IF (TARGET_TYPE STREQUAL "Windows")
		MESSAGE (STATUS "Build for Windows Debug")
                SET (CMAKE_C_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -O0 -g3 -DIVSAPP")
                SET (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -O0 -g3 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Windows")

	IF (TARGET_TYPE STREQUAL "ARM")
		MESSAGE (STATUS "Build for ARM Debug")
                SET (CMAKE_C_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -Wno-format-y2k -fomit-frame-pointer -O0 -g3")
                SET (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -fsigned-char -W -Wall -Wextra -Wno-format-y2k -fomit-frame-pointer -O0 -g3")
	ENDIF (TARGET_TYPE STREQUAL "ARM")

ELSE ()

	IF (TARGET_TYPE STREQUAL "Linux")
		MESSAGE (STATUS "Build for Linux Release")
                SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -DIVSAPP")
                SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Linux")

	IF (TARGET_TYPE STREQUAL "Windows")
		MESSAGE (STATUS "Build for Windows Release")
                SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -DIVSAPP")
                SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -DIVSAPP")
	ENDIF (TARGET_TYPE STREQUAL "Windows")

	IF (TARGET_TYPE STREQUAL "ARM")
		MESSAGE (STATUS "Build for ARM Release")
                SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -Wno-format-y2k -fomit-frame-pointer -fvisibility=hidden")
                SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -s -fsigned-char -Wall -O2 -Wno-format-y2k -fomit-frame-pointer -fvisibility=hidden -fvisibility-inlines-hidden")
	ENDIF (TARGET_TYPE STREQUAL "ARM")

ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

# генерировать файл image.c
ADD_CUSTOM_COMMAND(
 	OUTPUT ${CMAKE_SOURCE_DIR}/cimages/images.c 
	COMMAND ${CMAKE_SOURCE_DIR}/${TOOLCHAIN_PATH}/bin/pbres -c ${CMAKE_SOURCE_DIR}/cimages/images.c -4 ${CMAKE_SOURCE_DIR}/images/*.bmp ${CMAKE_SOURCE_DIR}/../pbframework11/images/*.bmp)

FILE (GLOB SRC_LIST
	${CMAKE_SOURCE_DIR}/libdjvu/*.cpp
	${CMAKE_SOURCE_DIR}/src/*.cpp
	${CMAKE_SOURCE_DIR}/../pbframework11/framework/*.cpp
	${CMAKE_SOURCE_DIR}/../pbframework11/controls/*.cpp)

IF (SYNOPSIS STREQUAL "yes")
	MESSAGE (STATUS "Synopsis support enabled")
	ADD_DEFINITIONS(-DUSESYNOPSIS=1)
	SET (TARGET_LIB ${TARGET_LIB} synopsis)
ELSE ()
	MESSAGE (STATUS "Synopsis support disabled")
ENDIF (SYNOPSIS STREQUAL "yes")

ADD_EXECUTABLE (djviewer 
		${SRC_LIST}
		cimages/images.c )

INCLUDE_DIRECTORIES (${TARGET_INCLUDE} include)
TARGET_LINK_LIBRARIES (djviewer ${TARGET_LIB})

