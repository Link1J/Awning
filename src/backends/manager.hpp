#pragma once
#include <vector>
#include <string>
#include <ljh/function_pointer.hpp>
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
		int        mode   ;
	};
	typedef std::vector<Display> Displays;

	namespace Functions
	{
		using Poll        = ljh::function_pointer<void    ()>;
		using Draw        = ljh::function_pointer<void    ()>;
		using Hand        = ljh::function_pointer<void    ()>;
		using GetDisplays = ljh::function_pointer<Displays()>;
		using Cleanup     = ljh::function_pointer<void    ()>;
	};

	void                 Init(API output, API input);
	std::tuple<int, int> Size(Displays displays    );

	extern Functions::Poll        Poll       ;
	extern Functions::Draw        Draw       ;
	extern Functions::Hand        Hand       ;
	extern Functions::GetDisplays GetDisplays;
	extern Functions::Cleanup     Cleanup    ;
}