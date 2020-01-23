#pragma once

#include "protocols/xdg-shell-protocol.h"
#include "protocols/xdg-decoration-protocol.h"
#include <unordered_map>
#include <string>

namespace Awning::ZXDG::Toplevel_Decoration
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* toplevel;
		};

		std::unordered_map<wl_resource*,Instance> decorations;
	};

	extern const struct zxdg_toplevel_decoration_v1_interface interface;
	extern Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Mode(struct wl_client* client, struct wl_resource* resource, uint32_t mode);
		void Unset_Mode(struct wl_client* client, struct wl_resource *resource);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* toplevel);
	void Destroy(struct wl_resource* resource);
}

namespace Awning::ZXDG::Decoration_Manager
{
	struct Data 
	{
		wl_global* global;
	};

	extern const struct zxdg_decoration_manager_v1_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Get_Toplevel_Decoration(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* toplevel);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}