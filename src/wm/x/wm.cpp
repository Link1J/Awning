#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>

#include <unordered_map>

#include "log.hpp"
#include "../window.hpp"

static Display*	display = nullptr;
static Window	root;
static Atom		surfaceID;

std::unordered_map<Window, Awning::WM::Window*> windows;

namespace Awning::WM::X
{
	void Init()
	{
		display = XOpenDisplay(":1");
		root = DefaultRootWindow(display);
		XSelectInput(display, root, SubstructureRedirectMask|SubstructureNotifyMask);
		//XCompositeRedirectSubwindows(display, root, CompositeRedirectManual);
		surfaceID = XInternAtom(display, "WL_SURFACE_ID", False);
	}

	void EventLoop()
	{
		if (display == nullptr)
			return;

		while(XPending(display))
		{
			XEvent e;
			XWindowChanges changes;
    		XNextEvent(display, &e);

			switch (e.type) {
				#define PRINT_EVENT(type) case type: printf("[X11EVENT]" #type "\n"); break
				PRINT_EVENT(KeyPress		);
				PRINT_EVENT(KeyRelease		);
				PRINT_EVENT(ButtonPress		);
				PRINT_EVENT(ButtonRelease	);
				PRINT_EVENT(MotionNotify	);
				PRINT_EVENT(EnterNotify		);
				PRINT_EVENT(LeaveNotify		);
				PRINT_EVENT(FocusIn			);
				PRINT_EVENT(FocusOut		);
				PRINT_EVENT(KeymapNotify	);
				PRINT_EVENT(Expose			);
				PRINT_EVENT(GraphicsExpose	);
				PRINT_EVENT(NoExpose		);
				PRINT_EVENT(VisibilityNotify);
				PRINT_EVENT(CreateNotify	);
				PRINT_EVENT(DestroyNotify	);
				PRINT_EVENT(UnmapNotify		);
				PRINT_EVENT(MapNotify		);
				PRINT_EVENT(MapRequest		);
				PRINT_EVENT(ReparentNotify	);
				PRINT_EVENT(ConfigureNotify	);
				PRINT_EVENT(ConfigureRequest);
				PRINT_EVENT(GravityNotify	);
				PRINT_EVENT(ResizeRequest	);
				PRINT_EVENT(CirculateNotify	);
				PRINT_EVENT(CirculateRequest);
				PRINT_EVENT(PropertyNotify	);
				PRINT_EVENT(SelectionClear	);
				PRINT_EVENT(SelectionRequest);
				PRINT_EVENT(SelectionNotify	);
				PRINT_EVENT(ColormapNotify	);
				PRINT_EVENT(ClientMessage	);
				PRINT_EVENT(MappingNotify	);
				PRINT_EVENT(GenericEvent	);
				PRINT_EVENT(LASTEvent		);
				#undef PRINT_EVENT
			}

			switch (e.type) {
				case CreateNotify:
					Log::Function::Locate("WM::X", "CreateNotify");
					//windows[e.xcreatewindow.window] = Window::Create();
					break;
				case DestroyNotify:
					Log::Function::Locate("WM::X", "DestroyNotify");
					Window::Destory(windows[e.xdestroywindow.window]);
					windows.erase(e.xdestroywindow.window);
					break;
				case ReparentNotify:
					Log::Function::Locate("WM::X", "ReparentNotify");
					break;
				case MapRequest:
					Log::Function::Locate("WM::X", "MapRequest");
        			windows[e.xmaprequest.window]->Mapped(true);
					XMapWindow(display, e.xmaprequest.window);
        			break;
				case ConfigureRequest:
					Log::Function::Locate("WM::X", "ConfigureRequest");

  					changes.x = e.xconfigurerequest.x;
  					changes.y = e.xconfigurerequest.y;
  					changes.width = e.xconfigurerequest.width;
  					changes.height = e.xconfigurerequest.height;
  					changes.border_width = e.xconfigurerequest.border_width;
  					changes.sibling = e.xconfigurerequest.above;
  					changes.stack_mode = e.xconfigurerequest.detail;

					XConfigureWindow(display, e.xconfigurerequest.window, e.xconfigurerequest.value_mask, &changes);
        			break;
				case ConfigureNotify:
					Log::Function::Locate("WM::X", "ConfigureNotify");
					break;
				case ClientMessage:
					Log::Function::Locate("WM::X", "ClientMessage");
					if (e.xclient.message_type == surfaceID)
					{
						Log::Function::Locate("WM::X", "ClientMessage.WL_SURFACE_ID");
					}
					break;
			}
		}
	}
}