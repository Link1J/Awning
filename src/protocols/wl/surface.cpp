#include "surface.hpp"
#include <spdlog/spdlog.h>
#include "protocols/handler/xdg-shell.h"
#include "renderers/manager.hpp"

#include "protocols/zwp/dmabuf.hpp"

#include <cstring>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <GL/gl.h>

#include <unordered_set>
#include <chrono>
#include <vector>

uint32_t NextSerialNum();

namespace Awning
{
	namespace Server
	{
		struct Data
		{
			wl_display* display;
			wl_event_loop* event_loop;
			wl_protocol_logger* logger; 
			wl_listener client_listener;

			struct {
				EGLDisplay display;
				EGLint major, minor;
				EGLContext context;
				EGLSurface surface;
			} egl;
		};
		extern Data data;
	}
};

extern PFNEGLQUERYWAYLANDBUFFERWL eglQueryWaylandBufferWL;

namespace Awning::Protocols::WL::Surface
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

	struct FrameCallback
	{
		bool alive;
	};

	std::unordered_map<wl_resource*, FrameCallback> frameCallbacks;
	std::vector<wl_resource*> list;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Surface::Destroy(resource);
		}

		void Attach(struct wl_client* client, struct wl_resource* resource, struct wl_resource* buffer, int32_t x, int32_t y)
		{
			data.surfaces[resource].buffer = buffer;
		}

		void Damage(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			auto& surface = data.surfaces[resource];
			surface.damage.xp = x;
			surface.damage.yp = y;
			surface.damage.xs = width;
			surface.damage.ys = height;
		}

		void Release_Callback(struct wl_resource* resource)
		{
			frameCallbacks[resource].alive = false;
		}

		void Frame(struct wl_client* client, struct wl_resource* resource, uint32_t callback)
		{
			struct wl_resource* resource_cp = wl_resource_create(client, &wl_callback_interface, 1, callback);
			if (resource == nullptr) {
				wl_client_post_no_memory(client);
				return;
			}
			wl_resource_set_implementation(resource_cp, NULL, NULL, Release_Callback);
			frameCallbacks[resource_cp].alive = true;
			list.push_back(resource_cp);
		}

		void Set_Opaque_Region(struct wl_client* client, struct wl_resource* resource, struct wl_resource* region)
		{
		}

		void Set_Input_Region(struct wl_client* client, struct wl_resource* resource, struct wl_resource* region)
		{
		}

		void Commit(struct wl_client* client, struct wl_resource* resource)
		{
			auto& surface = data.surfaces[resource];

			if (surface.buffer == nullptr)
			{
				if (surface.window)
				{
					surface.window->Texture(surface.texture);
					Window::Manager::Raise(surface.window);
				}
				if (surface.type == 1)
				{
					xdg_surface_send_configure(surface.shell, NextSerialNum());
				}
				if (surface.type == 2)
				{
					xdg_surface_send_configure(surface.shell, NextSerialNum());
				}
				return;
			}

			if (surface.window)
			{
				if (!surface.window->Texture())
				{
					surface.window->Texture(surface.texture);
					Window::Manager::Raise(surface.window);
				}
			}

			EGLint texture_format;
			auto shm_buffer = wl_shm_buffer_get(surface.buffer);

			if (surface.buffer->destroy == Protocols::ZWP::Linux_Buffer_Params::BufferDestroy)
				Renderers::FillTextureFrom::LinuxDMABuf(surface.buffer, surface.texture, surface.damage);
			else if (shm_buffer)
				Renderers::FillTextureFrom::SHMBuffer(shm_buffer, surface.texture, surface.damage);
			else if (eglQueryWaylandBufferWL(Server::data.egl.display, surface.buffer, EGL_TEXTURE_FORMAT, &texture_format))
				Renderers::FillTextureFrom::EGLImage(surface.buffer, surface.texture, surface.damage);

			if (surface.window)
			{
				if ((surface.window->XSize() == 0 && surface.window->YSize() == 0) || surface.type == 2 || surface.type == 3)
					Window::Manager::Resize(surface.window, surface.texture->width, surface.texture->height);

				surface.window->Mapped(true);
			}

			wl_buffer_send_release(surface.buffer);
			surface.buffer = nullptr;
		}

		void Set_Buffer_Transform(struct wl_client* client, struct wl_resource* resource, int32_t transform)
		{
		}

		void Set_Buffer_Scale(struct wl_client* client, struct wl_resource* resource, int32_t scale)
		{
		}

		void Damage_Buffer(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			auto& surface = data.surfaces[resource];
			surface.damage.xp = x;
			surface.damage.yp = y;
			surface.damage.xs = width;
			surface.damage.ys = height;
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);
		
		data.surfaces[resource].client = wl_client;
		data.surfaces[resource].texture = new Texture();
		memset(data.surfaces[resource].texture, 0, sizeof(Texture));

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.surfaces.contains(resource))
			return;

		delete data.surfaces[resource].texture;
		data.surfaces.erase(resource);
	}

	void HandleFrameCallbacks()
	{
		for (auto i: list)
		{
			if (frameCallbacks[i].alive)
			{
				uint32_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
				wl_callback_send_done(i, time);
				wl_resource_destroy(i);
			}
			frameCallbacks.erase(i);
		}
		frameCallbacks.clear();
	}
}