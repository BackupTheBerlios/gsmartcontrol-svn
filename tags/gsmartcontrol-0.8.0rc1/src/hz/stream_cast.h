/**************************************************************************
 Copyright:
      (C) 2000 - 2008  Kevlin Henney
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_LEXICAL_CAST_H
#define HZ_LEXICAL_CAST_H

#include "hz_config.h"  // feature macros

#include <cstddef>  // std::size_t
#include <string>
#include <sstream>
#include <limits>  // std::numeric_limits

#include "bad_cast_exception.h"
#include "type_properties.h"  // type_is_pointer


/*
stream_cast library, based on boost::lexical_cast.

Convert any type supporting "operator<<" to any other type
supporting "operator>>".

Original notes and copyright info follow:
*/

// what:  lexical_cast custom keyword cast
// who:   contributed by Kevlin Henney,
//        enhanced with contributions from Terje Sletteb�,
//        with additional fixes and suggestions from Gennaro Prota,
//        Beman Dawes, Dave Abrahams, Daryle Walker, Peter Dimov,
//        and other Boosters
// when:  November 2000, March 2003, June 2005

// Copyright Kevlin Henney, 2000-2005. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




namespace hz {



namespace internal {


	template<typename Target, typename Source>
	class lexical_stream {

		public:
			lexical_stream()
			{
				stream.unsetf(std::ios::skipws);  // disable whitespace skipping

				if (std::numeric_limits<Target>::is_specialized) {
					stream.precision(std::numeric_limits<Target>::digits10 + 1);

				} else if (std::numeric_limits<Source>::is_specialized) {
					stream.precision(std::numeric_limits<Source>::digits10 + 1);
				}
			}


			bool operator<<(const Source &input)
			{
				return !(stream << input).fail();
			}


			template<typename InputStreamable>
			bool operator>>(InputStreamable &output)
			{
				return !hz::type_is_pointer<InputStreamable>::value &&
					stream >> output &&
					stream.get() == std::char_traits<std::stringstream::char_type>::eof();
			}


			bool operator>>(std::string &output)
			{
				output = stream.str();
				return true;
			}

		private:
			std::stringstream stream;

	};



	// call-by-const reference version

	template<class T>
	struct array_to_pointer_decay {
		typedef T type;
	};

	template<class T, std::size_t N>
	struct array_to_pointer_decay<T[N]> {
		typedef const T* type;
	};



}  // ns internal




// This is thrown in case of cast failure
DEFINE_BAD_CAST_EXCEPTION(bad_stream_cast,
		"Failed stream_cast from \"%s\" to \"%s\".", "Failed stream_cast.");




template<typename Target, typename Source> inline
Target stream_cast(const Source& arg)
{
	typedef typename internal::array_to_pointer_decay<Source>::type NewSource;

	internal::lexical_stream<Target, NewSource> interpreter;
	Target result;

	if (!(interpreter << arg && interpreter >> result))
		THROW_CUSTOM_BAD_CAST(bad_stream_cast, typeid(NewSource), typeid(Target));

	return result;
}




}  // ns


#endif
