cmake_minimum_required(VERSION 3.10)
project(xrock_gui_model VERSION 1.0.0 DESCRIPTION "xrock_gui")
find_package(PkgConfig REQUIRED)
find_package(lib_manager)

lib_defaults()
define_module_info()
if(APPLE)
add_definitions(-std=c++17)
add_definitions(-DGL_SILENCE_DEPRECATION)
endif(APPLE)
add_definitions(-DXROCK_DEFAULT_RESOURCES_PATH=\"${CMAKE_INSTALL_PREFIX}/share\")

set(FILE_DB 1)

set(QT_USE_QTWEBKIT 1)
setup_qt()
if (${USE_QT5})
  QT5_ADD_RESOURCES(icon_resource resources/resources.qrc)
  set(PUBLIC_DEPENDECIES Qt5Widgets)
else()
  QT4_ADD_RESOURCES(icon_resource resources/resources.qrc)
  set(PUBLIC_DEPENDENCIES "QtCore QtGui")
endif()
pkg_check_modules(lib_manager REQUIRED IMPORTED_TARGET lib_manager)
pkg_check_modules(bagel_gui REQUIRED IMPORTED_TARGET bagel_gui)
pkg_check_modules(osg_graph_viz REQUIRED IMPORTED_TARGET osg_graph_viz)
pkg_check_modules(configmaps REQUIRED IMPORTED_TARGET configmaps)
pkg_check_modules(mars_utils REQUIRED IMPORTED_TARGET mars_utils)
pkg_check_modules(main_gui REQUIRED IMPORTED_TARGET main_gui)
pkg_check_modules(config_map_gui REQUIRED IMPORTED_TARGET config_map_gui)
pkg_check_modules(cfg_manager REQUIRED IMPORTED_TARGET cfg_manager)
pkg_check_modules(smurf_parser REQUIRED IMPORTED_TARGET smurf_parser)

set(SOURCES 
  src/ComponentModelInterface.cpp
  src/XRockGUI.cpp
  src/ComponentModelEditorWidget.cpp
  src/ImportDialog.cpp
  src/VersionDialog.cpp
  src/ConfigureDialog.cpp
  src/MultiDBConfigDialog.cpp
  src/ConfigMapHelper.cpp
  src/BasicModelHelper.cpp
  src/FileDB.cpp
  src/ToolbarBackend.cpp
  src/plugins/MARSIMUConfig.cpp
  src/plugins/ROCKTASKConfig.cpp
  src/BuildModuleDialog.cpp
)

set(HEADERS
  src/ComponentModelInterface.hpp
  src/XRockGUI.hpp
  src/ComponentModelEditorWidget.hpp
  src/ImportDialog.hpp
  src/VersionDialog.hpp
  src/ConfigureDialog.hpp
  src/ConfigureDialogLoader.hpp
  src/MultiDBConfigDialog.hpp
  src/ConfigMapHelper.hpp
  src/BasicModelHelper.hpp
  src/FileDB.hpp
  src/ToolbarBackend.hpp
  src/DBInterface.hpp
  src/XRockIOLibrary.hpp
  src/BuildModuleDialog.hpp
  src/utils/WaitCursorRAII.hpp
  
)

set (QT_MOC_HEADER
  src/ComponentModelEditorWidget.hpp
  src/ImportDialog.hpp
  src/VersionDialog.hpp
  src/ConfigureDialog.hpp
  src/MultiDBConfigDialog.hpp
  src/ToolbarBackend.hpp
  src/plugins/MARSIMUConfig.hpp
  src/plugins/ROCKTASKConfig.hpp
  src/BuildModuleDialog.hpp
)

if (${USE_QT5})
qt5_wrap_cpp ( QT_MOC_HEADER_SRC ${QT_MOC_HEADER} )
else (${USE_QT5})
qt4_wrap_cpp ( QT_MOC_HEADER_SRC ${QT_MOC_HEADER} )
endif (${USE_QT5})
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${QT_MOC_HEADER_SRC} ${icon_resource})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
  $<INSTALL_INTERFACE:include/xrock_gui_model>
  $<INSTALL_INTERFACE:include/>
)
if (${USE_QT5})
qt5_use_modules(${PROJECT_NAME} Widgets WebKitWidgets)
endif (${USE_QT5})

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-inconsistent-missing-override)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME}
        PkgConfig::lib_manager
        PkgConfig::bagel_gui
        PkgConfig::osg_graph_viz
        PkgConfig::configmaps
        PkgConfig::mars_utils
        PkgConfig::main_gui
        PkgConfig::config_map_gui
        PkgConfig::cfg_manager
        PkgConfig::smurf_parser
        ${QT_LIBRARIES}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17) # Use C++17

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

if(APPLE)
  set(PYTHON_SHEBANG "/opt/local/bin/python")
else(APPLE)
  set(PYTHON_SHEBANG "/usr/bin/env python")
endif(APPLE)

configure_file(bin/xrock_gui.in ${CMAKE_BINARY_DIR}/xrock_gui @ONLY)
configure_file(bin/xbagel_gui.in ${CMAKE_BINARY_DIR}/xbagel_gui @ONLY)
configure_file(bin/shader_gui.in ${CMAKE_BINARY_DIR}/shader_gui @ONLY)
install(DIRECTORY configuration/ DESTINATION configuration/)
INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/xrock_gui DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/xbagel_gui DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/shader_gui DESTINATION bin)
#INSTALL(PROGRAMS bin/cnd_gui DESTINATION bin)
INSTALL(PROGRAMS bin/xrock-resolve-ports DESTINATION bin)
install(DIRECTORY resources/ DESTINATION share/xrock_gui_model/resources/)
configure_file(bin/xrock-create-deployment.in ${CMAKE_BINARY_DIR}/xrock-create-deployment @ONLY)
INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/xrock-create-deployment DESTINATION bin)
