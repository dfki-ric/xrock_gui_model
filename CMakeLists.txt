cmake_minimum_required(VERSION 3.10)
project(xrock_gui_model VERSION 1.0.0 DESCRIPTION "xrock_gui")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(lib_manager)
pkg_check_modules(PKGCONFIG REQUIRED
          lib_manager)
lib_defaults()
define_module_info()
if(APPLE)
add_definitions(-std=c++17)
add_definitions(-DGL_SILENCE_DEPRECATION)
endif(APPLE)


set(FILE_DB 1)

set(QT_USE_QTWEBKIT 1)
setup_qt()
#pkg_check_modules(lib_manager REQUIRED IMPORTED_TARGET lib_manager)
pkg_check_modules(bagel_gui REQUIRED IMPORTED_TARGET bagel_gui)
pkg_check_modules(osg_graph_viz REQUIRED IMPORTED_TARGET osg_graph_viz)
pkg_check_modules(configmaps REQUIRED IMPORTED_TARGET configmaps)
pkg_check_modules(mars_utils REQUIRED IMPORTED_TARGET mars_utils)
pkg_check_modules(main_gui REQUIRED IMPORTED_TARGET main_gui)
pkg_check_modules(config_map_gui REQUIRED IMPORTED_TARGET config_map_gui)
pkg_check_modules(cfg_manager REQUIRED IMPORTED_TARGET cfg_manager)


find_package(xdbi REQUIRED)
find_package(modkom_types REQUIRED)
find_package( Boost COMPONENTS system thread REQUIRED)

set(SOURCES 
  src/Model.cpp
  src/ModelLib.cpp
  src/ModelWidget.cpp
  src/ImportDialog.cpp
  src/VersionDialog.cpp
  src/ConfigureDialog.cpp
  src/ConfigMapHelper.cpp
  src/FileDB.cpp
  src/RestDB.cpp
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
  src/RestDB.hpp
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

target_link_libraries(${PROJECT_NAME}  PUBLIC
	lib_manager
        PkgConfig::bagel_gui
        PkgConfig::osg_graph_viz
        PkgConfig::configmaps
        PkgConfig::mars_utils
        PkgConfig::main_gui
        PkgConfig::config_map_gui
        PkgConfig::cfg_manager
        ${QT_LIBRARIES}
        modkom_types::modkom_types_cpp
        xtypes_generator::xtypes_generator_cpp
        xdbi::xdbi_cpp
        ${Boost_SYSTEM_LIBRARY} 	
)

add_executable(test_restdb test/test_restdb.cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17) # Use C++17
target_link_libraries(test_restdb ${PROJECT_NAME})

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
INSTALL(PROGRAMS bin/xrock-resolve-ports DESTINATION bin)
INSTALL(PROGRAMS bin/xrock-create-deployment DESTINATION bin)
