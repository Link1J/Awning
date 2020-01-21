#include "output.hpp"
#include "log.hpp"

#include "backends/X11.hpp"

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

		if (version >= WL_OUTPUT_GEOMETRY_SINCE_VERSION)
			wl_output_send_geometry(resource, 0, 0, X11::Width(), X11::Height(), WL_OUTPUT_SUBPIXEL_NONE, "X.Org Foundation", "11.0", WL_OUTPUT_TRANSFORM_NORMAL);
		if (version >= WL_OUTPUT_SCALE_SINCE_VERSION   )
			wl_output_send_scale   (resource, 1);
		if (version >= WL_OUTPUT_MODE_SINCE_VERSION    )
			wl_output_send_mode    (resource, WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED, X11::Width(), X11::Height(), 60000);
		if (version >= WL_OUTPUT_DONE_SINCE_VERSION    )
			wl_output_send_done    (resource);
	}
}