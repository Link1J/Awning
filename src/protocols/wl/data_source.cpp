#include "data_source.hpp"
#include "log.hpp"

namespace Awning::Protocols::WL::Data_Source
{
	const struct wl_data_source_interface interface = {
		.offer       = Interface::Offer      ,
		.destroy     = Interface::Destroy    ,
		.set_actions = Interface::Set_Actions,
	};

	Data data;

	namespace Interface
	{
		void Offer(struct wl_client* client, struct wl_resource* resource, const char* mime_type)
		{
			Log::Function::Called("Protocols::WL::Data_Source::Interface");

			auto& info = data.info[resource];

			for (auto mime : info.mime_types)
				if (mime == mime_type)
					return;

			info.mime_types.push_back(mime_type);
		}

		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WL::Data_Source::Interface");
			Data_Source::Destroy(resource);
		}

		void Set_Actions(struct wl_client* client, struct wl_resource* resource, uint32_t dnd_actions)
		{
			Log::Function::Called("Protocols::WL::Data_Source::Interface");
			
			auto& info = data.info[resource];
			info.dnd_actions = dnd_actions;
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Protocols::WL::Data_Source");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_data_source_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::WL::Data_Source");

		if (!data.info.contains(resource))
			return;

		data.info.erase(resource);
	}
}
