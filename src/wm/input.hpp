#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "window.hpp"
#include "functions.hpp"

#include <xkbcommon/xkbcommon.h>

#undef None

#include <tuple>

namespace std
{
    namespace
    {
        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780
        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v)
        {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
			static void apply(size_t& seed, Tuple const& tuple)
			{
				HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
				hash_combine(seed, std::get<Index>(tuple));
			}
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0>
        {
			static void apply(size_t& seed, Tuple const& tuple)
			{
				hash_combine(seed, std::get<0>(tuple));
			}
        };
    }

    template <typename ... TT>
    struct hash<std::tuple<TT...>> 
    {
        size_t
        operator()(std::tuple<TT...> const& tt) const
        {                                              
            size_t seed = 0;                             
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);    
            return seed;                                 
        }
    };
}

namespace Awning::Input
{
	enum class Action
	{
		Application,
		Resize,
		Move,
		Maximize,
		Minimize,
		Close
	};

	enum State
	{
		Normal,
		WindowManger,
		Grab,
	};

	enum class WindowSide
	{
		NONE         = 0b0000,
		TOP          = 0b1000,
		LEFT         = 0b0010,
		BOTTOM       = 0b0100,
		RIGHT        = 0b0001,
		TOP_LEFT     = 0b1010,
		TOP_RIGHT    = 0b1001,
		BOTTOM_LEFT  = 0b0110,
		BOTTOM_RIGHT = 0b0101,
	};

	class Device
	{
	public:
		enum class Type { Keyboard, Mouse, Tablet, Switch, Touch, Gesture, Other };
	
	private:
		Type type;
		std::string name;

	public:
		Device() = default;
		Device(Type type, std::string name);
		Type GetType();
	};

	extern std::unordered_set<Window*> cursors;

	class Seat
	{
		friend Window;
	public:
		enum class Capability { None = 0, Keyboard = 1, Mouse = 2, Touch = 4 };

		struct Pointer
		{
			Window* window;
			int xPos, yPos;
			int lockButton;
		} pointer;

		struct Keyboard
		{
			xkb_keymap* keymap;
			xkb_state * state;
		} keyboard;
		
		struct FunctionSet
		{
			Functions::Moved  moved ;
			Functions::Button button;
			Functions::Scroll scroll;
			Functions::Enter  enter ;
			Functions::Leave  leave ;
			Functions::Frame  frame ;
			void*             data  ;
		};
	private:

		static xkb_context* ctx;

		std::string name;
		std::vector<Device*> devices;
		void* global;
		Capability capability = Capability::None;

		Action     action;
		WindowSide side  ;
		State      state ;

		Window* hovered        ;
		Window* active         ;
		bool    entered = false;

		std::unordered_map<void*, std::vector<FunctionSet>> functions[3];
		std::unordered_set<std::tuple<void*,int>>           changed     ;

	public:
		Seat(                 ) = default;
		Seat(std::string name )          ;
		Seat(const Seat& other)          ;

		Seat& operator=(const Seat&  other);
		Seat& operator=(      Seat&& other);
		
		void AddDevice   (Device& device);
		bool HasDevice   (Device& device);
		void RemoveDevice(Device& device);

		Capability  Capabilities();
		std::string Name        ();

		void Moved (Device& device, double x     , double y      );
		void Button(Device& device, int    button, bool   pressed);
		void Axis  (Device& device, int    axis  , double step   );
		void End   (                                             );

		void AddFunctions(int type, void* client, FunctionSet function);
		void RemoveFunctions(int type, void* client, void* data);
	};
}