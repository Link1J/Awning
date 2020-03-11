#pragma once

namespace Awning 
{
	class Frame 
	{
	public:
		struct Size {
			int width, height;
		};
		struct Rect {
			int top, bottom, left, right;
		};
		enum class State {
			Move, Resize, Close, Maximize, Minimized,
		};
	private:
		
	};
}

namespace Frame
{
	namespace Resize 
	{
		int const top    = 4;
		int const bottom = 4;
		int const left   = 4;
		int const right  = 4;
	}
	namespace Move 
	{
		int const top    = 10;
		int const bottom =  0;
		int const left   =  0;
		int const right  =  0;
	}
	namespace Maximized
	{
		int const width  = 10;
		int const height = 10;
	}
	namespace Minimized
	{
		int const width  = 10;
		int const height = 10;
	}
	namespace Close
	{
		int const width  = 10;
		int const height = 10;
	}
	namespace Size 
	{
		int const top    = Resize::top    + Move::top   ;
		int const bottom = Resize::bottom + Move::bottom;
		int const left   = Resize::left   + Move::left  ;
		int const right  = Resize::right  + Move::right ;
	}
}