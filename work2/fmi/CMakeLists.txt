include(FetchContent)
include(ExternalProject)


set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(intocpsfmi
        GIT_REPOSITORY https://github.com/INTO-CPS-Association/org.intocps.maestro.fmi.git
        GIT_TAG  Release/1.0.12
        GIT_SHALLOW ON
        SOURCE_DIR "${CMAKE_BINARY_DIR}/intocpsfmi-src"
        )

FetchContent_MakeAvailable(intocpsfmi)
set(intocpsfmi-src "intocpsfmi-src")


SET(BUILD_TOOLS OFF CACHE BOOL "Build tools in the src directory (zipcmp, zipmerge, ziptool)")
SET(BUILD_REGRESS OFF CACHE BOOL "Build regression tests")
SET(BUILD_EXAMPLES OFF CACHE BOOL "Build examples")
SET(BUILD_DOC OFF CACHE BOOL "Build documentation")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

FetchContent_Declare(libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG  v1.11
        GIT_SHALLOW ON
        SOURCE_DIR "${CMAKE_BINARY_DIR}/libzip"
        )


set(ENABLE_GNUTLS OFF CACHE BOOL "")
set(ENABLE_MBEDTLS OFF CACHE BOOL "")
set(ENABLE_OPENSSL OFF CACHE BOOL "")
set(ENABLE_WINDOWS_CRYPTO OFF CACHE BOOL "")

set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_REGRESS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
set(BUILD_DOC OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

FetchContent_MakeAvailable(libzip )





add_library(fmi STATIC
        ${intocpsfmi_SOURCE_DIR}/jnifmuapi/src/main/native/src/sim_support.c
        ${CMAKE_CURRENT_SOURCE_DIR}/uri.c

        ${CMAKE_CURRENT_SOURCE_DIR}/unzip.cpp
       )

target_include_directories(fmi PUBLIC ${PROJECT_SOURCE_DIR}
        ${intocpsfmi_SOURCE_DIR}/jnifmuapi/src/main/native/src/external/shared/fmi/include
        ${intocpsfmi_SOURCE_DIR}/jnifmuapi/src/main/native/src/external/shared
        ${intocpsfmi_SOURCE_DIR}/jnifmuapi/src/main/native/src
        ${CMAKE_CURRENT_SOURCE_DIR}/
        )

target_link_libraries(fmi zip)
target_compile_definitions(fmi PRIVATE -DFMI_COSIMULATION)