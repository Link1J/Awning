#pragma once
#include "wm/texture.hpp"
#include <wayland-server.h>

namespace Awning::Renderers
{
	enum class API
	{
		NONE, Software, OpenGL_ES_2,
	};

	namespace Functions
	{
		typedef void (*Draw     )(                                                              );
		typedef void (*EGLImage )(wl_resource  * buffer, WM::Texture* texture, WM::Damage damage);
		typedef void (*SHMBuffer)(wl_shm_buffer* buffer, WM::Texture* texture, WM::Damage damage);
	};

	void Init(API renderer);

	extern Functions::Draw Draw;

	namespace FillTextureFrom
	{
		extern Functions::SHMBuffer SHMBuffer;
		extern Functions::EGLImage  EGLImage ;
	}
}