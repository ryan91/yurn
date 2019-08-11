PROJECT(yurn C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

add_custom_command(OUTPUT resources.c
  PRE_BUILD
  COMMAND glib-compile-resources --generate --target=${CMAKE_BINARY_DIR}/resources.c exampleapp.gresource.xml
  DEPENDS exampleapp.gresource.xml window.glade app-menu.ui
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )

add_custom_command(OUTPUT yurn_style.h
  PRE_BUILD
  COMMAND xxd --include yurn.css > ${CMAKE_BINARY_DIR}/yurn_style.h
  DEPENDS yurn.css
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
add_custom_target(yurn_style DEPENDS yurn_style.h)

add_executable(yurn
  main.c
  exampleapp.c
  exampleappwin.c
  resources.c
  gamedata.c
  jsonparser.c)
target_link_libraries(yurn PRIVATE ${GTK3_LIBRARIES} jansson)
target_include_directories(yurn PRIVATE ${CMAKE_BINARY_DIR})
target_compile_options(yurn PRIVATE -Wall -g)
add_dependencies(yurn yurn_style)
