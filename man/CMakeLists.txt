# test for pandoc availability
if(NOT DEFINED PANDOC)
  message(STATUS "Looking for pandoc")
  find_program(PANDOC pandoc)
  if(PANDOC)
    message(STATUS "Looking for pandoc - found")
  else()
    message(STATUS "Looking for pandoc - not found")
  endif()
  mark_as_advanced(PANDOC)
endif()

if(PANDOC)
  set(MAN_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/flvmeta.1.md)
  set(MAN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/man/man1)
  set(MAN_FILE ${MAN_OUTPUT_DIR}/flvmeta.1)

  # build man file from markdown source
  add_custom_command(
    OUTPUT ${MAN_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MAN_OUTPUT_DIR}
    COMMAND ${PANDOC} -s ${MAN_SOURCE} -o ${MAN_FILE}
    MAIN_DEPENDENCY ${MAN_SOURCE}
  )
  
  # add custom target, built by default
  add_custom_target(man ALL
    DEPENDS ${MAN_FILE}
    SOURCES ${MAN_SOURCE}
  )
  
  # install man file on systems having a manual directory
  include(GNUInstallDirs)
  if(CMAKE_INSTALL_MANDIR AND NOT WIN32)
    install(DIRECTORY ${MAN_OUTPUT_DIR}
      DESTINATION ${CMAKE_INSTALL_MANDIR}
    )
  endif()
endif()
