#include "manager.hpp"
#include "software.hpp"
#include "gles2.hpp"

namespace Awning::Renderers
{
	Functions::Draw Draw;

	namespace FillTextureFrom
	{
		Functions::SHMBuffer SHMBuffer;
		Functions::EGLImage  EGLImage ;
	}

	void Init(API renderer)
	{
		switch (renderer)
		{
		case API::Software:
			Software::Init();
			                Draw       = Software                 ::Draw     ;
			FillTextureFrom::SHMBuffer = Software::FillTextureFrom::SHMBuffer;
			FillTextureFrom::EGLImage  = Software::FillTextureFrom::EGLImage ;
			break;
		case API::OpenGL_ES_2:
			GLES2::Init();
			                Draw       = GLES2                    ::Draw     ;
			FillTextureFrom::SHMBuffer = GLES2   ::FillTextureFrom::SHMBuffer;
			FillTextureFrom::EGLImage  = GLES2   ::FillTextureFrom::EGLImage ;
			break;
		
		default:
			break;
		}
	}
}