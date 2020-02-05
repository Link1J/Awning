#include "popup.hpp"
#include "log.hpp"

namespace Awning::XDG::Popup
{
	const struct xdg_popup_interface interface = {
		.destroy = Interface::Destroy,
		.grab    = Interface::Grab,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("XDG::Popup::Interface");
		}

		void Grab(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			Log::Function::Called("XDG::Popup::Interface");
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* parent) 
	{
		Log::Function::Called("XDG::Popup");

		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_popup_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.popups[resource] = Data::Instance();
		data.popups[resource].parent = parent;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("XDG::Popup");

		data.popups.erase(resource);
	}
}