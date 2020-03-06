cmake_minimum_required(VERSION 3.10)
project (Awning VERSION 1.0 LANGUAGES CXX C)

#------------------------------------------------------------------------------

set(SOURCES
    src/main.cpp

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
    src/protocols/wl/subcompositor.cpp
    src/protocols/wl/subsurface.cpp

    src/protocols/xdg/wm_base.cpp
    src/protocols/xdg/surface.cpp
    src/protocols/xdg/toplevel.cpp
    src/protocols/xdg/popup.cpp
    src/protocols/xdg/positioner.cpp

    src/protocols/zxdg/decoration.cpp
    src/protocols/zxdg/output.cpp

    src/wm/manager.cpp
    src/wm/window.cpp
    src/wm/client.cpp
    src/wm/output.cpp

    src/wm/x/wm.cpp
    src/wm/x/server.cpp

    src/renderers/software.cpp
    src/renderers/gles2.cpp
    src/renderers/manager.cpp
    src/renderers/egl.cpp

    src/protocols/awn/config.cpp
	src/protocols/awn/config_output.cpp
	
    src/protocols/wlr/output_manager.cpp
    src/protocols/wlr/layer_shell.cpp
    src/protocols/wlr/layer_surface.cpp
    
    src/protocols/zwp/dmabuf.cpp

    src/protocols/kde/decoration.cpp
)

set(SOURCE_WAYLAND
	src/protocols/handler/xdg-shell.c 
    src/protocols/handler/xdg-decoration.c
    src/protocols/handler/linux-dmabuf.c
	src/protocols/handler/awn-config.c
	src/protocols/handler/wlr-output-management.c
    src/protocols/handler/xdg-output.c
    src/protocols/handler/server_decoration.c
    src/protocols/handler/wlr-layer-shell.c

    src/protocols/handler/xdg-shell.h 
    src/protocols/handler/xdg-decoration.h
    src/protocols/handler/linux-dmabuf.h
	src/protocols/handler/awn-config.h
	src/protocols/handler/wlr-output-management.h
    src/protocols/handler/xdg-output.h
    src/protocols/handler/server_decoration.h
    src/protocols/handler/wlr-layer-shell.h
)

#------------------------------------------------------------------------------

add_subdirectory(external/fmt)
add_subdirectory(external/spdlog)
add_subdirectory(external/Lyra)

#------------------------------------------------------------------------------

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
set(OpenGL_GL_PREFERENCE "GLVND")

find_package(X11 REQUIRED)
find_package(Wayland COMPONENTS Client Server Cursor Egl REQUIRED)
find_package(UDev REQUIRED)
find_package(Libinput REQUIRED)
find_package(OpenGL REQUIRED)
find_package(EGL REQUIRED)
find_package(XCB COMPONENTS XCB ATOM COMPOSITE DRI2 DRI3 EWMH ICCCM XFIXES XKB RENDER SHAPE REQUIRED)

find_package(WaylandScanner REQUIRED)
find_program(PKG_CONFIG pkg-config)

#------------------------------------------------------------------------------

function(wayland_protocol_create file_name file_to_use)
	#message(${file_name})
	#message(${file_to_use})

	add_custom_command(
	    OUTPUT  ${CMAKE_SOURCE_DIR}/src/protocols/handler/${file_name}.h
	            ${CMAKE_SOURCE_DIR}/src/protocols/handler/${file_name}.c

		DEPENDS ${file_to_use}
		
	    COMMAND ${WaylandScanner_EXECUTABLE} server-header
	            ${file_to_use} ${CMAKE_SOURCE_DIR}/src/protocols/handler/${file_name}.h
	
	    COMMAND ${WaylandScanner_EXECUTABLE} private-code
	            ${file_to_use} ${CMAKE_SOURCE_DIR}/src/protocols/handler/${file_name}.c
	)
	execute_process(
	    COMMAND ${WaylandScanner_EXECUTABLE} server-header
	            ${file_to_use} ${CMAKE_SOURCE_DIR}/src/protocols/handler/${file_name}.h
	
	    COMMAND ${WaylandScanner_EXECUTABLE} private-code
	            ${file_to_use} ${CMAKE_SOURCE_DIR}/src/protocols/handler/${file_name}.c
	)
endfunction(wayland_protocol_create)

#------------------------------------------------------------------------------

execute_process(COMMAND ${PKG_CONFIG} --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE protocol_dir OUTPUT_STRIP_TRAILING_WHITESPACE)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/src/protocols/handler")

wayland_protocol_create(xdg-shell ${protocol_dir}/stable/xdg-shell/xdg-shell.xml)
wayland_protocol_create(xdg-decoration ${protocol_dir}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml)
wayland_protocol_create(linux-dmabuf ${protocol_dir}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml)
wayland_protocol_create(awn-config ${CMAKE_SOURCE_DIR}/protocols/awn-config.xml)
wayland_protocol_create(wlr-output-management ${CMAKE_SOURCE_DIR}/protocols/wlr-output-management-unstable-v1.xml)
wayland_protocol_create(xdg-output ${protocol_dir}/unstable/xdg-output/xdg-output-unstable-v1.xml)
wayland_protocol_create(server_decoration ${CMAKE_SOURCE_DIR}/protocols/server_decoration.xml)
wayland_protocol_create(wlr-layer-shell ${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml)

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
    Wayland::Client
    XCB::XCB
    XCB::ATOM
    XCB::COMPOSITE
    XCB::DRI2
    XCB::DRI3
    XCB::EWMH
    XCB::ICCCM
    XCB::XFIXES
    XCB::XKB
    XCB::RENDER
    XCB::SHAPE
    spdlog::spdlog

    gbm
    GLESv2
    xkbcommon
    rt
)
