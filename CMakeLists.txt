cmake_minimum_required(VERSION 3.10)
project (Awning VERSION 1.0 LANGUAGES CXX C)

#------------------------------------------------------------------------------

set(SOURCES
    src/main.cpp
    src/log.cpp

    src/backends/X11.cpp
    src/backends/manager.cpp
    src/backends/evdev.cpp
    src/backends/libinput.cpp
    src/backends/drm.cpp

    src/protocols/wl/compositor.cpp
    src/protocols/wl/seat.cpp
    src/protocols/wl/output.cpp
    src/protocols/wl/shell_surface.cpp
    src/protocols/wl/shell.cpp
    src/protocols/wl/surface.cpp
    src/protocols/wl/region.cpp
    src/protocols/wl/pointer.cpp
    src/protocols/wl/keyboard.cpp
    src/protocols/wl/data_device_manager.cpp
    src/protocols/wl/data_source.cpp
    src/protocols/wl/data_device.cpp

    src/protocols/xdg/wm_base.cpp
    src/protocols/xdg/surface.cpp
    src/protocols/xdg/toplevel.cpp
    src/protocols/xdg/popup.cpp
    src/protocols/xdg/positioner.cpp

    src/protocols/zxdg/decoration.cpp

    src/wm/manager.cpp
    src/wm/window.cpp
    src/wm/client.cpp
    src/wm/output.cpp

    src/wm/x/wm.cpp

    src/renderers/software.cpp
    src/renderers/gles2.cpp
    src/renderers/manager.cpp

    src/protocols/awn/config.cpp
    src/protocols/awn/config_outpur.cpp
)

set(SOURCE_WAYLAND
	src/protocols/handler/xdg-shell-protocol.c 
    src/protocols/handler/xdg-decoration-protocol.c
    src/protocols/handler/linux-dmabuf-protocol.c
    src/protocols/handler/awn-config.c
	src/protocols/handler/xdg-shell-protocol.h 
    src/protocols/handler/xdg-decoration-protocol.h
    src/protocols/handler/linux-dmabuf-protocol.h
    src/protocols/handler/awn-config.h
)

#------------------------------------------------------------------------------

add_subdirectory(external/fmt-6.1.2)

#------------------------------------------------------------------------------

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
set(OpenGL_GL_PREFERENCE "GLVND")

find_package(X11 REQUIRED)
find_package(Wayland COMPONENTS Client Server Cursor Egl REQUIRED)
find_package(UDev REQUIRED)
find_package(Libinput REQUIRED)
find_package(OpenGL REQUIRED)
#find_package(fmt REQUIRED)
find_package(EGL REQUIRED)

find_package(WaylandScanner REQUIRED)
find_program(PKG_CONFIG pkg-config)

#------------------------------------------------------------------------------

execute_process(COMMAND ${PKG_CONFIG} --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE protocol_dir OUTPUT_STRIP_TRAILING_WHITESPACE)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/src/protocols/handler")

add_custom_command(
    OUTPUT  ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-shell-protocol.h
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-shell-protocol.c

    DEPENDS ${protocol_dir}/stable/xdg-shell/xdg-shell.xml

    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${protocol_dir}/stable/xdg-shell/xdg-shell.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-shell-protocol.h
            
    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${protocol_dir}/stable/xdg-shell/xdg-shell.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-shell-protocol.c
)
add_custom_command(
    OUTPUT  ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-decoration-protocol.h
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-decoration-protocol.c

    DEPENDS ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml

    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-decoration-protocol.h

    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-decoration-protocol.c
)
add_custom_command(
    OUTPUT  ${CMAKE_SOURCE_DIR}/src/protocols/handler/linux-dmabuf-protocol.h
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/linux-dmabuf-protocol.c

    DEPENDS ${protocol_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml

    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${protocol_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/linux-dmabuf-protocol.h
    
    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${protocol_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/linux-dmabuf-protocol.c
)
add_custom_command(
    OUTPUT  ${CMAKE_SOURCE_DIR}/src/protocols/handler/awn-config.h
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/awn-config.c

    DEPENDS ${CMAKE_SOURCE_DIR}/protocols/awn-config.xml

    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${CMAKE_SOURCE_DIR}/protocols/awn-config.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/awn-config.h

    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${CMAKE_SOURCE_DIR}/protocols/awn-config.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/awn-config.c
)

execute_process(
    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${protocol_dir}/stable/xdg-shell/xdg-shell.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-shell-protocol.h
            
    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${protocol_dir}/stable/xdg-shell/xdg-shell.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-shell-protocol.c
)
execute_process(
    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-decoration-protocol.h

    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/xdg-decoration-protocol.c
)
execute_process(
    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${protocol_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/linux-dmabuf-protocol.h
    
    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${protocol_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/linux-dmabuf-protocol.c
)
execute_process(
    COMMAND ${WaylandScanner_EXECUTABLE} server-header
            ${CMAKE_SOURCE_DIR}/protocols/awn-config.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/awn-config.h

    COMMAND ${WaylandScanner_EXECUTABLE} private-code
            ${CMAKE_SOURCE_DIR}/protocols/awn-config.xml
            ${CMAKE_SOURCE_DIR}/src/protocols/handler/awn-config.c
)

#------------------------------------------------------------------------------

add_executable   (Awning                    ${SOURCES}       )
add_custom_target(Wayland-Protocols DEPENDS ${SOURCE_WAYLAND})

add_dependencies(Awning         Wayland-Protocols)
target_sources  (Awning PRIVATE ${SOURCE_WAYLAND})

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
    X11::Xfixes
    X11::Xcomposite
    X11::Xrender
    OpenGL::GL
    Libinput::Libinput
    UDev::UDev
    fmt::fmt
    EGL::EGL

    gbm
    GLESv2
    xkbcommon
    rt
)