#include "protocols/wl/pointer.hpp"
#include "protocols/wl/keyboard.hpp"

#include "log.hpp"

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

#include "wm/manager.hpp"
#include "wm/client.hpp"

static struct { double x; double y; } cursor;
static struct libinput* li;
static struct udev* udev;

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
 
void Awning::Backend::libinput::Start()
{
	udev = udev_new();
	li = libinput_udev_create_context(&interface, NULL, udev);
    libinput_udev_assign_seat(li, "seat0");
}

void Awning::Backend::libinput::Hand()
{
    struct libinput_event* eventBase;
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
            break;
        }
        case LIBINPUT_EVENT_DEVICE_REMOVED:
        {
            break;
        }
        case LIBINPUT_EVENT_KEYBOARD_KEY:
        {
            auto event = libinput_event_get_keyboard_event(eventBase);
            auto key = libinput_event_keyboard_get_key(event);

            if (libinput_event_keyboard_get_key_state(event))
                WM::Manager::Handle::Input::Keyboard::Pressed(key);
            else
                WM::Manager::Handle::Input::Keyboard::Released(key);
            
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION:
        {
            auto event = libinput_event_get_pointer_event(eventBase);
	        auto x = libinput_event_pointer_get_dx(event);
            auto y = libinput_event_pointer_get_dy(event);

            auto [width, height] = Backend::Size(Backend::GetDisplays());

            if (cursor.x + x < 0 || cursor.x + x > width )
                x = 0;
            if (cursor.y + y < 0 || cursor.y + y > height)
                y = 0;

            cursor.x += x;
            cursor.y += y;

            WM::Manager::Handle::Input::Mouse::Moved(cursor.x, cursor.y);
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        {
            auto event = libinput_event_get_pointer_event(eventBase);
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON:
        {
            auto event = libinput_event_get_pointer_event(eventBase);
            auto button = libinput_event_pointer_get_button(event);

            if (libinput_event_pointer_get_button_state(event))
                WM::Manager::Handle::Input::Mouse::Pressed(button);
            else
                WM::Manager::Handle::Input::Mouse::Released(button);

            break;
        }
        case LIBINPUT_EVENT_POINTER_AXIS:
        {
            auto event = libinput_event_get_pointer_event(eventBase);

            int vert = libinput_event_pointer_get_axis_value(event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
            WM::Manager::Handle::Input::Mouse::Scroll(LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL, vert / abs(vert) >= 0, abs(vert));

            int horz = libinput_event_pointer_get_axis_value(event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
            WM::Manager::Handle::Input::Mouse::Scroll(LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL, horz / abs(horz) >= 0, abs(horz));
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