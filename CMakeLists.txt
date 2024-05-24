cmake_minimum_required(VERSION 3.29)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

project(ConfigurationsPP
  VERSION 1.0.0
  DESCRIPTION "easy to use and powerful command line options parser"
  HOMEPAGE_URL "https://github.com/changhaoxuan23/configurationspp"
  LANGUAGES CXX
)

add_library(ConfigurationsPP SHARED configuration.cc)
target_compile_options(ConfigurationsPP PUBLIC -stdlib=libc++)
target_link_options(ConfigurationsPP PUBLIC -stdlib=libc++)
set_property(TARGET ConfigurationsPP PROPERTY VERSION ${PROJECT_VERSION})
set_property(TARGET ConfigurationsPP PROPERTY SOVERSION 1)
set_property(TARGET ConfigurationsPP PROPERTY INTERFACE_ConfigurationsPP_VERSION 1)
set_property(TARGET ConfigurationsPP APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING INTERFACE_ConfigurationsPP_VERSION
)
target_include_directories(ConfigurationsPP INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS ConfigurationsPP EXPORT ConfigurationsPPTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
install(
  FILES
    configuration.hh
  DESTINATION
    include
  COMPONENT
    Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationsPP/ConfigurationsPPConfigVersion.cmake"
  VERSION ${Upstream_VERSION}
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT ConfigurationsPPTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationsPP/ConfigurationsPPTargets.cmake"
)
configure_file(cmake/ConfigurationsPPConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationsPP/ConfigurationsPPConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/ConfigurationsPP)
install(EXPORT ConfigurationsPPTargets
  FILE
  ConfigurationsPPTargets.cmake
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
    cmake/ConfigurationsPPConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/ConfigurationsPP/ConfigurationsPPConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)