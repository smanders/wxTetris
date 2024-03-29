# - Find an externpro installation.
# externpro_DIR
################################################################################
# should match xpGetCompilerPrefix in externpro's xpfunmac.cmake
# NOTE: wanted to use externpro version, but chicken-egg problem
function(getCompilerPrefix _ret)
  set(options GCC_TWO_VER)
  cmake_parse_arguments(X "${options}" "" "" ${ARGN})
  if(MSVC)
    set(prefix vc${MSVC_TOOLSET_VERSION})
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    exec_program(${CMAKE_CXX_COMPILER}
      ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpfullversion -dumpversion
      OUTPUT_VARIABLE GCC_VERSION
      )
    if(X_GCC_TWO_VER)
      set(digits "\\1\\2")
    else()
      set(digits "\\1\\2\\3")
    endif()
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)?"
      "gcc${digits}"
      prefix ${GCC_VERSION}
      )
  elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") # LLVM/Apple Clang (clang.llvm.org)
    if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
      exec_program(${CMAKE_CXX_COMPILER}
        ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
        OUTPUT_VARIABLE CLANG_VERSION
        )
      string(REGEX REPLACE "([0-9]+)\\.([0-9]+)(\\.[0-9]+)?"
        "clang-darwin\\1\\2" # match boost naming
        prefix ${CLANG_VERSION}
        )
    else()
      string(REGEX REPLACE "([0-9]+)\\.([0-9]+)(\\.[0-9]+)?"
        "clang\\1\\2" # match boost naming
        prefix ${CMAKE_CXX_COMPILER_VERSION}
        )
    endif()
  else()
    message(SEND_ERROR "Findexternpro.cmake: compiler support lacking: ${CMAKE_CXX_COMPILER_ID}")
  endif()
  set(${_ret} ${prefix} PARENT_SCOPE)
endfunction()
function(getNumBits _ret)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(numBits 64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(numBits 32)
  else()
    message(FATAL_ERROR "numBits not 64 or 32")
  endif()
  set(${_ret} ${numBits} PARENT_SCOPE)
endfunction()
################################################################################
# TRICKY: clear cached variables each time we cmake so we can change
# externpro_REV and reuse the same build directory
unset(externpro_DIR CACHE)
################################################################################
# find the path to the externpro directory
getCompilerPrefix(COMPILER)
getNumBits(BITS)
# projects using externpro: set(externpro_REV `git describe --tags`)
set(externpro_SIG ${externpro_REV}-${COMPILER}-${BITS})
# TRICKY: match what is done in cmake's Modules/CPack.cmake, setting CPACK_SYSTEM_NAME
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(XP_SYSTEM_NAME win${BITS})
else()
  set(XP_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
endif()
set(XP_DEV_BUILD_NAME "externpro_${externpro_SIG}")
set(XP_INSTALLED_NAME "externpro-${externpro_SIG}-${XP_SYSTEM_NAME}")
# NOTE: environment variable setting examples:
# set(ENV{externpro_BUILD_DIR} ~/src/externpro/_bld)
# set(ENV{extern_DIR} ~/extern)
find_path(externpro_DIR
  NAMES
    externpro_${externpro_SIG}.txt
  PATHS
    # developer/build versions
    "$ENV{externpro_BUILD_DIR}/${XP_DEV_BUILD_NAME}"
    # installed versions
    "$ENV{extern_DIR}/${XP_INSTALLED_NAME}"
    "~/extern/${XP_INSTALLED_NAME}"
    "/opt/extern/${XP_INSTALLED_NAME}"
    "C:/opt/extern/${XP_INSTALLED_NAME}"
    "C:/dev/extern/${XP_INSTALLED_NAME}"
  DOC "externpro directory"
  )
if(NOT externpro_DIR)
  if(EXISTS $ENV{extern_DIR})
    set(archive_name "${XP_INSTALLED_NAME}.tar.xz")
    message(STATUS "${XP_INSTALLED_NAME} not found.")
    message(STATUS "Attempting download of ${archive_name} ...")
    file(DOWNLOAD https://github.com/smanders/externpro/releases/download/${externpro_REV}/${archive_name}
      $ENV{extern_DIR}/${archive_name}
      )
    message(STATUS "Attempting extraction of ${archive_name} ...")
    file(ARCHIVE_EXTRACT
      INPUT $ENV{extern_DIR}/${archive_name}
      DESTINATION $ENV{extern_DIR}
      )
    message(STATUS "Attempting to remove ${archive_name}")
    file(REMOVE $ENV{extern_DIR}/${archive_name})
    if(EXISTS $ENV{extern_DIR}/${XP_INSTALLED_NAME})
      set(externpro_DIR $ENV{extern_DIR}/${XP_INSTALLED_NAME})
    else()
      message(AUTHOR_WARNING "Automatic download and extraction failed. Verify https://github.com/smanders/externpro/releases/download/${externpro_REV}/${archive_name} exists and can be accessed.")
    endif()
  endif()
endif()
if(NOT externpro_DIR)
  set(externpro_INSTALL_INFO ".\n Installers located at https://github.com/smanders/externpro/releases\n tar -xf /path/to/externpro*.tar.xz --directory=/path/to/install/\n ** or set extern_DIR in ENV for automatic download and extraction\n") # externpro can set(XP_INSTALL_INFO) to define this
  if(DEFINED externpro_INSTALLER_LOCATION) # defined by project using externpro
    message(FATAL_ERROR "externpro ${externpro_SIG} not found.\n${externpro_INSTALLER_LOCATION}")
  else()
    message(FATAL_ERROR "externpro ${externpro_SIG} not found${externpro_INSTALL_INFO}")
  endif()
else()
  message(STATUS "Found externpro: ${externpro_DIR}")
  set(moduleDir ${externpro_DIR}/share/cmake)
  list(APPEND XP_MODULE_PATH ${moduleDir})
  set(FPHSA_NAME_MISMATCHED TRUE) # find_package_handle_standard_args NAME_MISMATCHED (prefix usexp-)
  if(EXISTS ${moduleDir}/xpfunmac.cmake)
    include(${moduleDir}/xpfunmac.cmake)
  endif()
  if(COMMAND xpCheckInstall)
    xpCheckInstall(externpro)
  endif()
endif()
