#include "output.hpp"
#include <spdlog/spdlog.h>

#include "backends/manager.hpp"

namespace Awning::Protocols::WL::Output
{
	const struct wl_output_interface interface = {
		.release = Interface::Release,
	};

	Data data;

	namespace Interface
	{
		void Release(struct wl_client* client, struct wl_resource* resource)
		{
			Output::Destroy(resource);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{		
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_output_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, Destroy);

		WM::Output::ID outputId = (WM::Output::ID)data;

		auto [mX, mY] = WM::Output::Get::Size        (outputId);
		auto [pX, pY] = WM::Output::Get::Position    (outputId);
		auto manuf    = WM::Output::Get::Manufacturer(outputId);
		auto model    = WM::Output::Get::Model       (outputId);

		wl_output_send_geometry(resource, 
			pX, pY, mX, mY,
			WL_OUTPUT_SUBPIXEL_NONE, 
			manuf.c_str(), 
			model.c_str(), 
			WL_OUTPUT_TRANSFORM_NORMAL
		);

		wl_output_send_scale(resource, 1);

		for (int a = 0; a < WM::Output::Get::NumberOfModes(outputId); a++)
		{		
			auto [sX, sY] = WM::Output::Get::Mode::Resolution (outputId, a);
			auto refresh  = WM::Output::Get::Mode::RefreshRate(outputId, a);
			auto prefered = WM::Output::Get::Mode::Prefered   (outputId, a);
			auto current  = WM::Output::Get::Mode::Current    (outputId, a);

			wl_output_send_mode(resource, 
				(current  ? WL_OUTPUT_MODE_CURRENT   : 0) | 
				(prefered ? WL_OUTPUT_MODE_PREFERRED : 0) , 
				sX, sY, refresh
			);
		}

		wl_output_send_done(resource);

		Output::data.resource_to_outputId[resource] = outputId;
		Output::data.outputId_to_resource[outputId].emplace(resource);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_output_interface, 3, data, Bind);
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.resource_to_outputId.contains(resource))
			return;

		auto id = data.resource_to_outputId[resource];
		data.outputId_to_resource[id].erase(resource);
		data.resource_to_outputId    .erase(resource);
	}
}