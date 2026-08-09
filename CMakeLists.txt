cmake_minimum_required(VERSION 4.3)
project(wxTetris)
include(xptoplevel)
set(exe_srcs src/App.cpp src/Tetris.cpp src/Tetris.hpp LICENSE)
add_executable(${PROJECT_NAME} WIN32 ${exe_srcs})
find_package(wxWidgets)
target_link_libraries(${PROJECT_NAME} PRIVATE wxWidgets::core)
set(targetsFile ${PROJECT_NAME}-targets)
xpExternPackage(TARGETS_FILE ${targetsFile} EXE ${PROJECT_NAME}
  BASE v0 XPDIFF "intro"
  WEB "https://github.com/smanders/wxTetris"
  DESC "wxWidgets Tetris game"
  LICENSE "[MIT](https://github.com/smanders/wxTetris/blob/dev/LICENSE 'MIT License')"
  )
install(TARGETS ${PROJECT_NAME} EXPORT ${targetsFile})
install(EXPORT ${targetsFile} DESTINATION ${CMAKE_INSTALL_CMAKEDIR})
xpSourceListAppend("${exe_srcs}")
