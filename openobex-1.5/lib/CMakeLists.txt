#
# Main library version and shared object version
#

# this defines supported properties that are set from
# variables of the form openobex_*
set ( openobex_PROPERTIES
  VERSION
  SOVERSION
  PUBLIC_HEADER
  COMPILE_DEFINITIONS
  COMPILE_FLAGS
  LINK_FLAGS
)

# the library version always matches the project version
foreach ( i VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH )
  set ( openobex_${i}   "${${i}}" )
endforeach ( i )
# the ABI version, must be increased on incompatible changes
set ( openobex_SOVERSION "1" )


set ( SOURCES
  obex.c
  obex_client.c
  obex_connect.c
  obex_header.c
  obex_main.c
  obex_object.c
  obex_server.c
  obex_transport.c
  databuffer.c
  inobex.c
)

set ( HEADERS
  obex_client.h
  obex_connect.h
  obex_header.h
  obex_main.h
  obex_object.h
  obex_server.h
  obex_transport.h
  databuffer.h
  inobex.h
)

set ( openobex_PUBLIC_HEADER
  ${openobex_INCLUDE_DIRS}/openobex/obex.h
  ${openobex_INCLUDE_DIRS}/openobex/obex_const.h
)

option ( ENABLE_DEBUG_MESSAGES "Enables debug messages printed during runtime" )
if ( ENABLE_DEBUG_MESSAGES )
  list ( APPEND openobex_COMPILE_DEFINITIONS OBEX_DEBUG )
  if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
     option ( DEBUG_USE_SYSLOG "Use SysLog facility instead of stderr for debug messages" )
  endif ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  if ( DEBUG_USE_SYSLOG )     
    list ( APPEND openobex_COMPILE_DEFINITIONS OBEX_SYSLOG )
  endif ( DEBUG_USE_SYSLOG )     
endif ( ENABLE_DEBUG_MESSAGES )

option ( ENABLE_DUMP_MESSAGES "Enables dumping of data" )
if ( ENABLE_DUMP_MESSAGES )
  list ( APPEND openobex_COMPILE_DEFINITIONS OBEX_DUMP )
endif ( ENABLE_DUMP_MESSAGES )

if ( OPENOBEX_IRDA )
  list ( APPEND SOURCES
    irobex.c
  )
  list ( APPEND HEADERS
    irobex.h
    irda.h
    irda_wrap.h
  )
endif ( OPENOBEX_IRDA )

if ( OPENOBEX_BLUETOOTH )
  list ( APPEND SOURCES
    btobex.c
  )
  list ( APPEND HEADERS
    btobex.h
    bluez_compat.h
  )
endif ( OPENOBEX_BLUETOOTH )

if ( OPENOBEX_USB )
  list ( APPEND openobex_LIBRARIES
    ${LibUSB_LIBRARIES}
  )
  include_directories ( BEFORE SYSTEM ${LibUSB_INCLUDE_DIRS} )
  list ( APPEND SOURCES
    usbobex.c
  )
  list ( APPEND HEADERS
    usbobex.h
  )
endif ( OPENOBEX_USB )

if ( COMPILER_SUPPORT_VISIBILITY )
  list ( APPEND openobex_COMPILE_DEFINITIONS HAVE_VISIBILITY )
  set ( openobex_COMPILE_FLAGS "${openobex_COMPILE_FLAGS} ${COMPILER_FLAG_VISIBILITY}" )
endif ( COMPILER_SUPPORT_VISIBILITY )
if ( COMPILER_SUPPORT_NOUNDEFINED )
  set ( openobex_LINK_FLAGS "${openobex_LINK_FLAGS} ${LINKER_FLAG_NOUNDEFINED}" )
endif ( COMPILER_SUPPORT_NOUNDEFINED )

if ( WIN32 )
  if ( CMAKE_COMPILER_IS_GNUCC )
    set ( openobex_LINK_FLAGS
      "${openobex_LINK_FLAGS} -Wl,--disable-stdcall-fixup -Wl,--add-stdcall-alias"
    )
  endif ( CMAKE_COMPILER_IS_GNUCC )

  list ( APPEND openobex_LIBRARIES
    ws2_32
  )

  if ( CMAKE_RC_COMPILER )
    set ( OPENOBEX_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/openobex.rc" )
    configure_file (
      "${CMAKE_CURRENT_SOURCE_DIR}/openobex.rc.in"
      "${OPENOBEX_RC_FILE}"
      @ONLY
    )
  endif ( CMAKE_RC_COMPILER )

  if ( MSVC )
    set ( OPENOBEX_DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/openobex.def" )
    file ( WRITE "${OPENOBEX_DEF_FILE}" "VERSION ${openobex_VERSION_MAJOR}.${openobex_VERSION_MINOR}\n" )
    file ( APPEND "${OPENOBEX_DEF_FILE}" "EXPORTS\n" )
    file ( READ "${CMAKE_CURRENT_SOURCE_DIR}/obex.sym" OPENOBEX_SYMBOLS )
    file ( APPEND "${OPENOBEX_DEF_FILE}" "${OPENOBEX_SYMBOLS}\n" )
  endif ( MSVC )
endif ( WIN32 )

if ( CYGWIN )
  #also define _WIN32 under CygWin
  list ( APPEND openobex_COMPILE_DEFINITIONS _WIN32)
endif ( CYGWIN )

# Add the openobex library target
add_library ( openobex
  ${SOURCES}
  ${HEADERS}
  ${openobex_PUBLIC_HEADER}
  ${OPENOBEX_RC_FILE}
  ${OPENOBEX_DEF_FILE}
)

target_link_libraries ( openobex
  ${openobex_LIBRARIES}
)
#disable link chaining
set_property(TARGET openobex PROPERTY LINK_INTERFACE_LIBRARIES "")

foreach ( i ${openobex_PROPERTIES} )
  if ( DEFINED openobex_${i} )
    set_property ( TARGET openobex PROPERTY ${i} ${openobex_${i}} )
  endif ( DEFINED openobex_${i} )
endforeach ( i )

set_property ( TARGET openobex PROPERTY
  DEFINE_SYMBOL OPENOBEX_EXPORTS
)

install ( TARGETS openobex
  RUNTIME       DESTINATION bin
  LIBRARY       DESTINATION lib
  ARCHIVE       DESTINATION lib
  PUBLIC_HEADER DESTINATION include/openobex
  COMPONENT library
)

#
# Copy the .pc file to install it only if the lib gets installed
#
add_custom_command ( TARGET openobex
  COMMAND ${CMAKE_COMMAND}
  ARGS    -E copy_if_different ${PROJECT_BINARY_DIR}/openobex.pc
          ${CMAKE_CURRENT_BINARY_DIR}/openobex.pc
  VERBATIM
)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/openobex.pc
  DESTINATION ${PKGCONFIG_INSTALL_DIR}
  OPTIONAL
)


#  include ( GetPrerequisites )
#  get_prerequisites ( openobex CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS 0 1 )
include ( InstallRequiredSystemLibraries )

