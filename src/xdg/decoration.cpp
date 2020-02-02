#include "decoration.hpp"
#include "toplevel.hpp"
#include "log.hpp"

namespace Awning::ZXDG::Toplevel_Decoration
{
	const struct zxdg_toplevel_decoration_v1_interface interface = {
		.destroy    = Interface::Destroy,
		.set_mode   = Interface::Set_Mode,
		.unset_mode = Interface::Unset_Mode,
	};
	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("ZXDG::Toplevel_Decoration::Interface");
		}

		void Set_Mode(struct wl_client* client, struct wl_resource* resource, uint32_t mode)
		{
			Log::Function::Called("ZXDG::Toplevel_Decoration::Interface");
			auto toplevel = data.decorations[resource].toplevel;
			XDG::TopLevel::data.toplevels[toplevel].window->Frame(mode != ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
		}

		void Unset_Mode(struct wl_client* client, struct wl_resource *resource)
		{
			Log::Function::Called("ZXDG::Toplevel_Decoration::Interface");
			auto toplevel = data.decorations[resource].toplevel;
			XDG::TopLevel::data.toplevels[toplevel].window->Frame(true);
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* toplevel)
	{
		Log::Function::Called("ZXDG::Toplevel_Decoration");

		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_toplevel_decoration_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.decorations[resource].toplevel = toplevel;
	}

	void Destroy(struct wl_resource* resource)
	{
	}
}

namespace Awning::ZXDG::Decoration_Manager
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
			Log::Function::Called("ZXDG::Decoration_Manager::Interface");
		}

		void Get_Toplevel_Decoration(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* toplevel)
		{
			Log::Function::Called("ZXDG::Decoration_Manager::Interface");
			Toplevel_Decoration::Create(client, 1, id, toplevel);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		Log::Function::Called("ZXDG::Decoration_Manager");

		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_decoration_manager_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}
}