set(flvmeta_src
  amf.c
  amf.h
  avc.c
  avc.h
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
    compat/win32_tmpfile.c
    compat/win32_tmpfile.h
  )
  include_directories(compat)
endif(WIN32)

# libyaml

# static build
if (WIN32)
  add_definitions(-DYAML_DECLARE_STATIC)
endif (WIN32)

include_directories(libyaml)
add_subdirectory(libyaml)

add_executable(flvmeta ${flvmeta_src})
target_link_libraries(flvmeta yaml)

if(WIN32)
  install(TARGETS flvmeta
    RUNTIME DESTINATION .)
else(WIN32)
  install(TARGETS flvmeta
    RUNTIME DESTINATION bin)
endif(WIN32)
