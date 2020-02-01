#include <X11/X.h>
#include <X11/Xlib.h>

#include "log.hpp"

static Display*	display = nullptr;
static Window	root;
static Atom		surfaceID;


namespace Awning::WM::X
{
	void Init()
	{
		display = XOpenDisplay(":1");
		root = DefaultRootWindow(display);
		XSelectInput(display, root, SubstructureRedirectMask|SubstructureNotifyMask);
		surfaceID = XInternAtom(display, "WL_SURFACE_ID", False);
	}

	void EventLoop()
	{
		if (display == nullptr)
			return;

		while(XPending(display))
		{
			XEvent e;
    		XNextEvent(display, &e);
			switch (e.type) {
				case CreateNotify:
					Log::Function::Locate("Awning::WM::X", "CreateNotify");
					//OnCreateNotify(e.xcreatewindow);
					break;
				case DestroyNotify:
					Log::Function::Locate("Awning::WM::X", "DestroyNotify");
					//OnDestroyNotify(e.xdestroywindow);
					break;
				case ReparentNotify:
					Log::Function::Locate("Awning::WM::X", "ReparentNotify");
					//OnReparentNotify(e.xreparent);
					break;
				case ClientMessage:
					Log::Function::Locate("Awning::WM::X", "ClientMessage");
					if (e.xclient.message_type == surfaceID)
					{
						Log::Function::Locate("Awning::WM::X", "ClientMessage.WL_SURFACE_ID");
					}
					break;
			}
		}
	}
}