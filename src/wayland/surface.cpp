#include "surface.hpp"
#include "log.hpp"

#include <unordered_set>

extern std::unordered_set<wl_resource*> openWindows;

namespace Awning::Wayland::Surface
{
	const struct wl_surface_interface interface = {
		.destroy = Interface::Destroy,
		.attach = Interface::Attach,
		.damage = Interface::Damage,
		.frame = Interface::Frame,
		.set_opaque_region = Interface::Set_Opaque_Region,
		.set_input_region = Interface::Set_Input_Region,
		.commit = Interface::Commit,
		.set_buffer_transform = Interface::Set_Buffer_Transform,
		.set_buffer_scale = Interface::Set_Buffer_Scale,
		.damage_buffer = Interface::Damage_Buffer,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}

		void Attach(struct wl_client* client, struct wl_resource* resource, struct wl_resource* buffer, int32_t x, int32_t y)
		{
			Log::Function::Called("Wayland::Surface::Interface");
			data.surfaces[resource].buffer = buffer;
		}

		void Damage(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			Log::Function::Called("Wayland::Surface::Interface");
			data.surfaces[resource].damaged = true;
		}

		void Frame(struct wl_client* client, struct wl_resource* resource, uint32_t callback)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}

		void Set_Opaque_Region(struct wl_client* client, struct wl_resource* resource, struct wl_resource* region)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}

		void Set_Input_Region(struct wl_client* client, struct wl_resource* resource, struct wl_resource* region)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}

		void Commit(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Wayland::Surface::Interface");

			auto& surface = data.surfaces[resource];

			if (surface.buffer == nullptr)
				return;

			struct wl_shm_buffer * shmBuffer = wl_shm_buffer_get(surface.buffer);

			if (shmBuffer)
			{
				surface.xDimension =        wl_shm_buffer_get_width (shmBuffer);
				surface.yDimension =        wl_shm_buffer_get_height(shmBuffer);
				surface.data       = (char*)wl_shm_buffer_get_data  (shmBuffer);
			}

			wl_buffer_send_release(surface.buffer);
			surface.buffer = nullptr;
		}

		void Set_Buffer_Transform(struct wl_client* client, struct wl_resource* resource, int32_t transform)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}

		void Set_Buffer_Scale(struct wl_client* client, struct wl_resource* resource, int32_t scale)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}

		void Damage_Buffer(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			Log::Function::Called("Wayland::Surface::Interface");
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Wayland::Surface");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);
		data.surfaces[resource] = Data::Instance();
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Wayland::Surface");

		openWindows.erase(resource);
		data.surfaces.erase(resource);
	}
}