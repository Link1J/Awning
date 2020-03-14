#include "toplevel.hpp"
#include "surface.hpp"
#include <spdlog/spdlog.h>

#include "protocols/wl/surface.hpp"
#include "protocols/wl/pointer.hpp"
#include "protocols/wl/seat.hpp"

#include "wm/input.hpp"

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
	std::unordered_map<wl_resource*, Instance> instances;

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
			((Input::Seat*)WL::Seat::instances[seat].seat)->Lock(Input::Action::Move, Input::WindowSide::TOP);
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{

			Input::WindowSide side;
			switch (edges)
			{
			case XDG_TOPLEVEL_RESIZE_EDGE_TOP         : side = Input::WindowSide::TOP         ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM      : side = Input::WindowSide::BOTTOM      ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_LEFT        : side = Input::WindowSide::LEFT        ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT    : side = Input::WindowSide::TOP_LEFT    ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT : side = Input::WindowSide::BOTTOM_LEFT ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_RIGHT       : side = Input::WindowSide::RIGHT       ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT   : side = Input::WindowSide::TOP_RIGHT   ; break;
			case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT: side = Input::WindowSide::BOTTOM_RIGHT; break;
			}

			((Input::Seat*)WL::Seat::instances[seat].seat)->Lock(Input::Action::Resize, side);
		}

		void Set_Max_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height)
		{
			instances[resource].window->ConfigMaxSize(width, height);
		}

		void Set_Min_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height)
		{
			instances[resource].window->ConfigMinSize(width, height);
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

		auto surface_wl = Surface::instances[surface].surface_wl;

		instances[resource] = Instance();
		instances[resource].surface = surface;
		instances[resource].window  = Window::Create(wl_client);

		Window::Manager::Manage(instances[resource].window);

		WL::Surface::instances[surface_wl].window = instances[resource].window;
		    Surface::instances[surface   ].window = instances[resource].window;

		WL::Surface::instances[surface_wl].type = 1;

		instances[resource].window->Data      (resource);
		instances[resource].window->SetRaised (Raised  );
		instances[resource].window->SetResized(Resized );
		instances[resource].window->SetLowered(Lowered );

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		auto surface    =         instances[resource].surface   ; 
		auto surface_wl = Surface::instances[surface ].surface_wl;

			Surface::instances[surface   ].window = nullptr;
		WL::Surface::instances[surface_wl].window = nullptr;

		instances[resource].window->Mapped(false);
		instances[resource].window->Texture(nullptr);
		Window::Destory(instances[resource].window);
		instances.erase(resource);
	}

	void Raised(void* data)
	{
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states = new wl_array();
		wl_array_init(states);
		wl_array_add(states, sizeof(xdg_toplevel_state));
		((xdg_toplevel_state*)states->data)[0] = XDG_TOPLEVEL_STATE_ACTIVATED;
		xdg_toplevel_send_configure(resource, 
			Awning::Protocols::XDG::TopLevel::instances[resource].window->XSize(),
			Awning::Protocols::XDG::TopLevel::instances[resource].window->YSize(),
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
		xdg_surface_send_configure(TopLevel::instances[resource].surface, NextSerialNum());
	}

	void Lowered(void* data)
	{
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states = new wl_array();
		wl_array_init(states);
		xdg_toplevel_send_configure(resource, 
			Awning::Protocols::XDG::TopLevel::instances[resource].window->XSize(),
			Awning::Protocols::XDG::TopLevel::instances[resource].window->YSize(),
			states);
		wl_array_release(states);
		delete states;
	}
}