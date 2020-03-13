#include "input.hpp"

#include <wayland-server.h>

#include <spdlog/spdlog.h>
#include <stdio.h>

#include "protocols/wl/seat.hpp"
#include "backends/manager.hpp"

#include "protocols/wl/pointer.hpp"

#include "frame.hpp"
#include "server.hpp"

void xcbLog(xkb_context* context, xkb_log_level level, const char* format, va_list args)
{
    const int MESSAGE_MAX_SIZE = 200;
	std::string message; message.resize(MESSAGE_MAX_SIZE);
	vsnprintf((char*)message.c_str(), MESSAGE_MAX_SIZE, format, args);

	if (level == XKB_LOG_LEVEL_CRITICAL) spdlog::critical(message);
	if (level == XKB_LOG_LEVEL_ERROR   ) spdlog::error   (message);
	if (level == XKB_LOG_LEVEL_WARNING ) spdlog::warn    (message);
	if (level == XKB_LOG_LEVEL_INFO    ) spdlog::info    (message);
	if (level == XKB_LOG_LEVEL_DEBUG   ) spdlog::debug   (message);
}

namespace Awning::Input
{
	Device::Device(Type type, std::string name)
	{
		this->type = type;
		this->name = name;
	}

	Device::Type Device::GetType()
	{
		return type;
	}

	std::unordered_set<Window*> cursors;
	xkb_context* Seat::ctx;

	Seat::Seat(std::string name)
	{
		xkb_rule_names names = {
		    .rules   = NULL,
		    .model   = NULL,
		    .layout  = NULL,
		    .variant = NULL,
		    .options = NULL
		};

		if (!ctx)
		{
			ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
			xkb_context_set_log_fn(ctx, xcbLog);
		}

		this->name = name;
		global = Awning::Protocols::WL::Seat::Add(Awning::Server::global.display, this);

		pointer.window = Window::Create(0);
		pointer.window->Mapped(true);

		keyboard.keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
		keyboard.state = xkb_state_new(keyboard.keymap);

		cursors.emplace(pointer.window);
	}

	Seat::Seat(const Seat& other)
	{
		this->name       = other.name      ;
		this->devices    = other.devices   ;
		this->global     = other.global    ;
		this->capability = other.capability;

		this->pointer.window = other.pointer.window;

		this->keyboard.keymap = other.keyboard.keymap;
		this->keyboard.state  = other.keyboard.state ;
	}

	Seat& Seat::operator=(const Seat& other)
	{
		this->name       = other.name      ;
		this->devices    = other.devices   ;
		this->global     = other.global    ;
		this->capability = other.capability;

		this->pointer.window = other.pointer.window;

		this->keyboard.keymap = other.keyboard.keymap;
		this->keyboard.state  = other.keyboard.state ;

		return *this;
	}

	Seat& Seat::operator=(Seat&& other)
	{
		this->name       = other.name      ;
		this->devices    = other.devices   ;
		this->global     = other.global    ;
		this->capability = other.capability;

		this->pointer.window = other.pointer.window;

		this->keyboard.keymap = other.keyboard.keymap;
		this->keyboard.state  = other.keyboard.state ;

		wl_global_set_user_data((wl_global*)global, this);

		return *this;
	}

	void Seat::AddDevice(Device& device)
	{
		Device::Type type = device.GetType();
		if (type == Device::Type::Other)
			return;

		devices.push_back(&device);

		switch (type)
		{
		case Device::Type::Keyboard:
			capability = (Seat::Capability)((int)capability | (int)Seat::Capability::Keyboard);
			break;
		case Device::Type::Mouse   :
			capability = (Seat::Capability)((int)capability | (int)Seat::Capability::Mouse   );
			break;
		default:
			break;
		}
	}

	void Seat::RemoveDevice(Device& device)
	{
		auto curr = devices.begin();
		while (curr != devices.end())
		{
			if (*curr == &device)
				break;
			curr++;
		}
		if (curr != devices.end())
		{
			devices.erase(curr);
		}
	}

	bool Seat::HasDevice(Device& device)
	{
		auto curr = devices.begin();
		while (curr != devices.end())
		{
			if (*curr == &device)
				break;
			curr++;
		}
		return curr != devices.end();
	}

	Seat::Capability Seat::Capabilities()
	{
		return capability;
	}

	std::string Seat::Name()
	{
		return name;
	}

	void Seat::Moved(Device& device, double x, double y)
	{
		if (!HasDevice(device))
			return;

		if (device.GetType() == Device::Type::Mouse)
		{
			auto [width, height] = Backend::Size(Backend::GetDisplays());

			pointer.xPos += x;
			if (pointer.xPos < 0     )
				pointer.xPos = 0     ;
			if (pointer.xPos > width )
				pointer.xPos = width ;

			pointer.yPos += y;
			if (pointer.yPos < 0     )
				pointer.yPos = 0     ;
			if (pointer.yPos > height)
				pointer.yPos = height;

			Window::Manager::Move(pointer.window, pointer.xPos, pointer.yPos);

			if (state == State::Normal)
			{
				auto curr = Window::Manager::windowList.begin();
				bool found = false;

				while (curr != Window::Manager::windowList.end() && !found)
				{
					int windowLeft   = (*curr)->XPos() - (*curr)->XOffset()                   ;
					int windowTop    = (*curr)->YPos() - (*curr)->YOffset()                   ;
					int windowRight  = (*curr)->XPos() + (*curr)->XOffset() + (*curr)->XSize();
					int windowButtom = (*curr)->YPos() + (*curr)->YOffset() + (*curr)->YSize();

					if (pointer.xPos >= windowLeft  
					&&  pointer.yPos >= windowTop   
					&&  pointer.xPos <  windowRight 
					&&  pointer.yPos <  windowButtom
					)
					{
						action = Action::Application;
						found = true;
					}

					if ((*curr)->Frame() && !found)
					{
						if (pointer.xPos >= windowLeft   - ::Frame::Move::left  
						&&  pointer.yPos >= windowTop    - ::Frame::Move::top   
						&&  pointer.xPos <  windowRight  + ::Frame::Move::right 
						&&  pointer.yPos <  windowButtom + ::Frame::Move::bottom
						&& !found)
						{
							action = Action::Move;
							found = true;
						}

						if (pointer.xPos >= windowLeft   - ::Frame::Size::left  
						&&  pointer.yPos >= windowTop    - ::Frame::Size::top   
						&&  pointer.xPos <  windowRight  + ::Frame::Size::right 
						&&  pointer.yPos <  windowButtom + ::Frame::Size::bottom
						&& !found)
						{
							action = Action::Resize;
							found = true;
						}

						side = WindowSide::NONE;

						if (action == Action::Resize && found)
						{
							if (pointer.xPos <  windowLeft  ) side = (WindowSide)((int)side | (int)WindowSide::LEFT  );
							if (pointer.yPos <  windowTop   ) side = (WindowSide)((int)side | (int)WindowSide::TOP   );
							if (pointer.xPos >= windowRight ) side = (WindowSide)((int)side | (int)WindowSide::RIGHT );
							if (pointer.yPos >= windowButtom) side = (WindowSide)((int)side | (int)WindowSide::BOTTOM);
						}
					}

					if (!found)
						curr++;
				}
				
				if (action == Action::Application || hovered != *curr)
				{
					if (hovered != *curr && curr != Window::Manager::windowList.end())
					{
						int localX = pointer.xPos - (*curr)->XPos() - (*curr)->XOffset();
						int localY = pointer.yPos - (*curr)->YPos() - (*curr)->YOffset();

						void* surface = Client::Get::Surface(*curr);

						for (auto& functionSet : functions[1][(*curr)->Client()])
							functionSet.enter(functionSet.data, surface, localX, localY);
						changed.emplace(std::tuple<void*,int>{(*curr)->Client(),1});

						hovered = *curr;
						entered = true;
					}
					else if (hovered && entered)
					{		
						int localX = pointer.xPos - hovered->XPos() - hovered->XOffset();
						int localY = pointer.yPos - hovered->YPos() - hovered->YOffset();

						for (auto& functionSet : functions[1][hovered->Client()])
							functionSet.moved(functionSet.data, localX, localY);
						changed.emplace(std::tuple<void*,int>{hovered->Client(),1});
					}
				}
				else if (action != Action::Application)
				{
					if (hovered && entered)
					{
						void* surface = Client::Get::Surface(hovered);

						for (auto& functionSet : functions[1][hovered->Client()])
							functionSet.leave(functionSet.data, surface);
						changed.emplace(std::tuple<void*,int>{hovered->Client(),1});

						entered = false;
					}
				}
			}
			else if (active && state == State::WindowManger)
			{
				if (action == Action::Move)
				{
					int newX = active->XPos() + x;
					int newY = active->YPos() + y;
					Window::Manager::Move(active, newX, newY);
				}
				else if (action == Action::Resize)
				{
					int XSize = active->XSize();
					int YSize = active->YSize();
					int XPos  = active->XPos ();
					int YPos  = active->YPos ();

					if (((int)side & (int)WindowSide::TOP   ) != 0)
					{
						YPos += y;
						YSize -= y;
					}
					if (((int)side & (int)WindowSide::BOTTOM) != 0)
					{
						YSize += y;
					}
					if (((int)side & (int)WindowSide::LEFT  ) != 0)
					{
						XPos += x;
						XSize -= x;
					}
					if (((int)side & (int)WindowSide::RIGHT ) != 0)
					{
						XSize += x;
					}

					int preX = active->XSize();
					int preY = active->YSize();

					Window::Manager::Resize(active, XSize, YSize);

					if (preX == active->XSize())
						XPos =  active->XPos ();
					if (preY == active->YSize())
						YPos =  active->YPos ();

					Window::Manager::Move(hovered, XPos, YPos);
				}
			}
		}
	}

	void Seat::Button(Device& device, int button, bool pressed)
	{
		if (!HasDevice(device))
			return;

		if (device.GetType() == Device::Type::Mouse)
		{
			if (hovered != active)
			{
				if (hovered)
				{
					Window::Manager::Raise(hovered);

					int localX = pointer.xPos - hovered->XPos() - hovered->XOffset();
					int localY = pointer.yPos - hovered->YPos() - hovered->YOffset();

					void* surface = Client::Get::Surface(hovered);

					for (auto& functionSet : functions[1][hovered->Client()])
						functionSet.enter(functionSet.data, surface, localX, localY);
					changed.emplace(std::tuple<void*,int>{hovered->Client(),1});
				}

				if (active)
				{
					void* surface = Client::Get::Surface(active);

					for (auto& functionSet : functions[0][active->Client()])
						functionSet.leave(functionSet.data, surface);
					changed.emplace(std::tuple<void*,int>{active->Client(),0});
				}

				active = hovered;

				if (active)
				{
					void* surface = Client::Get::Surface(active);

					for (auto& functionSet : functions[0][active->Client()])
						functionSet.enter(functionSet.data, surface, 0, 0);
					changed.emplace(std::tuple<void*,int>{active->Client(),0});
				}
			}
			
			if (active && action == Action::Application)
			{
				for (auto& functionSet : functions[1][active->Client()])
					functionSet.button(functionSet.data, button, pressed);
				changed.emplace(std::tuple<void*,int>{active->Client(),1});
			}
			else if ((action == Action::Move || action == Action::Resize) && active)
			{
				if (pressed)
				{
					void* surface = Client::Get::Surface(active);

					for (auto& functionSet : functions[1][active->Client()])
						functionSet.leave(functionSet.data, surface);
					changed.emplace(std::tuple<void*,int>{active->Client(),1});

					state = State::WindowManger;
					pointer.lockButton = button;
				}
				else if (button == pointer.lockButton)
				{
					hovered = active = nullptr;
					state = State::Normal;
					action = Action::Application;
				}
			}
		}

		if (device.GetType() == Device::Type::Keyboard)
		{
			xkb_state_update_key(keyboard.state, button + 8, (xkb_key_direction)pressed);

			if (active)
			{
				void* surface = Client::Get::Surface(active);

				for (auto& functionSet : functions[0][active->Client()])
					functionSet.button(functionSet.data, button, pressed);
				changed.emplace(std::tuple<void*,int>{active->Client(),0});
			}
		}
	}

	void Seat::Axis(Device& device, int axis, double step)
	{
		if (!HasDevice(device))
			return;

		if (device.GetType() == Device::Type::Mouse)
		{
			for (auto& functionSet : functions[1][hovered->Client()])
				functionSet.scroll(functionSet.data, axis, step);
			changed.emplace(std::tuple<void*,int>{hovered->Client(),1});
		}
	}

	void Seat::End()
	{
		if (!hovered)
			return;

		for (auto& [client, type] : changed)
			for (auto& functionSet : functions[type][client])
				if (functionSet.frame != nullptr)
					functionSet.frame(functionSet.data);

		changed.clear();
	}

	void Seat::AddFunctions(int type, void* client, FunctionSet function)
	{
		functions[type][client].push_back(function);
	}
	
	void Seat::RemoveFunctions(int type, void* client, void* data)
	{
		auto curr = functions[type][client].begin();
		while (curr != functions[type][client].end())
		{
			if (curr->data == data)
				break;
			curr++;
		}
		if (curr != functions[type][client].end())
		{
			functions[type][client].erase(curr);
		}
	}
}