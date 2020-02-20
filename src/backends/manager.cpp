#include "manager.hpp"

#include "X11.hpp"
#include "fbdev.hpp"
#include "evdev.hpp"
#include "libinput.hpp"
#include "drm.hpp"

namespace Awning::Backend
{
	Functions::Poll        Poll       ;
	Functions::Draw        Draw       ;
	Functions::Hand        Hand       ;
	Functions::GetDisplays GetDisplays;

	void Init(API output, API input)
	{
		switch (output)
		{
		case API::X11:
			X11::Start();
			Poll        = X11  ::Poll       ;
			Draw        = X11  ::Draw       ;
			GetDisplays = X11  ::GetDisplays;
			break;
		case API::DRM:
			DRM::Start();
			Poll        = DRM  ::Poll       ;
			Draw        = DRM  ::Draw       ;
			GetDisplays = DRM  ::GetDisplays;
			break;
		
		default:
			break;
		}

		switch (input)
		{
		case API::X11:
			Hand = X11::Hand;
			break;
		case API::EVDEV:
			EVDEV::Start();
			Hand = EVDEV::Hand;
			break;
		case API::libinput:
			libinput::Start();
			Hand = libinput::Hand;
			break;
		
		default:
			break;
		}
	}

	std::tuple<int, int> Size(Displays displays)
	{
		int width = 0, height = 0;
		for (auto display : displays)
		{
			auto [px, py] = WM::Output::Get::Position(display.output);
			auto [sx, sy] = WM::Output::Get::Mode::Resolution(display.output, display.mode);

			if (width  < px + sx)
				width  = px + sx;
			if (height < py + sy)
				height = py + sy;
		}
		return {width, height};
	}
}