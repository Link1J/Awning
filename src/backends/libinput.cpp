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

#include <libinput.h>
#include <libudev.h>

#include "libinput.hpp"
#include "manager.hpp"


#include "wm/client.hpp"
#include "wm/input.hpp"

static struct { double x; double y; } cursor;
static struct libinput* li;
static struct udev* udev;

static std::unordered_map<std::string     ,                                                 Awning::Input::Seat   > seats  ;
static std::unordered_map<libinput_device*, std::unordered_map<Awning::Input::Device::Type, Awning::Input::Device>> devices;

static int open_restricted(const char *path, int flags, void *user_data)
{
        int fd = open(path, flags);
        return fd < 0 ? -errno : fd;
}
 
static void close_restricted(int fd, void *user_data)
{
        close(fd);
}
 
const static struct libinput_interface interface = {
        .open_restricted = open_restricted,
        .close_restricted = close_restricted,
};

void libinputLog(libinput* libinput, libinput_log_priority priority, const char* format, va_list args)
{
    const int MESSAGE_MAX_SIZE = 200;
	std::string message; message.resize(MESSAGE_MAX_SIZE);
	vsnprintf((char*)message.c_str(), MESSAGE_MAX_SIZE, format, args);

	if (priority == LIBINPUT_LOG_PRIORITY_ERROR) spdlog::error   (message);
	if (priority == LIBINPUT_LOG_PRIORITY_INFO ) spdlog::info    (message);
	if (priority == LIBINPUT_LOG_PRIORITY_DEBUG) spdlog::debug   (message);
}
 
void Awning::Backend::libinput::Start()
{
	udev = udev_new();
	li = libinput_udev_create_context(&interface, NULL, udev);
    libinput_log_set_handler(li, libinputLog);
    libinput_udev_assign_seat(li, "seat0");
    seats["seat0"] = Awning::Input::Seat("seat0");
}

void Awning::Backend::libinput::Hand()
{
    libinput_event* eventBase;
	libinput_dispatch(li);
 
    while ((eventBase = libinput_get_event(li)) != NULL) 
    {
        // handle the event here

		//auto event = libinput_event_get_keyboard_event(eventBase);
		//auto event = libinput_event_get_pointer_event(eventBase);
		//auto event = libinput_event_get_switch_event(eventBase);
		//auto event = libinput_event_get_tablet_pad_event(eventBase);
		//auto event = libinput_event_get_tablet_tool_event(eventBase);
		//auto event = libinput_event_get_touch_event(eventBase);

        switch (libinput_event_get_type(eventBase))
        {
        #define CASE(T) case T: printf(#T"\n"); break

        case LIBINPUT_EVENT_NONE:
        {
            break;
        }
        case LIBINPUT_EVENT_DEVICE_ADDED:
        {
            auto device = libinput_event_get_device(eventBase);

            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD   ))
                devices[device][Input::Device::Type::Keyboard] = Input::Device(Input::Device::Type::Keyboard, libinput_device_get_name(device));
            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER    ))
                devices[device][Input::Device::Type::Mouse   ] = Input::Device(Input::Device::Type::Mouse   , libinput_device_get_name(device));
            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH      ))
                devices[device][Input::Device::Type::Touch   ] = Input::Device(Input::Device::Type::Touch   , libinput_device_get_name(device));
            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TABLET_TOOL))
                devices[device][Input::Device::Type::Tablet  ] = Input::Device(Input::Device::Type::Tablet  , libinput_device_get_name(device));
            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TABLET_PAD ))
                devices[device][Input::Device::Type::Tablet  ] = Input::Device(Input::Device::Type::Tablet  , libinput_device_get_name(device));
            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_GESTURE    ))
                devices[device][Input::Device::Type::Gesture ] = Input::Device(Input::Device::Type::Gesture , libinput_device_get_name(device));
            if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_SWITCH     ))
                devices[device][Input::Device::Type::Switch  ] = Input::Device(Input::Device::Type::Switch  , libinput_device_get_name(device));

            for (auto& [type, deviceTemp] : devices[device])
              seats["seat0"].AddDevice(deviceTemp);

            break;
        }
        case LIBINPUT_EVENT_DEVICE_REMOVED:
        {
            auto device = libinput_event_get_device(eventBase);
            for (auto& [type, deviceTemp] : devices[device])
              seats["seat0"].RemoveDevice(deviceTemp);
            break;
        }
        case LIBINPUT_EVENT_KEYBOARD_KEY:
        {
            auto event  = libinput_event_get_keyboard_event    (eventBase);
            auto device = libinput_event_get_device            (eventBase);
            auto key    = libinput_event_keyboard_get_key      (event    );
            auto state  = libinput_event_keyboard_get_key_state(event    );

            seats["seat0"].Button(devices[device][Input::Device::Type::Keyboard], key, state);
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION:
        {
            auto event  = libinput_event_get_pointer_event(eventBase);
            auto device = libinput_event_get_device       (eventBase);
	        auto x      = libinput_event_pointer_get_dx   (event    );
            auto y      = libinput_event_pointer_get_dy   (event    );

            seats["seat0"].Moved(devices[device][Input::Device::Type::Mouse], x, y);
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        {
            auto event  = libinput_event_get_pointer_event(eventBase);
            auto device = libinput_event_get_device       (eventBase);
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON:
        {
            auto event  = libinput_event_get_pointer_event       (eventBase);
            auto device = libinput_event_get_device              (eventBase);
            auto button = libinput_event_pointer_get_button      (event    );
            auto state  = libinput_event_pointer_get_button_state(event    );

            seats["seat0"].Button(devices[device][Input::Device::Type::Mouse], button, state);
            break;
        }
        case LIBINPUT_EVENT_POINTER_AXIS:
        {
            auto event  = libinput_event_get_pointer_event(eventBase);
            auto device = libinput_event_get_device       (eventBase);

            libinput_pointer_axis axises[] = {LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL};

            for (auto axis : axises)
            {
                if (!libinput_event_pointer_has_axis(event, axis))
                    continue;

                double step = libinput_event_pointer_get_axis_value(event, axis);
                seats["seat0"].Axis(devices[device][Input::Device::Type::Mouse], axis, step);
            }
            break;
        }
        case LIBINPUT_EVENT_TOUCH_DOWN:
        {
            break;
        }
        case LIBINPUT_EVENT_TOUCH_UP:
        {
            break;
        }
        case LIBINPUT_EVENT_TOUCH_MOTION:
        {
            break;
        }
        case LIBINPUT_EVENT_TOUCH_CANCEL:
        {
            break;
        }
        case LIBINPUT_EVENT_TOUCH_FRAME:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_TOOL_AXIS:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_TOOL_PROXIMITY:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_TOOL_TIP:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_TOOL_BUTTON:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_PAD_BUTTON:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_PAD_RING:
        {
            break;
        }
        case LIBINPUT_EVENT_TABLET_PAD_STRIP:
        {
            break;
        }
        //case LIBINPUT_EVENT_TABLET_PAD_KEY:
        //{
        //    break;
        //}
        case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        {
            break;
        }
        case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        {
            break;
        }
        case LIBINPUT_EVENT_GESTURE_SWIPE_END:
        {
            break;
        }
        case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
        {
            break;
        }
        case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
        {
            break;
        }
        case LIBINPUT_EVENT_GESTURE_PINCH_END:
        {
            break;
        }
        case LIBINPUT_EVENT_SWITCH_TOGGLE:
        {
            break;
        }
        } 
 
        libinput_event_destroy(eventBase);
        libinput_dispatch(li);
    }
}