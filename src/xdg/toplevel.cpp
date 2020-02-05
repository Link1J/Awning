#include "toplevel.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wayland/surface.hpp"
#include "wayland/pointer.hpp"

#include "wm/manager.hpp"

#include <iostream>

namespace Awning::XDG::TopLevel
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
			Log::Function::Called("XDG::TopLevel::Interface");
			TopLevel::Destroy(resource);
		}

		void Set_Parent(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Set_App_id(struct wl_client* client, struct wl_resource* resource, const char* app_id)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Show_Window_Menu(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
			WM::Manager::Handle::Input::Lock(WM::Manager::Handle::Input::MOVE);
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{
			Log::Function::Called("XDG::TopLevel::Interface");

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
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Set_Min_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Set_Maximized(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Unset_Maximized(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Unset_Fullscreen(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}

		void Set_Minimized(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface) 
	{
		Log::Function::Called("XDG::TopLevel");

		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_toplevel_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		auto surface_wl = Surface::data.surfaces[surface].surface_wl;

		data.toplevels[resource] = Data::Instance();
		data.toplevels[resource].surface = surface;
		data.toplevels[resource].window = WM::Window::Create(wl_client);

		Wayland::Surface::data.surfaces[surface_wl].window = data.toplevels[resource].window;
		         Surface::data.surfaces[surface   ].window = data.toplevels[resource].window;

		Awning::XDG::TopLevel::data.toplevels[resource].window->Data      (resource);
		Awning::XDG::TopLevel::data.toplevels[resource].window->SetRaised (Raised  );
		Awning::XDG::TopLevel::data.toplevels[resource].window->SetResized(Resized );
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("XDG::TopLevel");

		if (!data.toplevels.contains(resource))
			return;

		auto surface    =         data.toplevels[resource].surface; 
		auto surface_wl = Surface::data.surfaces[surface ].surface_wl;

			     Surface::data.surfaces[surface   ].window = nullptr;
		Wayland::Surface::data.surfaces[surface_wl].window = nullptr;

		data.toplevels[resource].window->Mapped(false);
		data.toplevels[resource].window->Texture(nullptr);
		WM::Window::Destory(data.toplevels[resource].window);
		data.toplevels.erase(resource);
	}

	void Raised(void* data)
	{
		Log::Function::Called("XDG::TopLevel");
		
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states = new wl_array();
		wl_array_init(states);
		wl_array_add(states, sizeof(xdg_toplevel_state));
		((xdg_toplevel_state*)states->data)[0] = XDG_TOPLEVEL_STATE_ACTIVATED;
		xdg_toplevel_send_configure(resource, 
			Awning::XDG::TopLevel::data.toplevels[resource].window->XSize(),
			Awning::XDG::TopLevel::data.toplevels[resource].window->YSize(),
			states);
		wl_array_release(states);
		delete states;
	}

	void Resized(void* data, int width, int height)
	{
		Log::Function::Called("XDG::TopLevel");
		
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states = new wl_array();
		wl_array_init(states);
		wl_array_add(states, sizeof(xdg_toplevel_state) * 2);
		((xdg_toplevel_state*)states->data)[0] = XDG_TOPLEVEL_STATE_ACTIVATED;
		((xdg_toplevel_state*)states->data)[1] = XDG_TOPLEVEL_STATE_RESIZING;
		xdg_toplevel_send_configure(resource, width, height, states);
		wl_array_release(states);
		delete states;
	}
}