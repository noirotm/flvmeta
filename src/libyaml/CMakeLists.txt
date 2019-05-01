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

add_library(yaml ${LIBYAML_SRC})

target_include_directories(yaml BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/src/libyaml)
