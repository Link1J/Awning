#pragma once
#include <vector>
#include <string>
#include "wm/texture.hpp"

namespace Awning::Backend
{
	enum class API
	{
		NONE, X11, FBDEV,
	};

	namespace Functions
	{
		typedef void                     (*Poll)();
		typedef void                     (*Draw)();
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

	void Init(API api);

	extern Functions::Poll   Poll  ;
	extern Functions::Draw   Draw  ;
	extern Functions::Data   Data  ;

	namespace Outputs
	{
		std::vector<Output> Get();
		void Add   (       Output output);
		void Update(int a, Output output);
	}
}