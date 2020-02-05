#include "wm.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#include <unordered_map>

#include "log.hpp"
#include "../window.hpp"

static Display*	display = nullptr;
static Window	root;
static Atom		surfaceID;

std::unordered_map<Window, Awning::WM::Window*> windows;

namespace Awning::WM::X
{
	wl_client* xWaylandClient = nullptr;

	void Init()
	{
		display = XOpenDisplay(":1");
		root = DefaultRootWindow(display);
		XSelectInput(display, root, SubstructureRedirectMask|SubstructureNotifyMask);
    	XCompositeRedirectSubwindows(display, root, CompositeRedirectAutomatic);
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

			/*switch (e.type) {
				#define PRINT_EVENT(type) case type: printf("[X11 EVN]" #type "\n"); break
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
			}*/

			switch (e.type) {
				case CreateNotify:
					Log::Function::Locate("WM::X", "CreateNotify");
					windows[e.xcreatewindow.window] = Window::Create(xWaylandClient);
					windows[e.xcreatewindow.window]->Frame(true);
					break;
				case DestroyNotify:
					Log::Function::Locate("WM::X", "DestroyNotify");
					if (windows[e.xdestroywindow.window]->Texture())
					{
						delete windows[e.xdestroywindow.window]->Texture();
						windows[e.xdestroywindow.window]->Texture(nullptr);
					}
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
					WM::Manager::Window::Raise(windows[e.xmaprequest.window]);
        			break;
				case ConfigureRequest:
					{
						Log::Function::Locate("WM::X", "ConfigureRequest");

  						changes.x = e.xconfigurerequest.x;
  						changes.y = e.xconfigurerequest.y;
  						changes.width = e.xconfigurerequest.width;
  						changes.height = e.xconfigurerequest.height;
  						changes.border_width = e.xconfigurerequest.border_width;
  						changes.sibling = e.xconfigurerequest.above;
  						changes.stack_mode = e.xconfigurerequest.detail;

						XConfigureWindow(display, e.xconfigurerequest.window, e.xconfigurerequest.value_mask, &changes);

						/*XWindowAttributes attr;
						XGetWindowAttributes(display, e.xconfigurerequest.window, &attr);

						if (!windows[e.xconfigurerequest.window]->Texture())
							windows[e.xconfigurerequest.window]->Texture(new WM::Texture::Data());

						auto window = windows[e.xconfigurerequest.window];
						auto texture = window->Texture();

						XRenderPictFormat* format = XRenderFindVisualFormat(display, attr.visual);
						bool hasAlpha             = (format->type == PictTypeDirect && format->direct.alphaMask);
						int x                     = attr.x;
						int y                     = attr.y;
						int width                 = attr.width;
						int height                = attr.height;

						XRenderPictureAttributes pa;
						pa.subwindow_mode = IncludeInferiors; // Don't clip child widgets

						Picture picture = XRenderCreatePicture(display, e.xconfigurerequest.window, format, CPSubwindowMode, &pa);
						XImage* image = XGetImage(display, e.xconfigurerequest.window, 0, 0, width, height, AllPlanes, ZPixmap);

						texture->buffer.u8 = (uint8_t*)image->data;
						texture->width  = image->width ;
						texture->height = image->height;
						texture->bitsPerPixel = image->bits_per_pixel;
						texture->bytesPerLine = image->bytes_per_line;
						texture->size = texture->bytesPerLine * texture->height;
						texture->red   = { .size = 8, .offset = 16 };
						texture->green = { .size = 8, .offset =  8 };
						texture->blue  = { .size = 8, .offset =  0 };
						texture->alpha = { .size = 8, .offset = 24 };*/
					}
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