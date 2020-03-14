#include "output_manager.hpp"
#include <spdlog/spdlog.h>
#include "backends/manager.hpp"
#include <unistd.h>

extern uint32_t NextSerialNum();

namespace Awning::Protocols::WLR::Output_Manager
{
	const struct zwlr_output_manager_v1_interface interface = {
		.create_configuration = Interface::Create_Configuration,
		.stop                 = Interface::Stop                ,
	};
	std::unordered_map<wl_resource*, bool> sendDisplays;

	namespace Interface
	{
		void Create_Configuration(struct wl_client* client, struct wl_resource* resource, uint32_t id, uint32_t serial)
		{
			Output_Configuration::Create(client, wl_resource_get_version(resource), id);
		}

		void Stop(struct wl_client* client, struct wl_resource* resource)
		{
			Output_Manager::sendDisplays[resource] = false;
		}
	}
	
	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zwlr_output_manager_v1_interface, 1, data, Bind);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_manager_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		Output_Manager::sendDisplays[resource] = true;

		auto displays = Backend::GetDisplays();
		for (auto display : displays)
		{
			auto head = Head::Create(wl_client, version, 0, display.output, resource);
			Head::SendData(head);
		}

		zwlr_output_manager_v1_send_done(resource, NextSerialNum());
	}
}

namespace Awning::Protocols::WLR::Head
{
	std::unordered_map<Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
	std::unordered_map<wl_resource*, Output::ID> resource_to_outputId;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}
	}

	void SendData(wl_resource* resource)
	{
		auto outputId = resource_to_outputId[resource];

		auto [mX, mY] = Awning::Output::Get::Size        (outputId);
		auto [pX, pY] = Awning::Output::Get::Position    (outputId);
		auto manuf    = Awning::Output::Get::Description (outputId);
		auto model    = Awning::Output::Get::Name        (outputId);

		zwlr_output_head_v1_send_name(resource, model.c_str());
		zwlr_output_head_v1_send_description(resource, manuf.c_str());
		zwlr_output_head_v1_send_enabled(resource, true);
		zwlr_output_head_v1_send_position(resource, pX, pY);
		zwlr_output_head_v1_send_scale(resource, wl_fixed_from_int(1));
		zwlr_output_head_v1_send_transform(resource, WL_OUTPUT_TRANSFORM_NORMAL);
		zwlr_output_head_v1_send_physical_size(resource, mX, mY);

		for (int a = 0; a < Awning::Output::Get::NumberOfModes(outputId); a++)
		{
			auto mode = Mode::Create(wl_resource_get_client(resource), wl_resource_get_version(resource), 0, outputId, a, resource);
			Mode::SendData(mode);

			if (Awning::Output::Get::Mode::Current(outputId, a))
				zwlr_output_head_v1_send_current_mode(resource, mode);
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, Awning::Output::ID outputId, wl_resource* manager)
	{		
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_head_v1_interface, version, 0);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, nullptr, nullptr, Destroy);
		zwlr_output_manager_v1_send_head(manager, resource);

		Head::resource_to_outputId[resource] = outputId;
		Head::outputId_to_resource[outputId].emplace(resource);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!resource_to_outputId.contains(resource))
			return;

		auto id = resource_to_outputId[resource];
		outputId_to_resource[id].erase(resource);
		resource_to_outputId    .erase(resource);
	}
}

namespace Awning::Protocols::WLR::Mode
{
	std::unordered_map<Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
	std::unordered_map<wl_resource*, Output::ID> resource_to_outputId;
	std::unordered_map<wl_resource*, int> resource_to_mode;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{			
		}
	}

	void SendData(wl_resource* resource)
	{
		auto outputId = resource_to_outputId[resource];
		auto mode     = resource_to_mode    [resource];

		auto [mX, mY] = Awning::Output::Get::Mode::Resolution (outputId, mode);
		auto refresh  = Awning::Output::Get::Mode::RefreshRate(outputId, mode);

		zwlr_output_mode_v1_send_size     (resource, mX, mY );
		zwlr_output_mode_v1_send_refresh  (resource, refresh);

		if (Awning::Output::Get::Mode::Prefered(outputId, mode))
			zwlr_output_mode_v1_send_preferred(resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, Awning::Output::ID outputId, int mode, wl_resource* head)
	{		
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_mode_v1_interface, version, 0);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, nullptr, nullptr, Destroy);
		zwlr_output_head_v1_send_mode(head, resource);

		Mode::resource_to_outputId[resource] = outputId;
		Mode::outputId_to_resource[outputId].emplace(resource);
		Mode::resource_to_mode    [resource] = mode;

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!resource_to_outputId.contains(resource))
			return;

		auto id = resource_to_outputId[resource];
		outputId_to_resource[id].erase(resource);
		resource_to_outputId    .erase(resource);
		resource_to_mode        .erase(resource);
	}
}

namespace Awning::Protocols::WLR::Output_Configuration
{	
	const struct zwlr_output_configuration_v1_interface interface = {
		.enable_head  = Interface::Enable_Head ,
		.disable_head = Interface::Disable_Head,
		.apply        = Interface::Apply       ,
		.test         = Interface::Test        ,
		.destroy      = Interface::Destroy     ,
	};

	namespace Interface
	{
		void Enable_Head(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* head)
		{
		}

		void Disable_Head(struct wl_client* client, struct wl_resource* resource, struct wl_resource* head)
		{
		}

		void Apply(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Test(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{		
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_configuration_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
	}
}