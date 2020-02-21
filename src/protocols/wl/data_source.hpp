#pragma once

#include <wayland-server.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace Awning::Protocols::WL::Data_Source
{
	struct Data 
	{
		struct Info 
		{
			std::vector<std::string> mime_types;
			uint32_t dnd_actions;
		};

		std::unordered_map<wl_resource*, Info> info;
	};

	extern const struct wl_data_source_interface interface;
	extern Data data;

	namespace Interface
	{
		void Offer(struct wl_client* client, struct wl_resource* resource, const char* mime_type);
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Actions(struct wl_client* client, struct wl_resource* resource, uint32_t dnd_actions);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);
}
