#pragma once
#include <string>
#include <experimental/source_location>

namespace Log
{
	using std::experimental::source_location;
 
	namespace Function 
	{
		void Called(std::string from, source_location function = source_location::current());
		void Locate(std::string from, std::string location, source_location function = source_location::current());
	}
	namespace Report 
	{
		void Error(std::string message, source_location function = source_location::current());
		void Info (std::string message, source_location function = source_location::current());
	}
}