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
  DEPENDS exampleapp.gresource.xml window.ui app-menu.ui
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )

ADD_EXECUTABLE(yurn main.c exampleapp.c exampleappwin.c resources.c)
TARGET_LINK_LIBRARIES(yurn ${GTK3_LIBRARIES})
