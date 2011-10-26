
add_custom_target ( doc )

set ( SOURCES
  "${CMAKE_SOURCE_DIR}/lib/obex.c"
  "${CMAKE_SOURCE_DIR}/include/openobex/obex.h"
  "${CMAKE_SOURCE_DIR}/include/openobex/obex_const.h"
  "${CMAKE_SOURCE_DIR}/doc/openobex.coverpage"
)

find_package( Doxygen )
if ( DOXYGEN_FOUND )
  string ( REPLACE ";" " " DOC_SOURCE_FILES "${SOURCES}" )
  set ( DOC_VERSION ${SHORT_VERSION} )
  set ( DOC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )
  set ( DOC_OUTPUT_TYPES
    HTML
    LATEX
  )
  set ( DOC_HTML_OUTPUT_FILE "index.html" )
  set ( DOC_LATEX_OUTPUT_FILE "refman.tex" )

  foreach ( type ${DOC_OUTPUT_TYPES} )
    foreach ( i ${DOC_OUTPUT_TYPES} )
      set ( DOC_${i}_OUTPUT "NO" )
    endforeach ( i )
    set ( DOC_${type}_OUTPUT "YES" )
    configure_file (
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_${type}
      @ONLY
    )
    if ( DOC_${type}_OUTPUT_FILE )
      string ( TOLOWER "${type}" type_low )
      set ( DOC_${type}_OUTPUT_FILE_ABS "${CMAKE_CURRENT_BINARY_DIR}/${type_low}/${DOC_${type}_OUTPUT_FILE}" )

      add_custom_command ( OUTPUT "${DOC_${type}_OUTPUT_FILE_ABS}"
          COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_${type}"
          DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" ${SOURCES}
          VERBATIM
      )

      add_custom_target ( apidoc_${type_low} DEPENDS ${DOC_${type}_OUTPUT_FILE_ABS} )
    endif ( DOC_${type}_OUTPUT_FILE )
  endforeach ( type )

  set_directory_properties ( PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "html;latex"
  )
  add_dependencies ( openobex apidoc_html )
  add_dependencies ( doc apidoc_html )

  install ( DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
    DESTINATION ${DOCUMENTATION_INSTALL_DIR}
    COMPONENT documentation
  )
endif ( DOXYGEN_FOUND )

#
# xsltproc is preferred because it creates better output and is faster.
#
# xalan2 can also be used by adding xalan2.jar, xml-apis.jar, xercesImpl.jar
# and maybe docbook saxon extension jar file to the classpath, then runinng
# org.apache.xalan.xslt.Process
# There is also a xalan (1.10) binary of that name that did not work.
#
# Saxon-6.5.5 can also be used by adding saxon.jar to classpath, then running
# com.icl.saxon.StyleSheet
# Debian has wrapper shell scripts for saxon-6.5.5, called saxon-xslt.
# Saxon-B 9.1 did not work, Saxon-SA was not tested.
#
# Sablotron (1.0.3) does not work.
#
find_program ( XSLT_PROCESSOR
  NAMES xsltproc saxon-xslt
  DOC   "XSLT processor command (xsltproc, xalan2 or saxon are usable)"
)
mark_as_advanced ( XSLT_PROCESSOR )

#
# call as XSLT_TRANSFORM(<.xsl file URL> <input file> <output file...>)
#
macro ( XSL_TRANSFORM xslurl infile )
  string ( REPLACE " " ";" XSL_TRANSFORM_COMMAND ${XSLT_PROCESSOR} )
  if ( XSLT_PROCESSOR MATCHES ".*xsltproc.*" )
    list ( APPEND XSL_TRANSFORM_COMMAND
      --param use.id.as.filename 1
      ${xslurl} ${infile}
    )
  elseif ( XSLT_PROCESSOR MATCHES ".*xalan2.*" )
    list ( APPEND XSL_TRANSFORM_COMMAND
      -param use.id.as.filename 1
      -in ${infile} -xsl ${xslurl}
    )
  elseif ( XSLT_PROCESSOR MATCHES ".*saxon.*" )
    list ( APPEND XSL_TRANSFORM_COMMAND
      ${infile} ${xslurl}
      use.id.as.filename=1
    )
  else ( XSLT_PROCESSOR MATCHES ".*xsltproc.*" )
    message ( FATAL_ERROR "Unsupported XSLT processor" )
  endif ( XSLT_PROCESSOR MATCHES ".*xsltproc.*" )
  add_custom_command(
    OUTPUT ${ARGN}
    COMMAND ${XSL_TRANSFORM_COMMAND}
    DEPENDS openobex-apps.xml
    VERBATIM
  )
endmacro ( XSL_TRANSFORM )

if ( XSLT_PROCESSOR )
  if ( UNIX )
    option (BUILD_APPS_HELP_MAN "Build manpage application help files" ON)
    option (BUILD_APPS_HELP_HTML "Build HTML application help files" OFF)
  else ( UNIX )
    option (BUILD_APPS_HELP_HTML "Build HTML application help files" ON)
  endif ( UNIX )

  SET ( DOCBOOK_XSL_PREFIX "http://docbook.sourceforge.net/release/xsl/current"
        CACHE STRING "prefix to locate the docbook-XSL release files" )
  mark_as_advanced ( DOCBOOK_XSL_PREFIX )

  file ( READ "${CMAKE_CURRENT_SOURCE_DIR}/openobex-apps.xml" XML_FILE_CONTENTS )
  string ( REGEX MATCHALL "<refentrytitle>[^<]*" XML_REFENTRYTITLE "${XML_FILE_CONTENTS}" )
  foreach ( file ${XML_REFENTRYTITLE} )
    string ( REGEX REPLACE "^<refentrytitle>" "" file "${file}" )
    string ( REGEX REPLACE "[[:space:]]" "" file "${file}" )
    list ( APPEND XSLT_FILES ${file} )
  endforeach ( file )
  set ( XML_FILE_CONTENTS )
  set ( XML_REFENTRYTITLE )

  if ( BUILD_APPS_HELP_MAN )
    foreach ( file  ${XSLT_FILES} )
      list ( APPEND XSLT_FILES_MAN ${file}.1 )
    endforeach ( file )
    xsl_transform (
      "${DOCBOOK_XSL_PREFIX}/manpages/docbook.xsl"
      "${CMAKE_CURRENT_SOURCE_DIR}/openobex-apps.xml"
      ${XSLT_FILES_MAN}
    )
    foreach ( file ${XSLT_FILES_MAN} )
      list ( APPEND OPENOBEX_HELP_FILES ${file} )
      get_filename_component ( fileExt ${file} EXT )
      string ( REGEX REPLACE "^[.]" "" fileExt ${fileExt} )
      install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${file}
	DESTINATION ${MANPAGE_INSTALL_DIR}/man${fileExt}
	PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ
        COMPONENT documentation
      )
    endforeach ( file )
  endif ( BUILD_APPS_HELP_MAN )

  if ( BUILD_APPS_HELP_HTML )
    foreach ( file  ${XSLT_FILES} )
      list ( APPEND XSLT_FILES_HTML ${file}.html )
    endforeach ( file )
    list ( APPEND XSLT_FILES_HTML
      ${CMAKE_CURRENT_BINARY_DIR}/index.html
    )
    xsl_transform (
      "${DOCBOOK_XSL_PREFIX}/xhtml/chunk.xsl"
      "${CMAKE_CURRENT_SOURCE_DIR}/openobex-apps.xml"
      ${XSLT_FILES_HTML}
    )
    foreach ( file ${XSLT_FILES_HTML} )
      list ( APPEND OPENOBEX_HELP_FILES ${file} )
      INSTALL (
	FILES ${file}
        DESTINATION ${DOCUMENTATION_INSTALL_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ
        COMPONENT documentation
      )
    endforeach ( file )
  endif ( BUILD_APPS_HELP_HTML )

  add_custom_target ( openobex-apps-doc DEPENDS ${OPENOBEX_HELP_FILES} )
  add_dependencies ( openobex-apps openobex-apps-doc )
  add_dependencies ( doc openobex-apps-doc )
endif ( XSLT_PROCESSOR )
