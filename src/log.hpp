#pragma once
#include <string>
#include <experimental/source_location>

namespace Log
{
	using std::experimental::source_location;
 
	namespace Function 
	{
		void Called(std::string from, source_location function = source_location::current());
	}
}