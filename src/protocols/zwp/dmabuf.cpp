#include "dmabuf.hpp"
#include <spdlog/spdlog.h>

#include "renderers/egl.hpp"

#include <libdrm/drm_fourcc.h>

#include <unistd.h>
#include <string.h>

namespace Awning::Protocols::ZWP::Linux_Dmabuf
{
	const struct zwp_linux_dmabuf_v1_interface interface = {
		.destroy       = Interface::Destroy      ,
		.create_params = Interface::Create_Params,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Create_Params(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Linux_Buffer_Params::Create(client, wl_resource_get_version(resource), id);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zwp_linux_dmabuf_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		EGLint formatCount = 256, formats[256];
		eglQueryDmaBufFormatsEXT(Renderers::EGL::display, formatCount, formats, &formatCount);

		for (int a = 0; a < std::min(formatCount, 256); a++)
		{
			EGLint modifierCount = 256;
			EGLuint64KHR modifiers[256];
			EGLBoolean external[256];
			eglQueryDmaBufModifiersEXT(Renderers::EGL::display, formats[a], modifierCount, modifiers, external, &modifierCount);

			if (modifierCount == 0)
			{
				modifierCount = 1;
				modifiers[0] = DRM_FORMAT_MOD_INVALID;
			}

			for (int b = 0; b < std::min(modifierCount, 256); b++)
			{
				if (version >= ZWP_LINUX_DMABUF_V1_MODIFIER_SINCE_VERSION) 
				{
					uint32_t modifier_lo = modifiers[b] & 0xFFFFFFFF;
					uint32_t modifier_hi = modifiers[b] >> 32;
					zwp_linux_dmabuf_v1_send_modifier(resource, formats[a], modifier_hi, modifier_lo);
				}
				else
				{
					zwp_linux_dmabuf_v1_send_format(resource, formats[a]);
				}
				
			}
		}
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zwp_linux_dmabuf_v1_interface, 3, data, Bind);
	}
}

namespace Awning::Protocols::ZWP::Linux_Buffer_Params
{
	const struct zwp_linux_buffer_params_v1_interface interface = {
		.destroy      = Interface::Destroy,
		.add          = Interface::Add,
		.create       = Interface::Create,
		.create_immed = Interface::Create_Immed,
	};

	Data data;

	void CreateBuffer(struct wl_client* client, struct wl_resource* resource, uint32_t buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t flags);

	static void BufferDestroy(struct wl_client *client, struct wl_resource *resource) {
		delete resource->data;
		resource->data = nullptr;
		wl_resource_destroy(resource);
	}

	static const struct wl_buffer_interface buffer_impl = {
		.destroy = BufferDestroy,
	};

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Linux_Buffer_Params::Destroy(resource);
		}

		void Add(struct wl_client* client, struct wl_resource* resource, int32_t fd, uint32_t plane_idx, uint32_t offset, uint32_t stride, uint32_t modifier_hi, uint32_t modifier_lo)
		{
			if (!data.instances.contains(resource))
			{
				close(fd);
				return;
			}

			if (plane_idx >= DMABUF_MAX_PLANES)
			{
				wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX, "plane index %u > %u", plane_idx, DMABUF_MAX_PLANES);
				close(fd);
				return;
			}

			auto& instance = data.instances[resource];
			auto& plane = instance.planes[plane_idx];
			uint64_t modifier = ((uint64_t)modifier_hi << 32) | modifier_lo;

			if (plane.fd != -1) 
			{
				close(fd);
				return;
			}

			if (instance.hasMod && modifier != instance.modifier) 
			{
				wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_FORMAT, "sent modifier %" PRIu64 " for plane %u, expected" " modifier %" PRIu64 " like other planes", modifier, plane_idx, instance.modifier);
				close(fd);
				return;
			}

			instance.modifier = modifier;
			instance.hasMod = true;

			plane.fd = fd;
			plane.offset = offset;
			plane.stride = stride;

			instance.planesUsed++;
		}

		void Create(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height, uint32_t format, uint32_t flags)
		{
			CreateBuffer(client, resource, 0, width, height, format, flags);
		}

		void Create_Immed(struct wl_client* client, struct wl_resource* resource, uint32_t buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t flags)
		{
			CreateBuffer(client, resource, buffer_id, width, height, format, flags);
		}
	}

	void CreateBuffer(struct wl_client* client, struct wl_resource* resource, uint32_t buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t flags)
	{
		if (!data.instances.contains(resource))
			return;

		auto& instance = data.instances[resource];

		instance.width  = width ;
		instance.height = height;
		instance.format = format;
		instance.flags  = flags ;

		Data::Instance* copy = new Data::Instance();
		memcpy(copy, &instance, sizeof(instance));

		auto buffer_resource = wl_resource_create(client, &wl_buffer_interface, 1, buffer_id);
		if (!buffer_resource) {
			wl_resource_post_no_memory(resource);
			if (buffer_id == 0)
				zwp_linux_buffer_params_v1_send_failed(resource);
		}

		wl_resource_set_implementation(buffer_resource, &buffer_impl, copy, BufferDestroy);

		if (buffer_id == 0)
			zwp_linux_buffer_params_v1_send_created(resource, buffer_resource);
	}

	void BufferDestroy(struct wl_resource* resource)
	{
		if (!resource->data)
			return;
		delete resource->data;
		resource->data = nullptr;
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zwp_linux_buffer_params_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.instances[resource] = Data::Instance();

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.instances.contains(resource))
			return;

		data.instances.erase(resource);
	}
}