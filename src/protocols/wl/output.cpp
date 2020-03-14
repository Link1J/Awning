#include "output.hpp"
#include <spdlog/spdlog.h>

#include "backends/manager.hpp"

namespace Awning::Protocols::WL::Output
{
	const struct wl_output_interface interface = {
		.release = Interface::Release,
	};
	std::unordered_map<Awning::Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
	std::unordered_map<wl_resource*, Awning::Output::ID> resource_to_outputId;

	namespace Interface
	{
		void Release(struct wl_client* client, struct wl_resource* resource)
		{
			Output::Destroy(resource);
		}
	}

	void Resize(void* data, int width, int height)
	{
		auto id = resource_to_outputId[(wl_resource*)data];

		auto [sX, sY] = Awning::Output::Get::Mode::Resolution (id, Awning::Output::Get::CurrentMode(id));
		auto refresh  = Awning::Output::Get::Mode::RefreshRate(id, Awning::Output::Get::CurrentMode(id));
		auto prefered = Awning::Output::Get::Mode::Prefered   (id, Awning::Output::Get::CurrentMode(id));
		auto current  = Awning::Output::Get::Mode::Current    (id, Awning::Output::Get::CurrentMode(id));

		wl_output_send_mode((wl_resource*)data, 
			(current  ? WL_OUTPUT_MODE_CURRENT   : 0) | 
			(prefered ? WL_OUTPUT_MODE_PREFERRED : 0) , 
			sX, sY, refresh
		);

		wl_output_send_done((wl_resource*)data);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{		
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_output_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, Destroy);

		Awning::Output::ID outputId = (Awning::Output::ID)data;

		auto [mX, mY] = Awning::Output::Get::Size        (outputId);
		auto [pX, pY] = Awning::Output::Get::Position    (outputId);
		auto manuf    = Awning::Output::Get::Manufacturer(outputId);
		auto model    = Awning::Output::Get::Model       (outputId);

		wl_output_send_geometry(resource, 
			pX, pY, mX, mY,
			WL_OUTPUT_SUBPIXEL_NONE, 
			manuf.c_str(), 
			model.c_str(), 
			WL_OUTPUT_TRANSFORM_NORMAL
		);

		wl_output_send_scale(resource, 1);

		for (int a = 0; a < Awning::Output::Get::NumberOfModes(outputId); a++)
		{		
			auto [sX, sY] = Awning::Output::Get::Mode::Resolution (outputId, a);
			auto refresh  = Awning::Output::Get::Mode::RefreshRate(outputId, a);
			auto prefered = Awning::Output::Get::Mode::Prefered   (outputId, a);
			auto current  = Awning::Output::Get::Mode::Current    (outputId, a);

			wl_output_send_mode(resource, 
				(current  ? WL_OUTPUT_MODE_CURRENT   : 0) | 
				(prefered ? WL_OUTPUT_MODE_PREFERRED : 0) , 
				sX, sY, refresh
			);
		}

		wl_output_send_done(resource);

		resource_to_outputId[resource] = outputId;
		outputId_to_resource[outputId].emplace(resource);

		Awning::Output::AddResize(outputId, Resize, resource);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_output_interface, 3, data, Bind);
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!resource_to_outputId.contains(resource))
			return;

		auto id = resource_to_outputId[resource];

		Awning::Output::RemoveResize(id, Resize, resource);

		outputId_to_resource[id].erase(resource);
		resource_to_outputId    .erase(resource);
	}
}