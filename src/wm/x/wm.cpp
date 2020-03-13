#include "wm.hpp"
#include "server.hpp"

#include <unordered_map>

#include <spdlog/spdlog.h>
#include "../window.hpp"
#include "protocols/wl/surface.hpp"

#include "renderers/manager.hpp"
#include "backends/manager.hpp"

#include "wm/output.hpp"
#include "wm/input.hpp"

#include <fmt/format.h>

#include <GLES2/gl2.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <unistd.h>

#include <xcb/composite.h>
#include <xcb/xfixes.h>

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9   /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10   /* move via keyboard */
#define _NET_WM_MOVERESIZE_CANCEL           11   /* cancel operation */

std::unordered_map<xcb_window_t, Awning::Window*> windows;

#include <iostream>

struct MwmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
};
enum {
    MWM_HINTS_FUNCTIONS   = (1L << 0),
    MWM_HINTS_DECORATIONS = (1L << 1),

    MWM_FUNC_ALL      = (1L << 0),
    MWM_FUNC_RESIZE   = (1L << 1),
    MWM_FUNC_MOVE     = (1L << 2),
    MWM_FUNC_MINIMIZE = (1L << 3),
    MWM_FUNC_MAXIMIZE = (1L << 4),
    MWM_FUNC_CLOSE    = (1L << 5)
};

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

namespace Awning::X
{
	wl_client* xWaylandClient = nullptr;
	void* surface;
	EGLDisplay egl_display;

	const char* atom_map[ATOM_LAST] = {
		"WL_SURFACE_ID",
		"WM_DELETE_WINDOW",
		"WM_PROTOCOLS",
		"WM_HINTS",
		"WM_NORMAL_HINTS",
		"WM_SIZE_HINTS",
		"WM_WINDOW_ROLE",
		"_MOTIF_WM_HINTS",
		"UTF8_STRING",
		"WM_S0",
		"_NET_SUPPORTED",
		"_NET_WM_CM_S0",
		"_NET_WM_PID",
		"_NET_WM_NAME",
		"_NET_WM_STATE",
		"_NET_WM_WINDOW_TYPE",
		"WM_TAKE_FOCUS",
		"WINDOW",
		"_NET_ACTIVE_WINDOW",
		"_NET_WM_MOVERESIZE",
		"_NET_WM_NAME",
		"_NET_SUPPORTING_WM_CHECK",
		"_NET_WM_STATE_MODAL",
		"_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_PING",
		"WM_STATE",
		"CLIPBOARD",
		"PRIMARY",
		"_WL_SELECTION",
		"TARGETS",
		"CLIPBOARD_MANAGER",
		"INCR",
		"TEXT",
		"TIMESTAMP",
		"DELETE",
		"_NET_WM_WINDOW_TYPE_NORMAL",
		"_NET_WM_WINDOW_TYPE_UTILITY",
		"_NET_WM_WINDOW_TYPE_TOOLTIP",
		"_NET_WM_WINDOW_TYPE_DND",
		"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
		"_NET_WM_WINDOW_TYPE_POPUP_MENU",
		"_NET_WM_WINDOW_TYPE_COMBO",
		"_NET_WM_WINDOW_TYPE_MENU",
		"_NET_WM_WINDOW_TYPE_NOTIFICATION",
		"_NET_WM_WINDOW_TYPE_SPLASH",
		"XdndSelection",
		"XdndAware",
		"XdndStatus",
		"XdndPosition",
		"XdndEnter",
		"XdndLeave",
		"XdndDrop",
		"XdndFinished",
		"XdndProxy",
		"XdndTypeList",
		"XdndActionMove",
		"XdndActionCopy",
		"XdndActionAsk",
		"XdndActionPrivate",
		"_NET_CLIENT_LIST",
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
			spdlog::error("xcb connect failed: {}", rc);
			close(Server::wm_fd[0]);
			return;
		}

		xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));
		screen = screen_iterator.data;

		//event_source = wl_event_loop_add_fd(Awning::Server::global.event_loop, Server::wm_fd[0], WL_EVENT_READABLE, EventLoop, nullptr);

		int i = 0;
		xcb_intern_atom_cookie_t cookies[ATOM_LAST];
		for (i = 0; i < ATOM_LAST; i++) 
		{
			cookies[i] = xcb_intern_atom(xcb_conn, 0, strlen(atom_map[i]), atom_map[i]);
		}

		for (i = 0; i < ATOM_LAST; i++) 
		{
			xcb_intern_atom_reply_t* reply;
			xcb_generic_error_t* error;

			reply = xcb_intern_atom_reply(xcb_conn, cookies[i], &error);

			if (reply && !error) {
				atoms[i] = reply->atom;
			}
			free(reply);
			if (error)
			{
				spdlog::warn("Could not resolve atom {}, x11 error code {}", atom_map[i], error->error_code);
				free(error);
			}
		}

		uint32_t values[] = {
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_PROPERTY_CHANGE,
		};
		xcb_change_window_attributes_checked(xcb_conn, screen->root, XCB_CW_EVENT_MASK, values);
		xcb_composite_redirect_subwindows_checked(xcb_conn, screen->root, XCB_COMPOSITE_REDIRECT_MANUAL);

		xcb_atom_t supported[] = {
			atoms[NET_WM_STATE],
			atoms[_NET_ACTIVE_WINDOW],
			atoms[_NET_WM_MOVERESIZE],
			atoms[_NET_WM_STATE_MODAL],
			atoms[_NET_WM_STATE_FULLSCREEN],
			atoms[_NET_WM_STATE_MAXIMIZED_VERT],
			atoms[_NET_WM_STATE_MAXIMIZED_HORZ],
			atoms[_NET_CLIENT_LIST],
			atoms[_MOTIF_WM_HINTS],
		};
		xcb_change_property_checked(xcb_conn, XCB_PROP_MODE_REPLACE, screen->root, atoms[NET_SUPPORTED], XCB_ATOM_ATOM, 32, sizeof(supported)/sizeof(*supported), supported);
		xcb_flush(xcb_conn);

		int window_none = XCB_WINDOW_NONE;
		xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, screen->root, atoms[_NET_ACTIVE_WINDOW], atoms[WINDOW], 32, 1, &window_none);

		uint32_t eventMask[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };

		window = xcb_generate_id(xcb_conn);

		xcb_create_window(xcb_conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, 10, 10, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);
		xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, window, atoms[_NET_WM_NAME], atoms[UTF8_STRING], 8, strlen("Awning XWM"), "Awning XWM");
		xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, screen->root, atoms[_NET_SUPPORTING_WM_CHECK], XCB_ATOM_WINDOW, 32, 1, &window);
		xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, window, atoms[_NET_SUPPORTING_WM_CHECK], XCB_ATOM_WINDOW, 32, 1, &window);
		xcb_set_selection_owner(xcb_conn, window, atoms[WM_S0], XCB_CURRENT_TIME);
		xcb_set_selection_owner(xcb_conn, window, atoms[NET_WM_CM_S0], XCB_CURRENT_TIME);
		
		xcb_flush(xcb_conn);
	}

	void Resized(void* data, int width, int height)
	{
		auto w = (::Window)data;
		if (!windows.contains(w))
			return;

		uint32_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;

		uint32_t XPos  = windows[w]->XPos ();
		uint32_t YPos  = windows[w]->YPos ();
		uint32_t XSize = width              ;
		uint32_t YSize = height             ;

		uint32_t values[] = {XPos, YPos, XSize, YSize, 0};
		xcb_configure_window(xcb_conn, w, mask, values);
		xcb_flush(xcb_conn);
	}

	void Moved(void* data, int x, int y)
	{
		auto w = (::Window)data;
		if (!windows.contains(w))
			return;

		uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;

		uint32_t XPos  = x                  ;
		uint32_t YPos  = y                  ;
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

		xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, screen->root, atoms[_NET_ACTIVE_WINDOW], atoms[WINDOW], 32, 1, &w);
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

					spdlog::debug("X Window {} has been created", e->window);
					
					windows[e->window] = Window::Create(xWaylandClient);
					windows[e->window]->Data((void*) e->window          );
					Window::Manager::Manage (windows[e->window]         );
					Client::Bind   ::Surface(windows[e->window], surface);
				}
				break;
			case XCB_DESTROY_NOTIFY:
				{
					auto e = (xcb_destroy_notify_event_t*)event;

					Window::Destory(windows[e->window]);
					windows.erase(e->window);
				}
				break;
			case XCB_CONFIGURE_REQUEST:
				{
					auto e = (xcb_configure_request_event_t*)event;

					spdlog::debug("X Window {} has been configured", e->window);
					
					uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
						XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
						XCB_CONFIG_WINDOW_BORDER_WIDTH;

					uint32_t values[] = {e->x, e->y, e->width, e->height, 0};
					xcb_configure_window(xcb_conn, e->window, mask, values);
					
					Window::Manager::Move  (windows[e->window], e->x    , e->y     );
					Window::Manager::Resize(windows[e->window], e->width, e->height);

				}
				break;
			case XCB_CONFIGURE_NOTIFY:
				{
					auto e = (xcb_configure_notify_event_t*)event;

					spdlog::debug("X Window {} has been configured notify", e->window);

					windows[e->window]->SetResized(nullptr);
					windows[e->window]->SetRaised (nullptr);
					windows[e->window]->SetMoved  (nullptr);

					Window::Manager::Move  (windows[e->window], e->x    , e->y     );
					Window::Manager::Resize(windows[e->window], e->width, e->height);

					windows[e->window]->SetResized(Resized);
					windows[e->window]->SetRaised (Raised );
					windows[e->window]->SetMoved  (Moved  );
				}
				break;
			case XCB_MAP_REQUEST:
			case XCB_MAP_NOTIFY:
				{
					auto e = (xcb_map_request_event_t*)event;

					spdlog::debug("X Window {} has been mapped", e->window);
					
					const uint32_t value_list = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE;
					
					xcb_change_window_attributes_checked(xcb_conn, e->window, XCB_CW_EVENT_MASK, &value_list);
					xcb_map_window_checked              (xcb_conn, e->window);
					xcb_flush                           (xcb_conn);

					windows[e->window]->Frame (true);
        			windows[e->window]->Mapped(true);
					
					Window::Manager::Raise(windows[e->window]);
				}
				break;
			case XCB_UNMAP_NOTIFY:
				{
					auto e = (xcb_unmap_notify_event_t*)event;

        			windows[e->window]->Mapped(false);
				}
				break;
			case XCB_PROPERTY_NOTIFY:
				{
					auto e = (xcb_property_notify_event_t*)event;

					for (int i = 0; i < ATOM_LAST; i++) 
					{
						if (e->atom == atoms[i])
						{
							spdlog::debug("X Window {} has property {}", e->window, atom_map[i]);
						}
					}
				}
				break;
			case XCB_CLIENT_MESSAGE:
				{
					auto e = (xcb_client_message_event_t*)event;

					for (int i = 0; i < ATOM_LAST; i++) 
					{
						if (e->type == atoms[i])
						{
							spdlog::debug("X Window {} has send message {}", e->window, atom_map[i]);
						}
					}

					if (e->type == atoms[WL_SURFACE_ID])
					{
						uint32_t id = e->data.data32[0];
						struct wl_resource* resource = wl_client_get_object(xWaylandClient, id);
						Awning::Protocols::WL::Surface::data.surfaces[resource].window = windows[e->window];
						windows[e->window]->Texture(Awning::Protocols::WL::Surface::data.surfaces[resource].texture);
						Client::Bind::Surface(windows[e->window], resource);
        				windows[e->window]->Mapped(true);

						auto displays = Backend::GetDisplays();
						auto texture = Awning::Protocols::WL::Surface::data.surfaces[resource].texture;
					}
					if (e->type == atoms[_NET_WM_MOVERESIZE])
					{
						Input::Action     action;
						Input::WindowSide side  ;

						switch (e->data.data32[2])
						{
						case _NET_WM_MOVERESIZE_SIZE_TOPLEFT    :
							action = Input::Action::Resize;
							side   = Input::WindowSide::TOP_LEFT;
							break;
						case _NET_WM_MOVERESIZE_SIZE_TOP        :
							action = Input::Action::Resize;
							side   = Input::WindowSide::TOP;
							break;
						case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT   :
							action = Input::Action::Resize;
							side   = Input::WindowSide::TOP_RIGHT;
							break;
						case _NET_WM_MOVERESIZE_SIZE_RIGHT      :
							action = Input::Action::Resize;
							side   = Input::WindowSide::RIGHT;
							break;
						case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
							action = Input::Action::Resize;
							side   = Input::WindowSide::BOTTOM_RIGHT;
							break;
						case _NET_WM_MOVERESIZE_SIZE_BOTTOM     :
							action = Input::Action::Resize;
							side   = Input::WindowSide::BOTTOM;
							break;
						case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT :
							action = Input::Action::Resize;
							side   = Input::WindowSide::BOTTOM_LEFT;
							break;
						case _NET_WM_MOVERESIZE_SIZE_LEFT       :
							action = Input::Action::Resize;
							side   = Input::WindowSide::LEFT;
							break;
						case _NET_WM_MOVERESIZE_MOVE            :
							action = Input::Action::Move;
							side   = Input::WindowSide::TOP;
							break;
						}
						//Input::Lock(action, side);
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