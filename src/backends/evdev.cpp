#include "protocols/wl/pointer.hpp"
#include "protocols/wl/keyboard.hpp"

#include <spdlog/spdlog.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <chrono>
#include <thread>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#include "evdev.hpp"
#include "manager.hpp"


#include "wm/client.hpp"

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

static int inotify_fd = -1;
static int inotify_wd = -1;
static std::unordered_map<std::string, int> events;
static bool scrolling = false, mouseMoved;
static struct { int x; int y; } cursor;

void Awning::Backend::EVDEV::Start()
{
	inotify_fd = inotify_init();
	inotify_wd = inotify_add_watch(inotify_fd, "/dev/input", IN_CREATE|IN_DELETE);
	fcntl(inotify_fd, F_SETFL, fcntl(inotify_fd, F_GETFL)|O_NONBLOCK);

	std::string path = "/dev/input";
    for (const auto & entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_character_file())
		{
			auto name = entry.path().filename().string();
			auto path = entry.path()           .string();

			if (name.find("event") != std::string::npos)
			{
				events[name] = open(path.c_str(), O_RDONLY|O_NONBLOCK);
				if (events[name] == -1)
				{
					int error = errno;
					printf("(%s) %d: %s\n", name.c_str(), error, strerror(error));
				}
			}
		}
	}
}

void UpdateDevices()
{
	char buffer[EVENT_BUF_LEN];
	int length = read(inotify_fd, buffer, EVENT_BUF_LEN);
	int i = 0;
	while (i < length) 
	{
		 struct inotify_event* event = (struct inotify_event*)&buffer[i];
		 if (event->len) 
		 {
			std::string name = (char*)event->name;
			std::string path = "/dev/input/" + name;

			if (name.find("event") != std::string::npos)
			{
				if (event->mask & IN_CREATE) 
				{
					events[name] = open(path.c_str(), O_RDONLY|O_NONBLOCK);
				}
				else if (event->mask & IN_DELETE) 
				{
					close(events[name]);
					events.erase(name);
				}
				
			}
		}
		i += EVENT_SIZE + event->len;
	}
}

void Awning::Backend::EVDEV::Hand()
{
	UpdateDevices();

	for (auto&[name, fd] : events)
	{
		input_event event;
		int length = 0;

		for(;;) 
		{
			length = read(fd, &event, sizeof(event));

			if (length == -1)
			{
				int error = errno;
				//printf("(%s) %d: %s\n", name.c_str(), error, strerror(error));
				break;
			}

			if (length == 0) break;

			std::string event_types[] = {
				"EV_SYN",
				"EV_KEY",
				"EV_REL",
				"EV_ABS",
				"EV_MSC",
				"EV_SW",
				"EV_LED",
				"EV_SND",
				"EV_REP",
				"EV_FF",
				"EV_PWR",
				"EV_FF_STATUS",
				"EV_MAX",
				"EV_CNT",
			};

			if (event.type == EV_REL)
			{
				if (!scrolling)
				{
					auto [width, height] = Backend::Size(Backend::GetDisplays());

					if (event.code == 0)
					{
						cursor.x += (int)event.value;
						if (cursor.x < 0)
							cursor.x = 0;
						if (cursor.x > width)
							cursor.x = width;
					}
					else 
					{
						cursor.y += (int)event.value;
						if (cursor.y < 0)
							cursor.y = 0;
						if (cursor.y > height)
							cursor.y = height;
					}
					mouseMoved = true;
				}
				else
				{
					//using namespace WM::Manager::Handle::Input;
					//Mouse::Scroll(event.code, event.value);
				}
			}
			else if (event.type == EV_KEY)
			{
				if (event.code == BTN_MIDDLE)
				{
					scrolling = (int)event.value;
				}

				if (event.code >= BTN_MISC)
				{
					//using namespace WM::Manager::Handle::Input;

					//if (event.value == 1)
					//	Mouse::Pressed(event.code);
					//else
					//	Mouse::Released(event.code);
				}
				else
				{
					//using namespace WM::Manager::Handle::Input;

					//if (event.value == 1)
					//	Keyboard::Pressed(event.code);
					//else
					//	Keyboard::Released(event.code);
				}
			}

			//std::cout << name << ": " << event_types[event.type] << " " << event.code << " " << (int)event.value << "\n";
		}
	}

	if (mouseMoved)
	{
		//WM::Manager::Handle::Input::Mouse::Moved(cursor.x, cursor.y);
		mouseMoved = false;
	}
}