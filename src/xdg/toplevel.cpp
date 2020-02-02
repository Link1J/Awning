#include "toplevel.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wayland/surface.hpp"
#include "wayland/pointer.hpp"

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

			auto& topleve = data.toplevels[resource];
			auto& surface = XDG::Surface::data.surfaces[topleve.surface];
			auto& pointer = Wayland::Pointer::data.pointers[client];

			Wayland::Pointer::MoveMode();
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{
			Log::Function::Called("XDG::TopLevel::Interface");
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
		data.toplevels[resource].window = WM::Window::Create();

		Wayland::Surface::data.surfaces[surface_wl].window = data.toplevels[resource].window;
		         Surface::data.surfaces[surface   ].window = data.toplevels[resource].window;

		Awning::XDG::TopLevel::data.toplevels[resource].window->Data     (resource);
		Awning::XDG::TopLevel::data.toplevels[resource].window->SetRaised(Raised  );
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("XDG::TopLevel");

		auto surface    =         data.toplevels[resource].surface; 
		auto surface_wl = Surface::data.surfaces[surface ].surface_wl;

			     Surface::data.surfaces[surface   ].window = nullptr;
		Wayland::Surface::data.surfaces[surface_wl].window = nullptr;

		WM::Window::Destory(data.toplevels[resource].window);
		data.toplevels.erase(resource);
	}

	void Raised(void* data)
	{
		Log::Function::Called("XDG::TopLevel");
		
		struct wl_resource* resource = (struct wl_resource*)data;
		wl_array* states;
		wl_array_init(states);
		wl_array_add(states, XDG_TOPLEVEL_STATE_ACTIVATED);
		xdg_toplevel_send_configure(resource, 
			Awning::XDG::TopLevel::data.toplevels[resource].window->XSize(),
			Awning::XDG::TopLevel::data.toplevels[resource].window->YSize(),
			states);
		//wl_array_release(states);
	}
}