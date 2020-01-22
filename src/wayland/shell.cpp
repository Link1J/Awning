#include "shell.hpp"
#include "shell_surface.hpp"
#include "log.hpp"

namespace Awning::Wayland::Shell
{
	const struct wl_shell_interface interface = {
		.get_shell_surface = Interface::Get_Shell_Surface
	};

	Data data;

	namespace Interface
	{
		void Get_Shell_Surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface)
		{
			Log::Function::Called("Wayland::Shell::Interface");
			Shell_Surface::Create(client, wl_resource_get_version(resource), id, surface);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Wayland::Shell");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_shell_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}
}