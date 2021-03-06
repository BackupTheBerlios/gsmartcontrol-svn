/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DEXCEPT_H
#define LIBDEBUG_DEXCEPT_H

#include <cstddef>  // std::size_t
#include <cstring>  // std::strncpy / std::strlen
#include <exception>  // std::exception



// this is thrown on internal errors

struct debug_internal_error : virtual public std::exception {  // from <exception>

	debug_internal_error(const char* msg)
	{
		std::size_t buf_len = std::strlen(msg) + 1;
		msg_ = std::strncpy(new char[buf_len], msg, buf_len);
	}

	virtual ~debug_internal_error() throw()
	{
		delete[] msg_;
		msg_ = 0;  // protect from double-deletion compiler bugs
	}

	// Note: messages in exceptions are not newline-terminated.
 	virtual const char* what() const throw()
	{
		return msg_;
	}

	private:
		char* msg_;
};




// this is thrown on usage errors

struct debug_usage_error : virtual public std::exception {  // from <exception>

	debug_usage_error(const char* msg)
	{
		std::size_t buf_len = std::strlen(msg) + 1;
		msg_ = std::strncpy(new char[buf_len], msg, buf_len);
	}

	virtual ~debug_usage_error() throw()
	{
		delete[] msg_;
		msg_ = 0;  // protect from double-deletion compiler bugs
	}

	// Note: messages in exceptions are not newline-terminated.
 	virtual const char* what() const throw()
	{
		return msg_;
	}

	private:
		char* msg_;
};






#endif
