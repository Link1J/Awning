#include "output.hpp"
#include "log.hpp"

#include "backends/manager.hpp"

namespace Awning::Wayland::Output
{
	const struct wl_output_interface interface = {
		.release = Interface::Release,
	};

	Data data;

	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource *resource)
		{
			Log::Function::Called("Wayland::Output::Interface");
			wl_resource_destroy(resource);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Wayland::Output");
		
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_output_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		for (auto& output : Backend::Outputs::Get())
		{
			wl_output_send_geometry(resource, 
				0, 
				0, 
				output.physical.width, 
				output.physical.height, 
				WL_OUTPUT_SUBPIXEL_NONE, 
				output.manufacturer.c_str(), 
				output.model.c_str(), 
				WL_OUTPUT_TRANSFORM_NORMAL
			);

			wl_output_send_scale(resource, 1);

			for (auto& mode : output.modes)
			{
				wl_output_send_mode(resource, 
					(mode.current  ? WL_OUTPUT_MODE_CURRENT   : 0) | 
					(mode.prefered ? WL_OUTPUT_MODE_PREFERRED : 0) , 
					mode.resolution.width, 
					mode.resolution.height, 
					mode.refresh_rate
				);
			}
		}
		wl_output_send_done(resource);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_output_interface, 2, data, Bind);
	}
}