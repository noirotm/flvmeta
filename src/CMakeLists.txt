set(flvmeta_src
  amf.c
  amf.h
  avc.c
  avc.h
  bitstream.c
  bitstream.h
  check.c
  check.h
  dump.c
  dump.h
  dump_json.c
  dump_json.h
  dump_raw.c
  dump_raw.h
  dump_xml.c
  dump_xml.h
  dump_yaml.c
  dump_yaml.h
  flv.c
  flv.h
  flvmeta.c
  flvmeta.h
  info.c
  info.h
  json.c
  json.h
  types.c
  types.h
  update.c
  update.h
  util.c
  util.h
  ${CMAKE_BINARY_DIR}/config.h
)

# add support for getopt and gettext in windows
if(WIN32)
  set(flvmeta_src
    ${flvmeta_src}
    compat/getopt1.c
    compat/getopt.c
    compat/getopt.h
    compat/gettext.h
  )
  include_directories(compat)
endif()

# static build
if(WIN32)
  add_definitions(-DYAML_DECLARE_STATIC)
endif()

add_executable(flvmeta ${flvmeta_src})

# link with libm for isfinite when needed
if(HAVE_ISFINITE AND NOT MSVC)
  target_link_libraries(flvmeta m)
endif()

# libyaml
if(FLVMETA_USE_SYSTEM_LIBYAML)
  # search for libyaml on the system, link with it
  find_package(LibYAML REQUIRED)
  target_include_directories(flvmeta PRIVATE ${LIBYAML_INCLUDE_DIR})
  target_link_libraries(flvmeta ${LIBYAML_LIBRARIES})
else()
  # use bundled version of libyaml
  target_include_directories(flvmeta PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libyaml)
  add_subdirectory(libyaml)
  target_link_libraries(flvmeta yaml)
endif()

if(WIN32)
  install(
    TARGETS flvmeta
    RUNTIME DESTINATION .
  )
else()
  install(
    TARGETS flvmeta
    RUNTIME DESTINATION bin
  )
endif()
