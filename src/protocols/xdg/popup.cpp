#include "popup.hpp"
#include "log.hpp"

#include "surface.hpp"

#include "protocols/wl/surface.hpp"
#include "protocols/xdg/positioner.hpp"
#include "protocols/xdg/surface.hpp"

namespace Awning::Protocols::XDG::Popup
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
			Log::Function::Called("Protocols::XDG::Popup::Interface");
			Popup::Destroy(resource);
		}

		void Grab(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			Log::Function::Called("Protocols::XDG::Popup::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* parent, wl_resource* point) 
	{
		Log::Function::Called("Protocols::XDG::Popup");

		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_popup_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		auto surface_wl = Surface::data.surfaces[surface].surface_wl;

		data.popups[resource].parent  = parent;
		data.popups[resource].surface = surface;
		data.popups[resource].window  = WM::Window::Create(wl_client);

		WL::Surface::data.surfaces[surface_wl].window = data.popups[resource].window;
		    Surface::data.surfaces[surface   ].window = data.popups[resource].window;

		auto pointer = Positioner::data.instances[point];

		WL::Surface::data.surfaces[surface_wl].type = 2;

		data.popups[resource].window->Data(resource);
		data.popups[resource].window->ConfigPos(pointer.x, pointer.y);
		data.popups[resource].window->ConfigSize(pointer.width, pointer.height);
		data.popups[resource].window->Parent(Surface::data.surfaces[parent].window, true);
		Surface::data.surfaces[resource].configured = true;

		xdg_popup_send_configure(resource, pointer.x, pointer.y, pointer.width, pointer.height);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::XDG::Popup");

		if (!data.popups.contains(resource))
			return;

		auto surface    =          data.popups[resource  ].surface   ; 
		auto surface_wl = Surface::data.surfaces[surface ].surface_wl;

			Surface::data.surfaces[surface   ].window = nullptr;
		WL::Surface::data.surfaces[surface_wl].window = nullptr;

		data.popups[resource].window->Mapped (false  );
		data.popups[resource].window->Texture(nullptr);
		WM::Window::Destory(data.popups[resource].window);
		data.popups.erase(resource);
	}
}