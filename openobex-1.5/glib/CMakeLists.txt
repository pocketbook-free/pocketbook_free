
set ( openobex-glib_VERSION 1.0.0 )
set ( openobex-glib_SOVERSION 0 )

find_package ( Glib REQUIRED object thread)

set ( GENERATED_SOURCES
  obex-marshal.c
  obex-marshal.h
)
set_property ( SOURCE ${GENERATED_SOURCES} PROPERTY
  GENERATED true
)

if ( GLIB_GENMARSHAL_EXECUTABLE )
  add_custom_command (
    OUTPUT            ${CMAKE_CURRENT_BINARY_DIR}/obex-marshal.c
    COMMAND           ${GLIB_GENMARSHAL_EXECUTABLE}
    ARGS              --prefix=obex_marshal obex-marshal.list --body
                      > ${CMAKE_CURRENT_BINARY_DIR}/obex-marshal.c
    MAIN_DEPENDENCY   ${CMAKE_CURRENT_SOURCE_DIR}/obex-marshal.list
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_custom_command (
    OUTPUT            ${CMAKE_CURRENT_BINARY_DIR}/obex-marshal.h
    COMMAND           ${GLIB_GENMARSHAL_EXECUTABLE}
    ARGS              --prefix=obex_marshal obex-marshal.list --header
                      > ${CMAKE_CURRENT_BINARY_DIR}/obex-marshal.h
    MAIN_DEPENDENCY   ${CMAKE_CURRENT_SOURCE_DIR}/obex-marshal.list
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )

else ( GLIB_GENMARSHAL_EXECUTABLE )
  message ( FATAL_ERROR "glib-genmarshal not found but needed." )
endif ( GLIB_GENMARSHAL_EXECUTABLE )

set ( SOURCES
  obex-client.c
  obex-error.c
  obex-lowlevel.c
  ${GENERATED_SOURCES}
)

set ( HEADERS
  obex-client.h
  obex-debug.h
  obex-error.h
  obex-lowlevel.h
)

set ( openobex-glib_PUBLIC_HEADER
  obex-client.h
)

include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Glib_INCLUDE_DIRS}
)

if ( COMPILER_SUPPORT_NOUNDEFINED )
  set ( openobex-glib_LINK_FLAGS
    "${openobex-glib_LINK_FLAGS} ${LINKER_FLAG_NOUNDEFINED}"
  )
endif ( COMPILER_SUPPORT_NOUNDEFINED )

add_library ( openobex-glib
  ${SOURCES}
)

target_link_libraries ( openobex-glib
  ${Glib_LIBRARIES}
  openobex
)

foreach ( i VERSION SOVERSION PUBLIC_HEADER LINK_FLAGS )
  if ( DEFINED openobex-glib_${i} )
    set_property ( TARGET openobex-glib PROPERTY ${i} ${openobex-glib_${i}} )
  endif ( DEFINED openobex-glib_${i} )
endforeach ( i )

set_property ( TARGET openobex-glib PROPERTY
  DEFINE_SYMBOL OPENOBEX_GLIB_EXPORTS
)

if ( WIN32 )
  target_link_libraries ( openobex-glib 
    ws2_32
  )
endif ( WIN32 )

install ( TARGETS openobex-glib
  RUNTIME       DESTINATION bin
  LIBRARY       DESTINATION lib
  ARCHIVE       DESTINATION lib
  PUBLIC_HEADER DESTINATION include/openobex
  COMPONENT bindings
)
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/obex-client.h
  DESTINATION include/openobex
  COMPONENT bindings
) 

#
# Copy the .pc file to install it only if the lib gets installed
#
add_custom_command ( TARGET openobex-glib
  COMMAND ${CMAKE_COMMAND}
  ARGS    -E copy_if_different ${CMAKE_BINARY_DIR}/openobex-glib.pc
          ${CMAKE_CURRENT_BINARY_DIR}/openobex-glib.pc
  VERBATIM
)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/openobex-glib.pc
  DESTINATION ${PKGCONFIG_INSTALL_DIR}
  OPTIONAL
)



#
# These are some test apps for the library above.
#
set ( openobex-glib_TESTS
  test-client
  test-lowlevel
)
foreach ( prog ${openobex-glib_TESTS} )
  add_executable ( openobex-glib-${prog} EXCLUDE_FROM_ALL
    ${prog}.c
  )
  target_link_libraries ( openobex-glib-${prog}
    openobex-glib
    ${Glib_LIBRARIES}
  )
endforeach ( prog )
add_custom_target ( openobex-glib-tests
  DEPENDS ${openobex-glib_TESTS}
)
