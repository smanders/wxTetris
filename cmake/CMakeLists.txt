set(utility_project cmake)
########################################
file(GLOB dotcmake_srcs *.cmake ${CMAKE_SOURCE_DIR}/*.cmake)
source_group(.cmake FILES ${dotcmake_srcs})
list(APPEND ${utility_project}_srcs ${dotcmake_srcs})
###########
file(GLOB externpro_cmake ${externpro_DIR}/share/cmake/*.cmake)
source_group(externpro.cmake FILES ${externpro_cmake})
file(GLOB externpro_in ${externpro_DIR}/share/cmake/*.in)
source_group(externpro.in FILES ${externpro_in})
list(APPEND ${utility_project}_srcs ${externpro_cmake} ${externpro_in})
########################################
add_custom_target(${utility_project} SOURCES ${${utility_project}_srcs})
set_property(TARGET ${utility_project} PROPERTY FOLDER ${folder})
xpSourceListAppend("${${utility_project}_srcs}")
