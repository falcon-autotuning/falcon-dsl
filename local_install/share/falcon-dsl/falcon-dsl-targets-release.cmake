#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "falcon::falcon-atc-core" for configuration "Release"
set_property(TARGET falcon::falcon-atc-core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(falcon::falcon-atc-core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libfalcon-atc-core.a"
  )

list(APPEND _cmake_import_check_targets falcon::falcon-atc-core )
list(APPEND _cmake_import_check_files_for_falcon::falcon-atc-core "${_IMPORT_PREFIX}/lib/libfalcon-atc-core.a" )

# Import target "falcon::falcon-dsl" for configuration "Release"
set_property(TARGET falcon::falcon-dsl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(falcon::falcon-dsl PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "fmt::fmt"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libfalcon-dsl.so"
  IMPORTED_SONAME_RELEASE "libfalcon-dsl.so"
  )

list(APPEND _cmake_import_check_targets falcon::falcon-dsl )
list(APPEND _cmake_import_check_files_for_falcon::falcon-dsl "${_IMPORT_PREFIX}/lib/libfalcon-dsl.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
