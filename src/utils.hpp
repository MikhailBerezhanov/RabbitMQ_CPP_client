#pragma once

#include <string>

// Helpful Utilities

namespace utils
{
	template<typename Iter>
	std::string join(Iter begin, Iter end, const std::string &separator = "")
	{
		std::string res;

		while(begin != end){
			res.append( *begin );

			if(begin != (end - 1)){
				res.append(separator);
			}

			++begin;
		}

		return res;
	}


}