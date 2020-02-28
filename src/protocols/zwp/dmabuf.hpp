#pragma once

#include "protocols/handler/linux-dmabuf.h"
#include <unordered_map>

namespace Awning::Protocols::ZWP::Linux_Dmabuf
{
	struct Data 
	{
	};

	extern const struct zwp_linux_dmabuf_v1_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Create_Params(struct wl_client* client, struct wl_resource* resource, uint32_t id);
	}

	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}

namespace Awning::Protocols::ZWP::Linux_Buffer_Params
{
	const int DMABUF_MAX_PLANES = 4;

	struct Data 
	{
		struct Instance 
		{
			struct  
			{
				int32_t fd = -1;
				uint32_t offset, stride;
			}
			planes[DMABUF_MAX_PLANES];

			bool hasMod = false;
			int planesUsed = 0;
			uint64_t modifier;
			int32_t width, height;
			uint32_t format, flags;
		};

		std::unordered_map<wl_resource*, Instance> instances;
	};

	extern const struct zwp_linux_buffer_params_v1_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Add(struct wl_client* client, struct wl_resource* resource, int32_t fd, uint32_t plane_idx, uint32_t offset, uint32_t stride, uint32_t modifier_hi, uint32_t modifier_lo);
		void Create(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height, uint32_t format, uint32_t flags);
		void Create_Immed(struct wl_client* client, struct wl_resource* resource, uint32_t buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t flags);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);

	void BufferDestroy(struct wl_resource* resource);
}