/**************************************************************************
 Copyright:
      (C) 2003 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_STATIC_ASSERT_H
#define HZ_STATIC_ASSERT_H

#include "hz_config.h"  // feature macros


/**
\file
Note: Please specify the hz namespace explicitly (i.e. hz::static_assertion<>)
to avoid conflicts with various other implementations; Better yet,
use HZ_STATIC_ASSERT macro.
*/


namespace hz {


	/// Static assertion - similar to C++11's std::static_assert, but for C++98.
	template<bool B>
	struct static_assertion;

	template<>
	struct static_assertion<true>
	{ };


	// Use this when making an explicit static_assertion<false>.
	// Without T, Intel C++ explicitly instantiates the static_assertion type.
	// Usage: HZ_STATIC_ASSERT(hz::static_false<T>::value, cannot_do_this);
	// where T is an instantiation-dependent type.
	template<typename T>
	struct static_false
	{
		static const bool value = false;
	};


}  // ns


/// Use this macro for static checks with messages.
/// \c msg_identifier doesn't have to be defined as a C++ identifier,
/// it's just a message that will appear in compiler error output.
#define HZ_STATIC_ASSERT(cond, msg_identifier) \
	if (true) { \
		hz::static_assertion<((cond) != 0)> ERROR_##msg_identifier; \
		(void)ERROR_##msg_identifier; \
	} else (void)0


// Maybe use typedef?


// Alternate version:

/*
namespace hz {
	template<bool B>
	struct STATIC_ASSERTION_FAILURE;

	template<>
	struct STATIC_ASSERTION_FAILURE<true>
	{
		enum { value = 1 };
	};

	template<int N>
	struct static_assertion_test
	{ };
}  // ns

#define HZ_STATIC_ASSERT(cond) \
	typedef hz::static_assertion_test<sizeof(hz::STATIC_ASSERTION_FAILURE<(bool)(cond)>)> \
			hz_static_assertion_ ## __LINE__
*/





#endif

/// @}
