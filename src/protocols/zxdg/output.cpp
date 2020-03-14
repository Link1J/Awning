#include "output.hpp"
#include <spdlog/spdlog.h>

#include "protocols/wl/output.hpp"

namespace Awning::Protocols::ZXDG::Output
{
	const struct zxdg_output_v1_interface interface = {
		.destroy    = Interface::Destroy,
	};
	std::unordered_map<wl_resource*, Instance> instances;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Output::Destroy(resource);
		}
	}

	void Resize(void* data, int width, int height)
	{
		zxdg_output_v1_send_logical_size((wl_resource*)data, width, height);
		zxdg_output_v1_send_done((wl_resource*)data);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* output)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_output_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		Awning::Output::ID outputID = WL::Output::resource_to_outputId[output];

		auto [px, py] = Awning::Output::Get::Position(outputID);
		auto [sx, sy] = Awning::Output::Get::Mode::Resolution(outputID, Awning::Output::Get::CurrentMode(outputID));

		if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION       )
			zxdg_output_v1_send_name       (resource, Awning::Output::Get::Name       (outputID).c_str());
		if (version >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION)
			zxdg_output_v1_send_description(resource, Awning::Output::Get::Description(outputID).c_str());

		zxdg_output_v1_send_logical_position(resource, px, py);
		zxdg_output_v1_send_logical_size    (resource, sx, sy);

		zxdg_output_v1_send_done(resource);

		Awning::Output::AddResize(outputID, Resize, resource);

		instances[resource].output = output;
		instances[resource].id = outputID;

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		Awning::Output::ID outputID = instances[resource].id;
		Awning::Output::RemoveResize(outputID, Resize, resource);
		instances.erase(resource);
	}
}

namespace Awning::Protocols::ZXDG::Output_Manager
{
	const struct zxdg_output_manager_v1_interface interface = {
		.destroy        = Interface::Destroy       ,
		.get_xdg_output = Interface::Get_Xdg_Output,
	};

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Get_Xdg_Output(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* output)
		{
			Output::Create(client, wl_resource_get_version(resource), id, output);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_output_manager_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zxdg_output_manager_v1_interface, 2, data, Bind);
	}
}