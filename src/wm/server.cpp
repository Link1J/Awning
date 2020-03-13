#include "server.hpp"
#include <spdlog/spdlog.h>
#include "client.hpp"

namespace Awning::Server
{
	Global global;

	void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
	void ClientCreated(struct wl_listener *listener, void *data);

	void Init()
	{
		global.display = wl_display_create(); 
		global.socketname = wl_display_add_socket_auto(global.display);
		global.client_listener.notify = ClientCreated;
		global.event_loop = wl_display_get_event_loop(global.display);

		wl_display_add_client_created_listener(global.display, &global.client_listener);
		wl_display_add_protocol_logger(global.display, ProtocolLogger, nullptr);

		spdlog::info("Wayland Socket: {}", global.socketname);
	}

	void ClientCreated(struct wl_listener* listener, void* data)
	{
		Awning::Client::Create(data);
	}

	void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message)
	{
		const char* direction_strings[] = { 
			"REQUEST", 
			"EVENT  "
		};
	
		//spdlog::debug("[{}] {}: {}", direction_strings[direction], message->resource->object.interface->name, message->message->name);
	}
};
