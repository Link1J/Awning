#include "output.hpp"
#include <unordered_map>
#include <vector>

#include <wayland-server.h>
#include <EGL/egl.h>

#include "protocols/wl/output.hpp"

struct Size
{
	int width, height;
};

struct Position
{
	int x, y;
};

struct Mode
{
	Size resolution;
	int refresh_rate;
	bool prefered;
	bool current;
};

struct Output
{
	std::string       manufacturer;
	std::string       model       ;
	Size              physical    ;
	std::vector<Mode> modes       ;
	Position          position    ;
};

namespace Awning
{
	namespace Server
	{
		struct Data
		{
			wl_display* display;
			wl_event_loop* event_loop;
			wl_event_source* sigusr1;
			wl_protocol_logger* logger; 
			wl_listener client_listener;

			struct {
				EGLDisplay display;
				EGLint major, minor;
				EGLContext context;
				EGLSurface surface;
			} egl;
		};
		extern Data data;
	}
};

namespace Awning::WM::Output
{
	std::unordered_map<::Output*, wl_global*> globals;

	ID Create()
	{
		::Output* output = new ::Output();
		globals[output] = Protocols::WL::Output::Add(Server::data.display, output);
		return (ID)output;
	}

	void Destory(ID output)
	{
		wl_global_destroy(globals[(::Output*)output]);
		globals.erase((::Output*)output);
		delete (::Output*)output;
	}

	namespace Set
	{
		void NumberOfModes(ID id, int number)
		{
			::Output* output = (::Output*)id;
			output->modes.resize(number);
		}

		void Model(ID id, std::string model)
		{
			::Output* output = (::Output*)id;
			output->model = model;
		}

		void Manufacturer(ID id, std::string manufacturer)
		{
			::Output* output = (::Output*)id;
			output->manufacturer = manufacturer;
		}

		void Size(ID id, int width, int height)
		{
			::Output* output = (::Output*)id;
			output->physical.width  = width ;
			output->physical.height = height;
		}

		void Position(ID id, int x, int y)
		{
			::Output* output = (::Output*)id;
			output->position.x = x;
			output->position.y = y;
		}

		namespace Mode
		{
			void Resolution(ID id, int mode, int width, int height)
			{
				::Output* output = (::Output*)id;
				output->modes[mode].resolution.width  = width ;
				output->modes[mode].resolution.height = height;
			}

			void RefreshRate(ID id, int mode, int refreshRate)
			{
				::Output* output = (::Output*)id;
				output->modes[mode].refresh_rate = refreshRate;
			}

			void Prefered(ID id, int mode, bool prefered)
			{
				::Output* output = (::Output*)id;
				output->modes[mode].prefered = prefered;
			}

			void Current(ID id, int mode, bool current)
			{
				::Output* output = (::Output*)id;
				output->modes[mode].current = current;
			}
		}
	}

	namespace Get
	{
		int NumberOfModes(ID id)
		{
			::Output* output = (::Output*)id;
			return output->modes.size();
		}

		std::string Model(ID id)
		{
			::Output* output = (::Output*)id;
			return output->model;
		}

		std::string Manufacturer(ID id)
		{
			::Output* output = (::Output*)id;
			return output->manufacturer;
		}

		std::tuple<int,int> Size(ID id)
		{
			::Output* output = (::Output*)id;
			return {
				output->physical.width,
				output->physical.height
			};
		}

		std::tuple<int,int> Position(ID id)
		{
			::Output* output = (::Output*)id;
			return {
				output->position.x,
				output->position.y
			};
		}

		namespace Mode
		{
			std::tuple<int,int> Resolution(ID id, int mode)
			{
				::Output* output = (::Output*)id;
				return {
					output->modes[mode].resolution.width,
					output->modes[mode].resolution.height
				};
			}

			int RefreshRate(ID id, int mode)
			{
				::Output* output = (::Output*)id;
				return output->modes[mode].refresh_rate;
			}

			bool Prefered(ID id, int mode)
			{
				::Output* output = (::Output*)id;
				return output->modes[mode].prefered;
			}

			bool Current(ID id, int mode)
			{
				::Output* output = (::Output*)id;
				return output->modes[mode].current;
			}
		}
	}
}