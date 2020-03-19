#pragma once
#include "wm/texture.hpp"
#include <wayland-server.h>

namespace Awning::Renderers
{
	enum class API
	{
		NONE, Software, OpenGLES2,
	};

	namespace Functions
	{
		typedef void (*Draw       )(                                                                      );
		typedef void (*EGLImage   )(wl_resource  * buffer, Awning::Texture* texture, Awning::Damage damage);
		typedef void (*SHMBuffer  )(wl_shm_buffer* buffer, Awning::Texture* texture, Awning::Damage damage);
		typedef void (*LinuxDMABuf)(wl_resource  * buffer, Awning::Texture* texture, Awning::Damage damage);
	};

	void Init(API renderer);

	extern Functions::Draw Draw;

	namespace FillTextureFrom
	{
		extern Functions::SHMBuffer   SHMBuffer  ;
		extern Functions::EGLImage    EGLImage   ;
		extern Functions::LinuxDMABuf LinuxDMABuf;
	}
}