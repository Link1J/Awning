#include "data_source.hpp"
#include <spdlog/spdlog.h>

namespace Awning::Protocols::WL::Data_Source
{
	const struct wl_data_source_interface interface = {
		.offer       = Interface::Offer      ,
		.destroy     = Interface::Destroy    ,
		.set_actions = Interface::Set_Actions,
	};

	std::unordered_map<wl_resource*, Instance> instances;

	namespace Interface
	{
		void Offer(struct wl_client* client, struct wl_resource* resource, const char* mime_type)
		{
			auto& info = instances[resource];

			for (auto mime : info.mime_types)
				if (mime == mime_type)
					return;

			info.mime_types.push_back(mime_type);
		}

		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Data_Source::Destroy(resource);
		}

		void Set_Actions(struct wl_client* client, struct wl_resource* resource, uint32_t dnd_actions)
		{
			auto& info = instances[resource];
			info.dnd_actions = dnd_actions;
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_data_source_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		instances.erase(resource);
	}
}
