cmake_minimum_required ( VERSION 2.6 FATAL_ERROR )

project ( openobex C )

#
# The project version
#
set ( VERSION_MAJOR 1 )
set ( VERSION_MINOR 4 )
set ( VERSION_PATCH 0 )

set ( SHORT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}" )
set ( VERSION       "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
if ( VERSION_PATCH GREATER 0 )
  set ( SHORT_VERSION "${VERSION}" )
endif ( VERSION_PATCH GREATER 0 )

#
# The path for our own CMake modules
#
set ( CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMakeModules
)

#
# Define the default build type
#
set ( CMAKE_CONFIGURATION_TYPES "Release;Debug"
      CACHE STRING "" FORCE )
if ( NOT CMAKE_BUILD_TYPE )
  set ( CMAKE_BUILD_TYPE Release
        CACHE STRING "Build type" FORCE )
endif ( NOT CMAKE_BUILD_TYPE )

#
# define how to build libraries
#
option ( BUILD_SHARED_LIBS "Build shared libraries" ON )

#
# check common compiler flags
#
include ( CheckCCompilerFlag )
if ( CMAKE_COMPILER_IS_GNUCC )
  if ( UNIX AND NOT WIN32 )
    set ( COMPILER_FLAG_VISIBILITY -fvisibility=hidden )
    check_c_compiler_flag ( ${COMPILER_FLAG_VISIBILITY} COMPILER_SUPPORT_VISIBILITY )
  endif ( UNIX AND NOT WIN32 )

  set ( LINKER_FLAG_NOUNDEFINED   -Wl,--no-undefined )
  check_c_compiler_flag ( "${LINKER_FLAG_NOUNDEFINED}" COMPILER_SUPPORT_NOUNDEFINED )
endif ( CMAKE_COMPILER_IS_GNUCC )

if ( MSVC )
  # Some compiler options for MSVC to not print non-sense warnings.
  add_definitions ( -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE )
endif ( MSVC )

#
# which transports shall be included
#
find_package ( Bluetooth )
set ( OPENOBEX_BLUETOOTH_AVAILABLE ${Bluetooth_FOUND} )

find_package ( Irda )
set ( OPENOBEX_IRDA_AVAILABLE ${Irda_FOUND} )

find_package ( LibUSB )
set ( OPENOBEX_USB_AVAILABLE ${LibUSB_FOUND} )

foreach ( transport BLUETOOTH IRDA USB )
  if ( OPENOBEX_${transport}_AVAILABLE )
    set ( OPENOBEX_${transport} ON
          CACHE BOOL "Build with ${transport} support")
  else ( OPENOBEX_${transport}_AVAILABLE )
    set ( OPENOBEX_${transport} OFF
          CACHE BOOL "Build with ${transport} support")
  endif ( OPENOBEX_${transport}_AVAILABLE )
  if (OPENOBEX_${transport})
    add_definitions ( -DHAVE_${transport} )
  endif (OPENOBEX_${transport})
endforeach ( transport )

if ( OPENOBEX_IRDA )
  if ( WIN32 )
    add_definitions ( -DHAVE_IRDA_WINDOWS )
  else ( WIN32 )
    string ( TOUPPER "HAVE_IRDA_${CMAKE_SYSTEM_NAME}" IRDA_SYSTEM_DEFINITION )
    add_definitions ( -D${IRDA_SYSTEM_DEFINITION} )
  endif ( WIN32 )
endif ( OPENOBEX_IRDA )

if ( OPENOBEX_BLUETOOTH )
  if ( WIN32 )
    add_definitions ( -DHAVE_BLUETOOTH_WINDOWS )
  else ( WIN32 )
    string ( TOUPPER "HAVE_BLUETOOTH_${CMAKE_SYSTEM_NAME}" BLUETOOTH_SYSTEM_DEFINITION )
    add_definitions ( -D${BLUETOOTH_SYSTEM_DEFINITION} )
  endif ( WIN32 )
endif ( OPENOBEX_BLUETOOTH )

#
# create pkg-config files
# these get copied and installed in the library dirs
# TODO: those files should be moved to subdirs for each library
#
set ( prefix      "${CMAKE_INSTALL_PREFIX}" )
set ( exec_prefix "\${prefix}" )
set ( libdir      "\${prefix}/lib" )
set ( includedir  "\${prefix}/include" )
if ( OPENOBEX_USB AND UNIX AND NOT WIN32 )
   set ( REQUIRES "libusb" )
endif ( OPENOBEX_USB AND UNIX AND NOT WIN32 )
foreach ( file openobex openobex-glib )
  configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${file}.pc
    @ONLY
  )
endforeach ( file )

if ( NOT PKGCONFIG_INSTALL_DIR )
  set ( PKGCONFIG_INSTALL_DIR lib/pkgconfig
        CACHE PATH "Where to install .pc files to" FORCE )
endif ( NOT PKGCONFIG_INSTALL_DIR )
mark_as_advanced ( PKGCONFIG_INSTALL_DIR )


#
# process include directory
#
set ( openobex_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include" )
include_directories ( BEFORE SYSTEM "${openobex_INCLUDE_DIRS}" )
if ( MSVC )
  include_directories ( AFTER SYSTEM "${openobex_INCLUDE_DIRS}/msvc" )
endif ( MSVC )


#
# build the main library
#
add_subdirectory ( lib )
link_directories ( "${CMAKE_CURRENT_BINARY_DIR}/lib" )
if ( BUILD_SHARED_LIBS )
  add_definitions ( -DOPENOBEX_DLL )
endif ( BUILD_SHARED_LIBS )


#
# build the applications
#
add_custom_target ( openobex-apps )
add_subdirectory ( apps )
add_subdirectory ( ircp )


#
# build the documentation
#
option ( BUILD_DOCUMENTATION "Build library and application documentation" ON)
if ( BUILD_DOCUMENTATION )
  if ( NOT DOCUMENTATION_INSTALL_DIR )
    set ( DOCUMENTATION_INSTALL_DIR share/doc/openobex
      CACHE PATH "Where to install generic documentation files to" FORCE )
  endif ( NOT DOCUMENTATION_INSTALL_DIR )
  mark_as_advanced ( DOCUMENTATION_INSTALL_DIR )

  if ( NOT MANPAGE_INSTALL_DIR )
    set ( MANPAGE_INSTALL_DIR share/man
      CACHE PATH "Where to install manpage files to" FORCE )
  endif ( NOT MANPAGE_INSTALL_DIR )
  mark_as_advanced ( MANPAGE_INSTALL_DIR )

  add_subdirectory ( doc )
endif ( BUILD_DOCUMENTATION )


#
# build the glib binding library
# not enabled by default because it requires an additional dependency
#
#option ( BUILD_GLIB_BINDING "Build the glib binding library")
#if ( BUILD_GLIB_BINDING )
#  add_subdirectory ( glib )
#endif ( BUILD_GLIB_BINDING )


#
# The following adds CPack support
#
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenObex" )
set ( CPACK_PACKAGE_VENDOR "The OpenObex Development Team" )

set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LIB" )
set ( CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/README" )

set ( CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}" )
set ( CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}" )
set ( CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}" )
set ( CPACK_PACKAGE_VERSION       "${VERSION}" )

if ( UNIX )
  set ( CPACK_GENERATOR "TGZ" )
  set ( CPACK_SOURCE_GENERATOR "TGZ" )

elseif ( WIN32 )
  #
  # For NSIS, install from http://nsis.sf.net.
  # For ZIP, install e.g. info-zip from http://www.info-zip.org.
  #
  set ( CPACK_GENERATOR "ZIP;NSIS" )
  set ( CPACK_SOURCE_GENERATOR "ZIP" )
endif ( UNIX )
set ( CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/\\\\.git/"
  "/\\\\.gitignore$"
  "~$"
)

# this must _follow_ the settings!
include ( CPack )
