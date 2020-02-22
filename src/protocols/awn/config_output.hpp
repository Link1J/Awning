#pragma once

#include "protocols/handler/awn-config.h"

namespace Awning::Protocols::AWN::Config_Output
{
	struct Data 
	{
	};

	extern const struct awn_config_output_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Mode(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output, int32_t mode);
		void Set_Position(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output, int32_t x, int32_t y);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);
}