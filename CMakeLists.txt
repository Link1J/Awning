cmake_minimum_required(VERSION 3.10)
project (Awning VERSION 1.0 LANGUAGES CXX C)

#------------------------------------------------------------------------------

set(SOURCES
    src/main.cpp
    src/log.cpp

    src/backends/X11.cpp

    src/wayland/compositor.cpp
    src/wayland/seat.cpp
    src/wayland/output.cpp
    src/wayland/shell_surface.cpp
    src/wayland/shell.cpp
    src/wayland/surface.cpp
    src/wayland/region.cpp

    src/xdg/wm_base.cpp
    src/xdg/surface.cpp
    src/xdg/toplevel.cpp
    src/xdg/popup.cpp
)

set(SOURCE_WAYLAND
	src/protocols/xdg-shell-protocol.c 
	src/protocols/xdg-decoration-protocol.c
)

#------------------------------------------------------------------------------

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
set(OpenGL_GL_PREFERENCE "GLVND")

find_package(X11 REQUIRED)
find_package(Wayland COMPONENTS Client Server Cursor Egl REQUIRED)
find_package(OpenGL REQUIRED)

find_package(WaylandScanner REQUIRED)
find_program(PKG_CONFIG pkg-config)

#------------------------------------------------------------------------------

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/src/protocols/")

execute_process(COMMAND ${PKG_CONFIG} --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE protocol_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${WaylandScanner_EXECUTABLE} server-header ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_SOURCE_DIR}/src/protocols/xdg-shell-protocol.h
				COMMAND ${WaylandScanner_EXECUTABLE} private-code ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_SOURCE_DIR}/src/protocols/xdg-shell-protocol.c)
execute_process(COMMAND ${WaylandScanner_EXECUTABLE} server-header ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml ${CMAKE_SOURCE_DIR}/src/protocols/xdg-decoration-protocol.h
				COMMAND ${WaylandScanner_EXECUTABLE} private-code ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml ${CMAKE_SOURCE_DIR}/src/protocols/xdg-decoration-protocol.c)

#------------------------------------------------------------------------------

add_executable(Awning ${SOURCES} ${SOURCE_WAYLAND})

#------------------------------------------------------------------------------

set_target_properties(Awning PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

#------------------------------------------------------------------------------

target_include_directories(Awning PRIVATE "src")

#------------------------------------------------------------------------------

target_link_libraries(Awning PRIVATE
    Wayland::Server
    X11::X11
    X11::Xext
    OpenGL::GL
)