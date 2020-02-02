#include "log.hpp"
#include <iostream>

namespace Log
{
	using std::experimental::source_location;
 
	namespace Function 
	{
		void Called(std::string from, source_location function)
		{
			std::cout << "[CALLED ] " << from << "::" << function.function_name() << "\n";
		}

		void Locate(std::string from, std::string location, source_location function)
		{
			std::cout << "[LOCATE ] " << from << "::" << function.function_name() << "." << location << "\n";
		}
	}
}