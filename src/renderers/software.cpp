#include "software.hpp"

#include "backends/manager.hpp"

#include "wm/manager.hpp"

#include "wayland/pointer.hpp"

#include <string.h>

template <typename T>
struct reversion_wrapper { T& iterable; };
template <typename T>
auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }
template <typename T>
auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }
template <typename T>
reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

namespace Awning::Renderers::Software
{
	void Init()
	{

	}

	void Draw()
	{
		auto data = Awning::Backend::Data();
		auto list = Awning::WM::Manager::Window::Get();

		memset(data.buffer.u8, 0xEE, data.size);

		for (auto& window : reverse(list))
		{
			auto texture = window->Texture();

			if (!window->Mapped())
				continue;
			if (!texture)
				continue;

			auto winPosX  = window->XPos   ();
			auto winPosY  = window->YPos   ();
			auto winSizeX = window->XSize  ();
			auto winSizeY = window->YSize  ();
			auto winOffX  = window->XOffset();
			auto winOffY  = window->YOffset();

			for (int x = -Frame::Size::left; x < winSizeX + winOffX + Frame::Size::right; x++)
				for (int y = -Frame::Size::top; y < winSizeY + winOffY + Frame::Size::bottom; y++)
				{
					if ((winPosX + x - winOffX) <  0          )
						continue;
					if ((winPosY + y - winOffY) <  0          )
						continue;
					if ((winPosX + x - winOffX) >= data.width )
						continue;
					if ((winPosY + y - winOffY) >= data.height)
						continue;

					int windowOffset = (x) * (texture->bitsPerPixel / 8)
									 + (y) *  texture->bytesPerLine    ;

					int framebOffset = (winPosX + x - winOffX) * (data.bitsPerPixel / 8)
									 + (winPosY + y - winOffY) *  data.bytesPerLine    ;

					uint8_t red, green, blue, alpha;

					if (x < winSizeX + winOffX && y < winSizeY + winOffY && x >= 0 && y >= 0)
					{
						if (texture->buffer.u8 != nullptr && windowOffset < texture->size)
						{
							red   = texture->buffer.u8[windowOffset + (texture->red  .offset / 8)];
							green = texture->buffer.u8[windowOffset + (texture->green.offset / 8)];
							blue  = texture->buffer.u8[windowOffset + (texture->blue .offset / 8)];
							alpha = texture->buffer.u8[windowOffset + (texture->alpha.offset / 8)];
						}
						else
						{
							red   = 0x00;
							green = 0x00;
							blue  = 0x00;
							alpha = 0xFF;
						}						
					}
					else
					{
						if (window->Frame())
						{
							red   = 0x00;
							green = 0xFF;
							blue  = 0xFF;
							alpha = 0xFF;
						}
						else 
						{
							red   = 0x00;
							green = 0x00;
							blue  = 0x00;
							alpha = 0x00;
						}
					}

					if (alpha > 0)
					{
						uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
						uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
						uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

						buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
						buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
						buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
					}
				}
		}

		if (Awning::Wayland::Pointer::data.window)
		{
			auto texture = Awning::Wayland::Pointer::data.window->Texture();

			if (texture)
			{
				auto winPosX  = Awning::Wayland::Pointer::data.window->XPos   ();
				auto winPosY  = Awning::Wayland::Pointer::data.window->YPos   ();
				auto winSizeX = Awning::Wayland::Pointer::data.window->XSize  ();
				auto winSizeY = Awning::Wayland::Pointer::data.window->YSize  ();
				auto winOffX  = Awning::Wayland::Pointer::data.window->XOffset();
				auto winOffY  = Awning::Wayland::Pointer::data.window->YOffset();

				for (int x = 0; x < winSizeX; x++)
					for (int y = 0; y < winSizeY; y++)
					{
						if ((winPosX + x) <  0          )
							continue;
						if ((winPosY + y) <  0          )
							continue;
						if ((winPosX + x) >= data.width )
							continue;
						if ((winPosY + y) >= data.height)
							continue;

						int windowOffset = (x) * (texture->bitsPerPixel / 8)
										 + (y) *  texture->bytesPerLine    ;

						int framebOffset = (winPosX + x - winOffX) * (data.bitsPerPixel / 8)
										 + (winPosY + y - winOffY) *  data.bytesPerLine    ;

						uint8_t red, green, blue, alpha;

						if (texture->buffer.u8 != nullptr)
						{
							red   = texture->buffer.u8[windowOffset + (texture->red  .offset / 8)];
							green = texture->buffer.u8[windowOffset + (texture->green.offset / 8)];
							blue  = texture->buffer.u8[windowOffset + (texture->blue .offset / 8)];
							alpha = texture->buffer.u8[windowOffset + (texture->alpha.offset / 8)];
						}
						else
						{
							red   = 0x00;
							green = 0xFF;
							blue  = 0x00;
							alpha = 0xFF;
						}

						if (alpha > 0 && framebOffset > 0 && framebOffset < data.size)
						{
							uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
							uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
							uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

							buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
							buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
							buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
						}
					}
			}
			else
			{
				auto winPosX  = Awning::Wayland::Pointer::data.window->XPos   ();
				auto winPosY  = Awning::Wayland::Pointer::data.window->YPos   ();
				auto winSizeX = 3                                               ;
				auto winSizeY = 3                                               ;
				auto winOffX  = 0                                               ;
				auto winOffY  = 0                                               ;

				for (int x = 0; x < winSizeX; x++)
					for (int y = 0; y < winSizeY; y++)
					{
						if ((winPosX + x - winOffX) <  0          )
							continue;
						if ((winPosY + y - winOffY) <  0          )
							continue;
						if ((winPosX + x - winOffX) >= data.width )
							continue;
						if ((winPosY + y - winOffY) >= data.height)
							continue;


						int framebOffset = (winPosX + x) * (data.bitsPerPixel / 8)
										 + (winPosY + y) *  data.bytesPerLine    ;

						uint8_t red, green, blue, alpha;

						red   = 0x00;
						green = 0xFF;
						blue  = 0x00;
						alpha = 0xFF;

						uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
						uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
						uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

						buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
						buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
						buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
					}
			}			
		}
	}
}