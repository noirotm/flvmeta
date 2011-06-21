set(LIBYAML_SRC
  api.c
  config.h
  dumper.c
  emitter.c
  loader.c
  parser.c
  reader.c
  scanner.c
  writer.c
  yaml.h
  yaml_private.h
)

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src/libyaml)

add_library(yaml STATIC ${LIBYAML_SRC})