project(xrock_gui_model)
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "A")
cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

find_package(lib_manager)
lib_defaults()
define_module_info()

add_definitions(-std=c++11)
set(FILE_DB 1)

set(QT_USE_QTWEBKIT 1)
setup_qt()

pkg_check_modules(PKGCONFIG REQUIRED
          lib_manager
          bagel_gui
          osg_graph_viz
          configmaps
          mars_utils
          main_gui
          config_map_gui
          cfg_manager
)

include_directories(${PKGCONFIG_INCLUDE_DIRS})
link_directories(${PKGCONFIG_LIBRARY_DIRS})
add_definitions(${PKGCONFIG_CFLAGS_OTHER})  #flags excluding the ones with -I

include_directories(
  src
)

set(SOURCES 
  src/Model.cpp
  src/ModelLib.cpp
  src/ModelWidget.cpp
  src/ImportDialog.cpp
  src/VersionDialog.cpp
  src/ConfigureDialog.cpp
  src/ConfigMapHelper.cpp
  src/FileDB.cpp
)

set(HEADERS
  src/Model.hpp
  src/ModelLib.hpp
  src/ModelWidget.hpp
  src/ImportDialog.hpp
  src/VersionDialog.hpp
  src/ConfigureDialog.hpp
  src/ConfigMapHelper.hpp
  src/FileDB.hpp
)

set (QT_MOC_HEADER
  src/ModelWidget.hpp
  src/ImportDialog.hpp
  src/VersionDialog.hpp
  src/ConfigureDialog.hpp
)

if (${USE_QT5})
qt5_wrap_cpp ( QT_MOC_HEADER_SRC ${QT_MOC_HEADER} )
else (${USE_QT5})
qt4_wrap_cpp ( QT_MOC_HEADER_SRC ${QT_MOC_HEADER} )
endif (${USE_QT5})

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${QT_MOC_HEADER_SRC})

if (${USE_QT5})
qt5_use_modules(${PROJECT_NAME} Widgets WebKitWidgets)
endif (${USE_QT5})


target_link_libraries(${PROJECT_NAME}
                      ${PKGCONFIG_LIBRARIES}
                      ${QT_LIBRARIES}
)

if(WIN32)
  set(LIB_INSTALL_DIR bin) # .dll are in PATH, like executables
else(WIN32)
  set(LIB_INSTALL_DIR lib)
endif(WIN32)


set(_INSTALL_DESTINATIONS
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION lib
)

# Install the library into the lib folder
install(TARGETS ${PROJECT_NAME} ${_INSTALL_DESTINATIONS})

# Install headers into mars include directory
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

# Prepare and install necessary files to support finding of the library 
# using pkg-config
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)


install(DIRECTORY configuration/ DESTINATION configuration/)
INSTALL(PROGRAMS bin/xrock_gui DESTINATION bin)
INSTALL(PROGRAMS bin/shader_gui DESTINATION bin)
INSTALL(PROGRAMS bin/cnd_gui DESTINATION bin)
INSTALL(PROGRAMS bin/orogen_to_xrock DESTINATION bin)
