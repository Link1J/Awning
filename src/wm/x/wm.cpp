#include "wm.hpp"
#include "server.hpp"

#include <unordered_map>

#include "log.hpp"
#include "../window.hpp"

#include "renderers/manager.hpp"

#include <fmt/format.h>

#include <GLES2/gl2.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <unistd.h>

#include <xcb/composite.h>
#include <xcb/xfixes.h>

std::unordered_map<xcb_window_t, Awning::WM::Window*> windows;

#include <iostream>

namespace Awning
{
	namespace Server
	{
		struct Data
		{
			wl_display* display;
			wl_event_loop* event_loop;
			wl_protocol_logger* logger; 
			wl_listener client_listener;
		};
		extern Data data;
	}
};

namespace Awning::WM::X
{
	wl_client* xWaylandClient = nullptr;
	void* surface;
	EGLDisplay egl_display;

	const char* atom_map[ATOM_LAST] = {
		"WL_SURFACE_ID",
		"WM_DELETE_WINDOW",
		"WM_HINTS",
		"WM_PROTOCOLS",
		"WM_NORMAL_HINTS",
		"WM_SIZE_HINTS",
		"_MOTIF_WM_HINTS",
		"UTF8_STRING",
		"WM_S0",
		"_NET_SUPPORTED",
		"_NET_WM_S0",
		"_NET_WM_PID",
		"_NET_WM_NAME",
		"_NET_WM_STATE",
		"_NET_WM_WINDOW_TYPE",
		"WM_TAKE_FOCUS",
	};

	xcb_atom_t atoms[ATOM_LAST];
	xcb_connection_t* xcb_conn;
	xcb_screen_t* screen;
	xcb_window_t window;
	wl_event_source* event_source;

	void Init()
	{
		xcb_conn = xcb_connect_to_fd(Server::wm_fd[0], NULL);

		int rc = xcb_connection_has_error(xcb_conn);
		if (rc) {
			Log::Report::Error(fmt::format("xcb connect failed: {}", rc));
			close(Server::wm_fd[0]);
			return;
		}

		xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));
		screen = screen_iterator.data;

		event_source = wl_event_loop_add_fd(Awning::Server::data.event_loop, Server::wm_fd[0], WL_EVENT_READABLE, EventLoop, nullptr);

		int i = 0;
		xcb_intern_atom_cookie_t cookies[ATOM_LAST];
		for (i = 0; i < ATOM_LAST; i++) {
			cookies[i] = xcb_intern_atom(xcb_conn, 0, strlen(atom_map[i]), atom_map[i]);
		}
		for (i = 0; i < ATOM_LAST; i++) {
			xcb_intern_atom_reply_t* reply;
			xcb_generic_error_t* error;

			reply = xcb_intern_atom_reply(xcb_conn, cookies[i], &error);

			if (reply && !error) {
				atoms[i] = reply->atom;
			}
			free(reply);
			if (error)
			{
				Log::Report::Error(fmt::format("Could not resolve atom {}, x11 error code {}", atom_map[i], error->error_code));
				free(error);
			}
		}

		window = xcb_generate_id(xcb_conn);

		uint32_t values[] = {
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_PROPERTY_CHANGE,
		};
		xcb_change_window_attributes_checked(xcb_conn, screen->root, XCB_CW_EVENT_MASK, values);
		xcb_composite_redirect_subwindows_checked(xcb_conn, screen->root, XCB_COMPOSITE_REDIRECT_MANUAL);

		uint32_t eventMask[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };

		xcb_create_window_checked(xcb_conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_EVENT_MASK, eventMask);

		xcb_atom_t supported[] = {
			atoms[NET_WM_STATE],
		};
		xcb_change_property_checked(xcb_conn, XCB_PROP_MODE_REPLACE, screen->root, atoms[NET_SUPPORTED], XCB_ATOM_ATOM, 32, sizeof(supported)/sizeof(*supported), supported);

		xcb_set_selection_owner_checked(xcb_conn, window, atoms[WM_S0], XCB_CURRENT_TIME);
		xcb_set_selection_owner_checked(xcb_conn, window, atoms[NET_WM_S0], XCB_CURRENT_TIME);
		xcb_flush(xcb_conn);
	}	

	void UpdateTexture(::Window e)
	{
		return;

		//XWindowAttributes attr;
		//XGetWindowAttributes(display, e, &attr);
//
		//if (!windows[e]->Texture())
		//	windows[e]->Texture(new WM::Texture());
//
		//auto window = windows[e];
		//auto texture = window->Texture();
//
		//XRenderPictFormat* format = XRenderFindVisualFormat(display, attr.visual);
		//bool hasAlpha             = (format->type == PictTypeDirect && format->direct.alphaMask);
		//int x                     = attr.x;
		//int y                     = attr.y;
		//int width                 = attr.width;
		//int height                = attr.height;

		//Manager::Window::Reposition(window, x    , y     );
		//Manager::Window::Resize    (window, width, height);
	}

	void Resized(void* data, int width, int height)
	{
		auto w = (::Window)data;
		if (!windows.contains(w))
			return;

		uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
						XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
						XCB_CONFIG_WINDOW_BORDER_WIDTH;

		uint32_t XPos  = windows[w]->XPos ();
		uint32_t YPos  = windows[w]->YPos ();
		uint32_t XSize = windows[w]->XSize();
		uint32_t YSize = windows[w]->YSize();

		uint32_t values[] = {XPos, YPos, XSize, YSize, 0};
		xcb_configure_window(xcb_conn, w, mask, values);
		xcb_flush(xcb_conn);
	}

	void Raised(void* data)
	{
		auto w = (::Window)data;
		if (!windows.contains(w))
			return;
		if (windows[w]->XSize() == 0 || windows[w]->YSize() == 0)
			return;
	}

#define XCB_EVENT_RESPONSE_TYPE_MASK (0x7f)

	int EventLoop()
	{
		if (!xcb_conn)
			return - 1;

		int count = 0;
		xcb_generic_event_t *event;

		while ((event = xcb_poll_for_event(xcb_conn))) {
			count++;
			switch (event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK) {
			case XCB_CREATE_NOTIFY:
				{
					auto e = (xcb_create_notify_event_t*)event;
					Log::Function::Locate("WM::X", "CreateNotify");
					windows[e->window] = Window::Create(xWaylandClient);
					windows[e->window]->Frame     (true            );
					windows[e->window]->Data      ((void*)e->window);
					windows[e->window]->SetResized(Resized         );
					windows[e->window]->SetRaised (Raised          );
					Client::Bind::Surface(windows[e->window], surface);
				}
				break;
			case XCB_DESTROY_NOTIFY:
				{
					auto e = (xcb_destroy_notify_event_t*)event;
					Log::Function::Locate("WM::X", "DestroyNotify");
					if (windows[e->window]->Texture())
					{
						delete windows[e->window]->Texture();
						windows[e->window]->Texture(nullptr);
					}
					auto window = windows[e->window];
					windows.erase(e->window);
					Window::Destory(window);
				}
				break;
			case XCB_CONFIGURE_REQUEST:
				{
					auto e = (xcb_configure_request_event_t*)event;
					Log::Function::Locate("WM::X", "ConfigureRequest");
  					
					uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
						XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
						XCB_CONFIG_WINDOW_BORDER_WIDTH;

					uint32_t values[] = {e->x, e->y, e->width, e->height, 0};
					xcb_configure_window(xcb_conn, e->window, mask, values);
					xcb_flush(xcb_conn);

					Manager::Window::Reposition(windows[e->window], e->x    , e->y     );
					Manager::Window::Resize    (windows[e->window], e->width, e->height);
				}
				break;
			case XCB_MAP_REQUEST:
				{
					auto e = (xcb_map_request_event_t*)event;
					Log::Function::Locate("WM::X", "MapRequest");

					const uint32_t value_list = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE;
					xcb_change_window_attributes_checked(xcb_conn, e->window, XCB_CW_EVENT_MASK, &value_list);
					xcb_map_window_checked(xcb_conn, e->window);

        			windows[e->window]->Mapped(true);
					WM::Manager::Window::Raise(windows[e->window]);
				}
				break;
			case XCB_MAP_NOTIFY:
				{
					auto e = (xcb_map_notify_event_t*)event;
				}
				break;
			case XCB_UNMAP_NOTIFY:
				{
					auto e = (xcb_unmap_notify_event_t*)event;
				}
				break;
			case XCB_PROPERTY_NOTIFY:
				{
					auto e = (xcb_property_notify_event_t*)event;
				}
				break;
			case XCB_CLIENT_MESSAGE:
				{
					auto e = (xcb_client_message_event_t*)event;
					Log::Function::Locate("WM::X", "ClientMessage");
					if (e->type == atoms[WL_SURFACE_ID])
					{
						Log::Function::Locate("WM::X", "ClientMessage.WL_SURFACE_ID");
					}
				}
				break;
			}
			free(event);
		}

		xcb_flush(xcb_conn);
		return count;
	}

	int EventLoop(int fd, uint32_t mask, void *data)
	{
		return EventLoop();
	}
}