# - Find system default IrDA implementation
#
# On Linux it will use PkgConfig if present and supported,
# else and on all other architectures, it looks for it on its own.
# The following standard variables get defined:
#  Irda_FOUND:        true if Irda was found
#  Irda_INCLUDE_DIRS: the directory that contains the include file
#  Irda_LIBRARIES:    full path to the libraries

include ( CheckCSourceCompiles )

if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  set ( Irda_FOUND true )

elseif ( WIN32 )
  CHECK_C_SOURCE_COMPILES (
    "#include <winsock2.h>
     #include <af_irda.h>
     int main () {
       return 0;
     }"
    Irda_FOUND
  )

else ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  set ( ${Irda_FOUND} no )

endif ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )

if ( Irda_FOUND )
  set ( Irda_FOUND true )
else ( Irda_FOUND )
  set ( Irda_FOUND false )
endif ( Irda_FOUND )

if ( NOT Irda_FOUND )
  if ( NOT Irda_FIND_QUIETLY )
    message ( STATUS "Irda not found." )
  endif ( NOT Irda_FIND_QUIETLY )
  if ( Irda_FIND_REQUIRED )
    message ( FATAL_ERROR "Irda not found but required." )
  endif ( Irda_FIND_REQUIRED )
endif ( NOT Irda_FOUND )
