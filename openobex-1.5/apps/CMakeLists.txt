
#
# a static library to not compile those multiple times
#
add_library ( openobex-apps-common STATIC EXCLUDE_FROM_ALL
  obex_io.c obex_io.h
  obex_put_common.c obex_put_common.h
)

set ( OPENOBEX_APPS
  obex_test
  irxfer
  obex_tcp
  irobex_palm3
)

foreach ( prog ${OPENOBEX_APPS} )
  set ( ${prog}_SOURCES ${prog}.c )
  set ( ${prog}_LIBS
    openobex-apps-common
    openobex
  )
endforeach ( prog )

list ( APPEND obex_test_SOURCES
  obex_test.h
  obex_test_client.c obex_test_client.h
  obex_test_server.c obex_test_server.h
  obex_test_cable.c obex_test_cable.h
)

list ( APPEND obex_test_LIBS
  ${Bluetooth_LIBRARIES}
)

if ( WIN32 )
  list ( APPEND obex_tcp_LIBS ws2_32 )
  list ( APPEND obex_test_LIBS ws2_32 )
endif ( WIN32 )

foreach ( prog ${OPENOBEX_APPS} )
  add_executable ( ${prog} EXCLUDE_FROM_ALL ${${prog}_SOURCES} )
  target_link_libraries ( ${prog} ${${prog}_LIBS} )
  get_target_property ( ${prog}_EXECUTABLE ${prog} LOCATION )
  install ( PROGRAMS ${${prog}_EXECUTABLE}
    DESTINATION bin
    COMPONENT applications
    OPTIONAL
  )
endforeach ( prog )
add_dependencies ( openobex-apps ${OPENOBEX_APPS} )
