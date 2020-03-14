#include "decoration.hpp"
#include <spdlog/spdlog.h>

#include "protocols/wl/surface.hpp"

namespace Awning::Protocols::KDE::Decoration
{
	const struct org_kde_kwin_server_decoration_interface interface = {
		.release      = Interface::Release,
		.request_mode = Interface::Request_Mode,
	};
	std::unordered_map<wl_resource*,Instance> instances;

	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource *resource)
		{
		}

		void Request_Mode(struct wl_client *client, struct wl_resource *resource, uint32_t mode)
		{
			auto surface = instances[resource].surface;
			WL::Surface::instances[surface].window->Frame(mode == ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER);
			org_kde_kwin_server_decoration_send_mode(resource, mode);
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &org_kde_kwin_server_decoration_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		instances[resource].surface = surface;

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
	}
}

namespace Awning::Protocols::KDE::Decoration_Manager
{
	const struct org_kde_kwin_server_decoration_manager_interface interface = {
		.create = Interface::Create,
	};

	namespace Interface
	{
		void Create(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface)
		{
			Decoration::Create(client, 1, id, surface);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &org_kde_kwin_server_decoration_manager_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
		org_kde_kwin_server_decoration_manager_send_default_mode(resource, ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER);	
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &org_kde_kwin_server_decoration_manager_interface, 1, data, Bind);
	}
}