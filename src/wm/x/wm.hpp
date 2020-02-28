#pragma once
#include <wayland-server.h>
#include <EGL/egl.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

namespace Awning::WM::X
{
	enum atom_name {
		WL_SURFACE_ID,
		WM_DELETE_WINDOW,
		WM_PROTOCOLS,
		WM_HINTS,
		WM_NORMAL_HINTS,
		WM_SIZE_HINTS,
		MOTIF_WM_HINTS,
		UTF8_STRING,
		WM_S0,
		NET_SUPPORTED,
		NET_WM_S0,
		NET_WM_PID,
		NET_WM_NAME,
		NET_WM_STATE,
		NET_WM_WINDOW_TYPE,
		WM_TAKE_FOCUS,
		ATOM_LAST,
	};

	extern EGLDisplay egl_display;
	extern wl_client* xWaylandClient;
	extern void* surface;

	void Init();
	int EventLoop(int fd, uint32_t mask, void *data);
	int EventLoop();
}