#include "output.hpp"
#include "log.hpp"

#include "protocols/wl/output.hpp"

namespace Awning::Protocols::ZXDG::Output
{
	const struct zxdg_output_v1_interface interface = {
		.destroy    = Interface::Destroy,
	};
	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::ZXDG::Output::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* output)
	{
		Log::Function::Called("Protocols::ZXDG::Output");

		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_output_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		WM::Output::ID outputID = WL::Output::data.resource_to_outputId[output];

		auto [px, py] = WM::Output::Get::Position(outputID);
		auto [sx, sy] = WM::Output::Get::Mode::Resolution(outputID, WM::Output::Get::CurrentMode(outputID));

		if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION       )
			zxdg_output_v1_send_name       (resource, WM::Output::Get::Model       (outputID).c_str());
		if (version >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION)
			zxdg_output_v1_send_description(resource, WM::Output::Get::Manufacturer(outputID).c_str());

		zxdg_output_v1_send_logical_position(resource, px, py);
		zxdg_output_v1_send_logical_size    (resource, sx, sy);

		zxdg_output_v1_send_done(resource);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
	}
}

namespace Awning::Protocols::ZXDG::Output_Manager
{
	const struct zxdg_output_manager_v1_interface interface = {
		.destroy        = Interface::Destroy       ,
		.get_xdg_output = Interface::Get_Xdg_Output,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::ZXDG::Output_Manager::Interface");
		}

		void Get_Xdg_Output(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* output)
		{
			Log::Function::Called("Protocols::ZXDG::Output_Manager::Interface");
			Output::Create(client, 1, id, output);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Protocols::ZXDG::Output_Manager");

		struct wl_resource* resource = wl_resource_create(wl_client, &zxdg_output_manager_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zxdg_output_manager_v1_interface, 1, data, Bind);
	}
}