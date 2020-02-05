#include "surface.hpp"
#include "toplevel.hpp"
#include "popup.hpp"
#include "log.hpp"

#include "wayland/surface.hpp"
#include "wm/manager.hpp"

#include <unordered_map>

uint32_t NextSerialNum();

namespace Awning::XDG::Surface
{
	const struct xdg_surface_interface interface = {
		.destroy             = Interface::Destroy,
		.get_toplevel        = Interface::Get_Toplevel,
		.get_popup           = Interface::Get_Popup,
		.set_window_geometry = Interface::Set_Window_Geometry,
		.ack_configure       = Interface::Ack_Configure,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::Surface::Interface");
			Surface::Destroy(resource);
		}

		void Get_Toplevel(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("XDG::Surface::Interface");
			TopLevel::Create(client, 1, id, resource);
		}

		void Get_Popup(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* parent, struct wl_resource* positioner)
		{
			Log::Function::Called("XDG::Surface::Interface");
			Popup::Create(client, 1, id, parent);
		}

		void Set_Window_Geometry(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			Log::Function::Called("XDG::Surface::Interface");
			
			if (!data.surfaces.contains(resource))
				return;
			if (!data.surfaces[resource].window)
				return;

			data.surfaces[resource].window->ConfigPos(x, y, true);
			data.surfaces[resource].window->ConfigSize(width, height);
		}

		void Ack_Configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
			Log::Function::Called("XDG::Surface::Interface");
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface) 
	{
		Log::Function::Called("XDG::Surface");

		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.surfaces[resource].surface_wl = surface;
		
		Wayland::Surface::data.surfaces[surface].type = 1;
		Wayland::Surface::data.surfaces[surface].shell = resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("XDG::Surface");

		data.surfaces.erase(resource);
	}
}