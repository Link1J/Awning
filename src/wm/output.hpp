#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <tuple>

#include "functions.hpp"

namespace Awning::Output
{
	typedef uintptr_t ID;

	ID   Create (     );
	void Destory(ID id);

	void AddResize(ID id, Functions::Resized resized, void* data);
	void RemoveResize(ID id, Functions::Resized resized, void* data);
	
	namespace Set
	{
		void NumberOfModes(ID id, int number);

		void Model       (ID id, std::string model       );
		void Manufacturer(ID id, std::string manufacturer);
		void Size        (ID id, int width, int height   );
		void Position    (ID id, int x, int y            );
		void Name        (ID id, std::string name        );
		void Description (ID id, std::string description );
		 
		namespace Mode
		{
			void Resolution (ID id, int mode, int width, int height);
			void RefreshRate(ID id, int mode, int refreshRate      );
			void Prefered   (ID id, int mode, bool prefered        );
			void Current    (ID id, int mode, bool current         );
		}
	}

	namespace Get
	{
		int NumberOfModes(ID id);
		int CurrentMode(ID id);

		std::string         Model       (ID id);
		std::string         Manufacturer(ID id);
		std::tuple<int,int> Size        (ID id);
		std::tuple<int,int> Position    (ID id);
		std::string         Name        (ID id);
		std::string         Description (ID id);

		namespace Mode
		{
			std::tuple<int,int> Resolution (ID id, int mode);
			int                 RefreshRate(ID id, int mode);
			bool                Prefered   (ID id, int mode);
			bool                Current    (ID id, int mode);
		}
	}
}
