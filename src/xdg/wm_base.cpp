#include "wm_base.hpp"
#include "surface.hpp"
#include "log.hpp"

namespace Awning::XDG::WM_Base
{
	const struct xdg_wm_base_interface interface = {
		.destroy           = Interface::Destroy,
		.create_positioner = Interface::Create_Positioner,
		.get_xdg_surface   = Interface::Get_XDG_Surface,
		.pong              = Interface::Pong,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::WM_Base");
		}

		void Create_Positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("XDG::WM_Base");
		}

		void Get_XDG_Surface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface)
		{
			Log::Function::Called("XDG::WM_Base");
			Surface::Create(client, 1, id, surface);
		}

		void Pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
			Log::Function::Called("XDG::WM_Base");
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("XDG::WM_Base");

		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_wm_base_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}
}