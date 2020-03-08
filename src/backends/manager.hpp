#pragma once
#include <vector>
#include <string>
#include "wm/texture.hpp"
#include "wm/output.hpp"

namespace Awning::Backend
{
	enum class API
	{
		NONE, X11, EVDEV, libinput, DRM
	};

	struct Display
	{
		Output::ID output ;
		Texture    texture;
		int            mode   ;
	};
	typedef std::vector<Display> Displays;

	namespace Functions
	{
		typedef void    (*Poll       )();
		typedef void    (*Draw       )();
		typedef void    (*Hand       )();
		typedef Displays(*GetDisplays)();
	};

	void                 Init(API output, API input);
	std::tuple<int, int> Size(Displays displays    );

	extern Functions::Poll        Poll       ;
	extern Functions::Draw        Draw       ;
	extern Functions::Hand        Hand       ;
	extern Functions::GetDisplays GetDisplays;
}