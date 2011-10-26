# - Find system default bluetooth implementation
#
# On Linux it will use PkgConfig if present and supported,
# else and on all other architectures, it looks for it on its own.
# The following standard variables get defined:
#  Bluetooth_FOUND:        true if Bluetooth was found
#  Bluetooth_INCLUDE_DIRS: the directory that contains the include file
#  Bluetooth_LIBRARIES:    full path to the libraries

include ( CheckCSourceCompiles )
include ( CheckLibraryExists )
include ( CheckIncludeFile )

if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  find_package ( PkgConfig )
  if ( PKG_CONFIG_FOUND )
    pkg_check_modules ( PKGCONFIG_BLUEZ bluez )
  endif ( PKG_CONFIG_FOUND )
  if ( PKGCONFIG_BLUEZ_FOUND )
    foreach ( i ${PKGCONFIG_BLUEZ_LIBRARIES} )
      find_library ( ${i}_LIBRARY
        NAMES
	  ${i}
        PATHS
	  ${PKGCONFIG_BLUEZ_LIBRARY_DIRS}
      )
      mark_as_advanced ( ${i}_LIBRARY )
      if ( ${i}_LIBRARY )
	list ( APPEND Bluetooth_LIBRARIES ${${i}_LIBRARY} )
      endif ( ${i}_LIBRARY )
    endforeach ( i )
    add_definitions ( -DHAVE_SDPLIB )

  else ( PKGCONFIG_BLUEZ_FOUND )
    find_path ( Bluetooth_INCLUDE_DIRS
      NAMES
        bluetooth/bluetooth.h
      PATH_SUFFIXES
        include
    )
    mark_as_advanced ( Bluetooth_INCLUDE_DIRS )

    find_library ( bluetooth_LIBRARY
      NAMES
        bluetooth
      PATH_SUFFIXES
        lib
    )
    mark_as_advanced ( bluetooth_LIBRARY )
    if ( bluetooth_LIBRARY )
      set ( Bluetooth_LIBRARIES ${bluetooth_LIBRARY} )
    endif ( bluetooth_LIBRARY )
  endif ( PKGCONFIG_BLUEZ_FOUND )
  if ( Bluetooth_INCLUDE_DIRS AND Bluetooth_LIBRARIES )
    set ( Bluetooth_FOUND true )
  endif ( Bluetooth_INCLUDE_DIRS AND Bluetooth_LIBRARIES )

elseif ( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
  find_path ( Bluetooth_INCLUDE_DIRS
    NAMES
      bluetooth.h
    PATH_SUFFIXES
      include
  )
  mark_as_advanced ( Bluetooth_INCLUDE_DIRS )

  if ( Bluetooth_INCLUDE_DIRS )
    set ( CMAKE_REQUIRED_INCLUDES ${Bluetooth_INLUDE_DIRS} )
    CHECK_C_SOURCE_COMPILES (
      "#include <bluetooth.h>
       int main () {
         struct sockaddr_rfcomm f;
         return 0;
       }"
      Bluetooth_FOUND
    )
  endif ( Bluetooth_INCLUDE_DIRS )

elseif ( CMAKE_SYSTEM_NAME STREQUAL "NetBSD" )
  find_path ( Bluetooth_INCLUDE_DIRS
    NAMES
      bluetooth.h
    PATH_SUFFIXES
      include
  )
  mark_as_advanced ( Bluetooth_INCLUDE_DIRS )

  if ( Bluetooth_INCLUDE_DIRS )
    set ( CMAKE_REQUIRED_INCLUDES ${Bluetooth_INLUDE_DIRS} )
    CHECK_C_SOURCE_COMPILES (
      "#include <bluetooth.h>
       int main () {
         struct sockaddr_bt f;
         return 0;
       }"
      Bluetooth_FOUND
    )    
  endif ( Bluetooth_INCLUDE_DIRS )

elseif ( WIN32 )
  CHECK_C_SOURCE_COMPILES (
    "#include <winsock2.h>
     #include <ws2bth.h>
     int main () {
       SOCKADDR_BTH f;
       return 0;
     }"
    Bluetooth_FOUND
  )    
endif ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )

if ( Bluetooth_FOUND )
  set ( Bluetooth_FOUND true )
else ( Bluetooth_FOUND )
  set ( Bluetooth_FOUND false )
endif ( Bluetooth_FOUND )

if ( NOT Bluetooth_FOUND )
  if ( NOT Bluetooth_FIND_QUIETLY )
    message ( STATUS "Bluetooth not found." )
  endif ( NOT Bluetooth_FIND_QUIETLY )
  if ( Bluetooth_FIND_REQUIRED )
    message ( FATAL_ERROR "Bluetooth not found but required." )
  endif ( Bluetooth_FIND_REQUIRED )
endif ( NOT Bluetooth_FOUND )
