#include "popup.hpp"
#include <spdlog/spdlog.h>

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
	std::unordered_map<wl_resource*, Instance> instances;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Popup::Destroy(resource);
		}

		void Grab(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* parent, wl_resource* point) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_popup_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		auto surface_wl = Surface::instances[surface].surface_wl;

		instances[resource].parent  = parent;
		instances[resource].surface = surface;
		instances[resource].window  = Window::Create(wl_client);

		Window::Manager::Manage(instances[resource].window);

		WL::Surface::instances[surface_wl].window = instances[resource].window;
		    Surface::instances[surface   ].window = instances[resource].window;

		auto pointer = Positioner::data.instances[point];

		WL::Surface::instances[surface_wl].type = 2;

		Window::Manager::Move  (instances[resource].window, pointer.x, pointer.y);
		Window::Manager::Resize(instances[resource].window, pointer.width, pointer.height);

		instances[resource].window->Data(resource);
		instances[resource].window->Parent(Surface::instances[parent].window, true);
		Surface::instances[resource].configured = true;

		xdg_popup_send_configure(resource, pointer.x, pointer.y, pointer.width, pointer.height);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		auto surface    =          instances[resource  ].surface   ; 
		auto surface_wl = Surface::instances[surface ].surface_wl;

			Surface::instances[surface   ].window = nullptr;
		WL::Surface::instances[surface_wl].window = nullptr;

		instances[resource].window->Mapped (false  );
		instances[resource].window->Texture(nullptr);
		Window::Destory(instances[resource].window);
		instances.erase(resource);
	}
}