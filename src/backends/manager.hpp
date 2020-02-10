#pragma once
#include <vector>
#include <string>
#include "wm/texture.hpp"

namespace Awning::Backend
{
	enum class API
	{
		NONE, X11, FBDEV, EVDEV, libinput
	};

	namespace Functions
	{
		typedef void                     (*Poll)();
		typedef void                     (*Draw)();
		typedef void                     (*Hand)();
		typedef Awning::WM::Texture::Data(*Data)();
	};

	struct Output
	{
		struct Size
		{
			int width, height;
		};
		struct Mode
		{
			Size resolution;
			int refresh_rate;
			bool prefered;
			bool current;
		};

		std::string manufacturer;
		std::string model;
		Size physical;
		std::vector<Mode> modes;
	};

	void Init(API output, API input);

	extern Functions::Poll Poll;
	extern Functions::Draw Draw;
	extern Functions::Data Data;
	extern Functions::Hand Hand;

	namespace Outputs
	{
		std::vector<Output> Get();
		void Add   (       Output output);
		void Update(int a, Output output);
	}
}