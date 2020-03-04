#pragma once

#include "protocols/handler/server_decoration.h"
#include <unordered_map>
#include <string>

namespace Awning::Protocols::KDE::Decoration
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* surface;
		};

		std::unordered_map<wl_resource*,Instance> instances;
	};

	extern const struct org_kde_kwin_server_decoration_interface interface;
	extern Data data;

	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource *resource);
		void Request_Mode(struct wl_client *client, struct wl_resource *resource, uint32_t mode);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* toplevel);
	void Destroy(struct wl_resource* resource);
}

namespace Awning::Protocols::KDE::Decoration_Manager
{
	struct Data 
	{
	};

	extern const struct org_kde_kwin_server_decoration_manager_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Create(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);
	}

	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}