#include "toplevel.hpp"
#include "surface.hpp"
#include <spdlog/spdlog.h>

#include "protocols/wl/surface.hpp"
#include "protocols/wl/pointer.hpp"

#include "wm/manager.hpp"

#include <iostream>

uint32_t NextSerialNum();

namespace Awning::Protocols::XDG::TopLevel
{
	const struct xdg_toplevel_interface interface = {
		.destroy          = Interface::Destroy,
		.set_parent       = Interface::Set_Parent,
		.set_title        = Interface::Set_Title,
		.set_app_id       = Interface::Set_App_id,
		.show_window_menu = Interface::Show_Window_Menu,
		.move             = Interface::Move,
		.resize           = Interface::Resize,
		.set_max_size     = Interface::Set_Max_Size,
		.set_min_size     = Interface::Set_Min_Size,
		.set_maximized    = Interface::Set_Maximized,
		.unset_maximized  = Interface::Unset_Maximized,
		.set_fullscreen   = Interface::Set_Fullscreen,
		.unset_fullscreen = Interface::Unset_Fullscreen,
		.set_minimized    = Interface::Set_Minimized,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			TopLevel::Destroy(resource);
		}

		void Set_Parent(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent)
		{
		}

		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title)
		{
		}

		void Set_App_id(struct wl_client* client, struct wl_resource* resource, const char* app_id)
		{
		}

		void Show_Window_Menu(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
		{
		}

		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			WM::Manager::Handle::Input::Lock(WM::Manager::Handle::Input::MOVE);
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{

			WM::Manager::Handle::Input::WindowSide side;
			switch (edges)
			{
			case XDG_TOPLEVEL_RESIZE_EDGE_TOP         : side = WM::Manager::Handle::Input::TOP         ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM      : side = WM::Manager::Handle::Input::BOTTOM      ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_LEFT        : side = WM::Manager::Handle::Input::LEFT        ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT    : side = WM::Manager::Handle::Input::TOP_LEFT    ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT : side = WM::Manager::Handle::Input::BOTTOM_LEFT ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_RIGHT       : side = WM::Manager::Handle::Input::RIGHT       ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT   : side = WM::Manager::Handle::Input::TOP_RIGHT   ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT: side = WM::Manager::Handle::Input::BOTTOM_RIGHT; break;
			}

			WM::Manager::Handle::Input::Lock(WM::Manager::Handle::Input::RESIZE, side);
		}

		void Set_Max_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height)
		{
			data.toplevels[resource].window->ConfigMaxSize(width, height);
		}

		void Set_Min_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height)
		{
			data.toplevels[resource].window->ConfigMinSize(width, height);
		}

		void Set_Maximized(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Unset_Maximized(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output)
		{
		}

		void Unset_Fullscreen(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Set_Minimized(struct wl_client* client, struct wl_resource* resource)
		{
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_toplevel_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		auto surface_wl = Surface::data.surfaces[surface].surface_wl;

		data.toplevels[resource] = Data::Instance();
		data.toplevels[resource].surface = surface;
		data.toplevels[resource].window = Window::Create(wl_client);

		Window::Manager::Manage(data.toplevels[resource].window);

		WL::Surface::data.surfaces[surface_wl].window = data.toplevels[resource].window;
		    Surface::data.surfaces[surface   ].window = data.toplevels[resource].window;

		WL::Surface::data.surfaces[surface_wl].type = 1;

		data.toplevels[resource].window->Data      (resource);
		data.toplevels[resource].window->SetRaised (Raised  );
		data.toplevels[resource].window->SetResized(Resized );

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.toplevels.contains(resource))
			return;

		auto surface    =         data.toplevels[resource].surface   ; 
		auto surface_wl = Surface::data.surfaces[surface ].surface_wl;

			Surface::data.surfaces[surface   ].window = nullptr;
		WL::Surface::data.surfaces[surface_wl].window = nullptr;

		data.toplevels[resource].window->Mapped(false);
		data.toplevels[resource].window->Texture(nullptr);
		Window::Destory(data.toplevels[resource].window);
		data.toplevels.erase(resource);
	}

	void Raised(void* data)
	{
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states = new wl_array();
		wl_array_init(states);
		wl_array_add(states, sizeof(xdg_toplevel_state));
		((xdg_toplevel_state*)states->data)[0] = XDG_TOPLEVEL_STATE_ACTIVATED;
		xdg_toplevel_send_configure(resource, 
			Awning::Protocols::XDG::TopLevel::data.toplevels[resource].window->XSize(),
			Awning::Protocols::XDG::TopLevel::data.toplevels[resource].window->YSize(),
			states);
		wl_array_release(states);
		delete states;
	}

	void Resized(void* data, int width, int height)
	{
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states = new wl_array();
		wl_array_init(states);
		wl_array_add(states, sizeof(xdg_toplevel_state) * 2);
		((xdg_toplevel_state*)states->data)[0] = XDG_TOPLEVEL_STATE_RESIZING ;
		((xdg_toplevel_state*)states->data)[1] = XDG_TOPLEVEL_STATE_ACTIVATED;
		xdg_toplevel_send_configure(resource, width, height, states);
		wl_array_release(states);
		delete states;
		xdg_surface_send_configure(TopLevel::data.toplevels[resource].surface, NextSerialNum());
	}
}