set(flvmeta_src
    amf.c
    amf.h
    flv.c
    flv.h
    flvmeta.c
    flvmeta.h
    mmap.c
    mmap.h
    types.c
    types.h
    update.c
    update.h
)

include_directories(${CMAKE_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H)

if(WIN32)
    set(flvmeta_src ${flvmeta_src}
        ${PROJECT_SOURCE_DIR}/msvc/getopt1.c
        ${PROJECT_SOURCE_DIR}/msvc/getopt.c
        ${PROJECT_SOURCE_DIR}/msvc/getopt.h
        ${PROJECT_SOURCE_DIR}/msvc/gettext.h
        ${PROJECT_SOURCE_DIR}/msvc/config.h
    )
    include_directories(${PROJECT_SOURCE_DIR}/msvc)
endif(WIN32)

# C runtime deprecation in Visual C++ 2005 and later
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

add_executable(flvmeta ${flvmeta_src})
