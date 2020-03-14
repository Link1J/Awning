#include "subsurface.hpp"
#include <spdlog/spdlog.h>
#include "renderers/manager.hpp"
#include "wm/window.hpp"
#include "surface.hpp"

#include <cstring>

#include <unordered_set>
#include <chrono>
#include <vector>
#include <iostream>

uint32_t NextSerialNum();

namespace Awning::Protocols::WL::Subsurface
{
	const struct wl_subsurface_interface interface = {
		.destroy      = Interface::Destroy,
		.set_position = Interface::Set_Position,
		.place_above  = Interface::Place_Above,
		.place_below  = Interface::Place_Below,
		.set_sync     = Interface::Set_Sync,
		.set_desync   = Interface::Set_Desync,
	};
	std::unordered_map<wl_resource*, Instance> instances;
	std::vector<wl_resource*> list;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Subsurface::Destroy(resource);
		}

		void Set_Position(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y)
		{
			Window::Manager::Move(instances[resource].window, x, y);
		}

		void Place_Above(struct wl_client* client, struct wl_resource* resource, struct wl_resource* sibling)
		{
		}

		void Place_Below(struct wl_client* client, struct wl_resource* resource, struct wl_resource* sibling)
		{
		}

		void Set_Sync(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Set_Desync(struct wl_client* client, struct wl_resource* resource)
		{
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* surface, struct wl_resource* parent) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_subsurface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		instances[resource].window = Window::Create(wl_client);
		instances[resource].surface = surface;

		Surface::instances[surface].window = instances[resource].window;
		Surface::instances[surface].type = 3;

		Surface::instances[parent].window->AddSubwindow(instances[resource].window);
		
		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		Surface::instances[instances[resource].surface].window = nullptr;
		Window::Destory(instances[resource].window);

		instances.erase(resource);
	}
}