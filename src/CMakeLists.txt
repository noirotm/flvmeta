set(flvmeta_src
  amf.c
  amf.h
  avc.c
  avc.h
  dump.c
  dump.h
  flv.c
  flv.h
  flvmeta.c
  flvmeta.h
  json.c
  json.h
  types.c
  types.h
  inject.c
  inject.h
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
endif(WIN32)

add_executable(flvmeta ${flvmeta_src})

if(WIN32)
  install(TARGETS flvmeta
    RUNTIME DESTINATION .)
else(WIN32)
  install(TARGETS flvmeta
    RUNTIME DESTINATION bin)
endif(WIN32)
