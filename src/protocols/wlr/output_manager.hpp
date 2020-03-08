#include "protocols/handler/wlr-output-management.h"
#include <unordered_map>
#include <unordered_set>
#include "wm/output.hpp"

namespace Awning::Protocols::WLR::Output_Manager
{
	struct Data 
	{
		std::unordered_map<wl_resource*, bool> sendDisplays;
	};

	extern const struct zwlr_output_manager_v1_interface interface;
	extern Data data;

	namespace Interface
	{
		void Create_Configuration(struct wl_client* client, struct wl_resource* resource, uint32_t id, uint32_t serial);
		void Stop(struct wl_client* client, struct wl_resource* resource);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}

namespace Awning::Protocols::WLR::Head
{
	struct Data 
	{
		std::unordered_map<Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
		std::unordered_map<wl_resource*, Output::ID> resource_to_outputId;
	};

	extern Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, Output::ID outputId, wl_resource* manager);
	void Destroy(struct wl_resource* resource);
	void SendData(wl_resource* resource);
}

namespace Awning::Protocols::WLR::Mode
{
	struct Data 
	{
		std::unordered_map<Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
		std::unordered_map<wl_resource*, Output::ID> resource_to_outputId;
		std::unordered_map<wl_resource*, int> resource_to_mode;
	};

	extern Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, Output::ID outputId, int mode, wl_resource* head);
	void Destroy(struct wl_resource* resource);
	void SendData(wl_resource* resource);
}

namespace Awning::Protocols::WLR::Output_Configuration
{
	struct Data 
	{
	};
	
	extern const struct zwlr_output_configuration_v1_interface interface;
	extern Data data;

	namespace Interface
	{
		void Enable_Head(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* head);
		void Disable_Head(struct wl_client* client, struct wl_resource* resource, struct wl_resource* head);
		void Apply(struct wl_client* client, struct wl_resource* resource);
		void Test(struct wl_client* client, struct wl_resource* resource);
		void Destroy(struct wl_client* client, struct wl_resource* resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);
}