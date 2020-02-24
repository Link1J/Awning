#include "output_manager.hpp"
#include "log.hpp"
#include "backends/manager.hpp"

namespace Awning::Protocols::WLR::Output_Manager
{
	const struct zwlr_output_manager_v1_interface interface = {
		.create_configuration = Interface::Create_Configuration,
		.stop                 = Interface::Stop                ,
	};
	Data data;

	namespace Interface
	{
		void Create_Configuration(struct wl_client* client, struct wl_resource* resource, uint32_t id, uint32_t serial)
		{
			Log::Function::Called("Protocols::WLR::Output_Manager::Interface");
			Output_Configuration::Create(client, 1, id);
		}

		void Stop(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WLR::Output_Manager::Interface");
			Output_Manager::data.sendDisplays[resource] = false;
		}
	}
	
	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zwlr_output_manager_v1_interface, 1, data, Bind);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Protocols::WLR::Output_Manager");

		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_manager_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		Output_Manager::data.sendDisplays[resource] = true;

		auto displays = Backend::GetDisplays();
		for (auto display : displays)
		{
			auto head = Head::Create(wl_client, version, 0, display.output);
			zwlr_output_manager_v1_send_head(resource, head);
			Head::SendData(head);

			for (int a = 0; a < WM::Output::Get::NumberOfModes(display.output); a++)
			{
				auto mode = Mode::Create(wl_client, version, 0, display.output, a);
				zwlr_output_head_v1_send_mode(head, mode);
				Mode::SendData(mode);

				if (WM::Output::Get::Mode::Current(display.output, a))
					zwlr_output_head_v1_send_current_mode(head, mode);
			}

			//zwlr_output_head_v1_send_finished(resource);
		}
	}
}

namespace Awning::Protocols::WLR::Head
{
	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
	}

	void SendData(wl_resource* resource)
	{
		auto outputId = data.resource_to_outputId[resource];

		auto [mX, mY] = WM::Output::Get::Size        (outputId);
		auto [pX, pY] = WM::Output::Get::Position    (outputId);
		auto manuf    = WM::Output::Get::Manufacturer(outputId);
		auto model    = WM::Output::Get::Model       (outputId);

		zwlr_output_head_v1_send_name(resource, model.c_str());
		zwlr_output_head_v1_send_description(resource, manuf.c_str());
		zwlr_output_head_v1_send_position(resource, pX, pY);
		zwlr_output_head_v1_send_enabled(resource, true);
		zwlr_output_head_v1_send_physical_size(resource, mX, mY);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, WM::Output::ID outputId)
	{
		Log::Function::Called("Protocols::WLR::Head");
		
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_head_v1_interface, version, 0);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, nullptr, nullptr, Destroy);

		Head::data.resource_to_outputId[resource] = outputId;
		Head::data.outputId_to_resource[outputId].emplace(resource);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::WLR::Head");

		if (!data.resource_to_outputId.contains(resource))
			return;

		auto id = data.resource_to_outputId[resource];
		data.outputId_to_resource[id].erase(resource);
		data.resource_to_outputId    .erase(resource);
	}
}

namespace Awning::Protocols::WLR::Mode
{
	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
	}

	void SendData(wl_resource* resource)
	{
		auto outputId = data.resource_to_outputId[resource];
		auto mode     = data.resource_to_mode    [resource];

		auto [mX, mY] = WM::Output::Get::Mode::Resolution (outputId, mode);
		auto refresh  = WM::Output::Get::Mode::RefreshRate(outputId, mode);

		zwlr_output_mode_v1_send_size     (resource, mX, mY  );
		zwlr_output_mode_v1_send_refresh  (resource, refresh );

		if (WM::Output::Get::Mode::Prefered(outputId, mode))
			zwlr_output_mode_v1_send_preferred(resource);

		//zwlr_output_mode_v1_send_finished(resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, WM::Output::ID outputId, int mode)
	{
		Log::Function::Called("Protocols::WLR::Mode");
		
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_output_mode_v1_interface, version, 0);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, nullptr, nullptr, Destroy);

		Mode::data.resource_to_outputId[resource] = outputId;
		Mode::data.outputId_to_resource[outputId].emplace(resource);
		Mode::data.resource_to_mode    [resource] = mode;

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::WLR::Mode");

		if (!data.resource_to_outputId.contains(resource))
			return;

		auto id = data.resource_to_outputId[resource];
		data.outputId_to_resource[id].erase(resource);
		data.resource_to_outputId    .erase(resource);
		data.resource_to_mode        .erase(resource);
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

	Data data;

	namespace Interface
	{
		void Enable_Head(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* head)
		{
			Log::Function::Called("Protocols::WLR::Output_Configuration::Interface");
		}

		void Disable_Head(struct wl_client* client, struct wl_resource* resource, struct wl_resource* head)
		{
			Log::Function::Called("Protocols::WLR::Output_Configuration::Interface");
		}

		void Apply(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WLR::Output_Configuration::Interface");
		}

		void Test(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WLR::Output_Configuration::Interface");
		}

		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WLR::Output_Configuration::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Protocols::WLR::Output_Configuration");
		
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
		Log::Function::Called("Protocols::WLR::Output_Configuration");
	}
}