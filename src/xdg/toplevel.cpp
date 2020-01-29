#include "toplevel.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wayland/surface.hpp"
#include "wayland/seat.hpp"

#include "wm/drawable.hpp"

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

			auto& pointer = Wayland::Pointer::data.pointers[client];
			auto& surface = Surface::data.surfaces[data.toplevels[resource].surface];
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

		data.toplevels[resource] = Data::Instance();
		data.toplevels[resource].surface = surface;

		Surface::data.surfaces[resource].xPosition = 200;
		Surface::data.surfaces[resource].yPosition = 200;

		auto surface_wl = Surface::data.surfaces[surface].surface_wl;

		WM::Drawable::drawables[resource].xPosition  = &         Surface::data.surfaces[resource  ].xPosition ;
		WM::Drawable::drawables[resource].yPosition  = &         Surface::data.surfaces[resource  ].yPosition ;
		WM::Drawable::drawables[resource].xDimension = &         Surface::data.surfaces[resource  ].xDimension;
		WM::Drawable::drawables[resource].yDimension = &         Surface::data.surfaces[resource  ].yDimension;
		WM::Drawable::drawables[resource].data       = &Wayland::Surface::data.surfaces[surface_wl].data      ;
		WM::Drawable::drawables[resource].surface    =                                              surface_wl;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("XDG::TopLevel");

		WM::Drawable::drawables.erase(resource);
		data.toplevels.erase(resource);
	}
}