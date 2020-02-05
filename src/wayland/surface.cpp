#include "surface.hpp"
#include "log.hpp"
#include "protocols/xdg-shell-protocol.h"

#include <unordered_set>
#include <chrono>

#include <vector>

uint32_t NextSerialNum();

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

	std::unordered_set<wl_resource*> frameCallbacks;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Wayland::Surface::Interface");
			Surface::Destroy(resource);
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

			struct wl_resource* resource_cb = wl_resource_create(client, &wl_callback_interface, 1, callback);
			if (resource_cb == nullptr) {
				wl_client_post_no_memory(client);
				return;
			}
			wl_resource_set_implementation(resource_cb, nullptr, nullptr, [](struct wl_resource* resource){
				Log::Function::Called("Wayland::Surface::Interface::Frame.callback");
				frameCallbacks.erase(resource);
			});
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
			{
				if (surface.window)
				{
					surface.window->Texture(surface.texture);
					WM::Manager::Window::Raise(surface.window);
				}
				if (surface.type == 1)
				{
					xdg_surface_send_configure(surface.shell, NextSerialNum());
				}
				return;
			}

			struct wl_shm_buffer * shmBuffer = wl_shm_buffer_get(surface.buffer);

			if (shmBuffer)
			{
				surface.texture->width        =           wl_shm_buffer_get_width (shmBuffer);
				surface.texture->height       =           wl_shm_buffer_get_height(shmBuffer);
				surface.texture->bitsPerPixel =           32;
				surface.texture->bytesPerLine =           wl_shm_buffer_get_stride(shmBuffer);
				surface.texture->size         =           surface.texture->bytesPerLine * surface.texture->height;
				surface.texture->buffer.u8    = (uint8_t*)wl_shm_buffer_get_data  (shmBuffer);
				surface.texture->red          = { .size = 8, .offset = 16 };
				surface.texture->green        = { .size = 8, .offset =  8 };
				surface.texture->blue         = { .size = 8, .offset =  0 };
				surface.texture->alpha        = { .size = 8, .offset = 24 };

				if (surface.window)
				{
					if (surface.window->XSize() == 0 && surface.window->YSize() == 0)
					{
						surface.window->ConfigSize(surface.texture->width, surface.texture->height);
					}

					surface.window->Mapped(true);
				}
			}

			//wl_buffer_send_release(surface.buffer);
			//surface.buffer = nullptr;
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
		
		data.surfaces[resource].client = wl_client;
		data.surfaces[resource].texture = new WM::Texture::Data();
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Wayland::Surface");

		if (!data.surfaces.contains(resource))
			return;

		delete data.surfaces[resource].texture;
		data.surfaces.erase(resource);
	}

	void HandleFrameCallbacks()
	{
		for (auto i: frameCallbacks)
		{
			auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
			wl_callback_send_done(i, time);
			wl_resource_destroy(i);
		}
		frameCallbacks.clear();
	}
}