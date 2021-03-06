#include "decoration.hpp"
#include <spdlog/spdlog.h>

#include "protocols/xdg/toplevel.hpp"

namespace Awning::Protocols::ZXDG::Toplevel_Decoration
{
	const struct zxdg_toplevel_decoration_v1_interface interface = {
		.destroy    = Interface::Destroy,
		.set_mode   = Interface::Set_Mode,
		.unset_mode = Interface::Unset_Mode,
	};
	std::unordered_map<wl_resource*,Instance> instances;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Set_Mode(struct wl_client* client, struct wl_resource* resource, uint32_t mode)
		{
			auto toplevel = instances[resource].toplevel;
			XDG::TopLevel::instances[toplevel].window->Frame(mode != ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
		}

		void Unset_Mode(struct wl_client* client, struct wl_resource *resource)
		{
			auto toplevel = instances[resource].toplevel;
			XDG::TopLevel::instances[toplevel].window->Frame(true);
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* toplevel)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_toplevel_decoration_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		instances[resource].toplevel = toplevel;

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
	}
}

namespace Awning::Protocols::ZXDG::Decoration_Manager
{
	const struct zxdg_decoration_manager_v1_interface interface = {
		.destroy                 = Interface::Destroy,
		.get_toplevel_decoration = Interface::Get_Toplevel_Decoration,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Get_Toplevel_Decoration(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* toplevel)
		{
			Toplevel_Decoration::Create(client, 1, id, toplevel);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_decoration_manager_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zxdg_decoration_manager_v1_interface, 1, data, Bind);
	}
}