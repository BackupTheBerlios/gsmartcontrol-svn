/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_ERROR_H
#define HZ_ERROR_H

#include "hz_config.h"  // feature macros

#include <string>
#include <exception>  // for std::exception specialization

#ifndef DISABLE_RTTI
#	include <typeinfo>  // std::type_info
#endif

#if defined ENABLE_GLIBMM
#	include <glibmm/error.h>  // for Glib::Error specialization
#endif

#include "debug.h"  // DBG_ASSERT
#include "errno_string.h"  // hz::errno_string
#include "process_signal.h"  // hz::signal_to_string
#include "bad_cast_exception.h"
#include "i18n.h"  // HZ__



// Compilation options:
// - Define DISABLE_RTTI to disable RTTI checks and typeinfo-getter
// functions. NOT recommended.
// - Define ENABLE_GLIBMM to enable glibmm-related code (mainly
// utf8 string messages and Glib::Error specialization). Note that this
// will also enable GLIB.

// Note that __GXX_RTTI (not used here) is available for gcc >= 4.3.



namespace hz {



/*
Predefined types are: "errno", "signal" (child exited with signal).
*/


struct ErrorLevel {
	enum level_t {
		none = 0,
		dump = 1 << 0,
		info = 1 << 1,  // default
		warn = 1 << 2,
		error = 1 << 3,
		fatal = 1 << 4,
	};
};



template<typename CodeType>
class Error;


class ErrorBase {

	public:

		typedef ErrorLevel::level_t level_t;


		DEFINE_BAD_CAST_EXCEPTION(type_mismatch,
				"Error type mismatch. Original type: \"%s\", requested type: \"%s\".", "Error type mismatch.");


		ErrorBase(const std::string& type_, level_t level_, const std::string& msg)
				: type(type_), level(level_), message(msg)
		{ }

		ErrorBase(const std::string& type_, level_t level_)
				: type(type_), level(level_)
		{ }

		virtual ~ErrorBase()
		{ }

		virtual ErrorBase* clone() = 0;  // needed for copying by base pointers


#ifndef DISABLE_RTTI
		virtual const std::type_info& get_code_type() const = 0;
#endif


		template<class CodeMemberType>
		CodeMemberType get_code() const  // this may throw on bad cast!
		{
#ifndef DISABLE_RTTI
			if (get_code_type() != typeid(CodeMemberType))
				THROW_CUSTOM_BAD_CAST(type_mismatch, get_code_type(), typeid(CodeMemberType));
#endif
			return static_cast<const Error<CodeMemberType>*>(this)->code;
		}


		template<class CodeMemberType>
		bool get_code(CodeMemberType& put_it_here) const  // this doesn't throw
		{
#ifndef DISABLE_RTTI
			if (get_code_type() != typeid(CodeMemberType))
				return false;
#endif
			put_it_here = static_cast<const Error<CodeMemberType>*>(this)->code;
			return true;
		}


		// increase the level (seriousness) of the error
		level_t level_inc()
		{
			if (level == ErrorLevel::fatal)
				return level;
			return (level = static_cast<level_t>(static_cast<int>(level) << 1));
		}

		level_t level_dec()
		{
			if (level == ErrorLevel::none)
				return level;
			return (level = static_cast<level_t>(static_cast<int>(level) >> 1));
		}

		level_t get_level() const
		{
			return level;
		}


		std::string get_type() const
		{
			return type;
		}


		std::string get_message() const
		{
			return message;
		}


		// no set_type, set_message - we don't allow changing those.



	protected:

		std::string type;
		level_t level;
		std::string message;

};





// provide some common stuff for Error to ease template specializations.
template<typename CodeType>
class ErrorCodeHolder : public ErrorBase {
	protected:

		ErrorCodeHolder(const std::string& type_, ErrorLevel::level_t level_, const CodeType& code_,
				const std::string& msg)
			: ErrorBase(type_, level_, msg), code(code_)
		{ }

		ErrorCodeHolder(const std::string& type_, ErrorLevel::level_t level_, const CodeType& code_)
			: ErrorBase(type_, level_), code(code_)
		{ }

	public:

#ifndef DISABLE_RTTI
		const std::type_info& get_code_type() const { return typeid(CodeType); }
#endif

		CodeType code;  // we have a class specialization for references too

};



// specialization for void, helpful for custom messages
template<>
class ErrorCodeHolder<void> : public ErrorBase {
	protected:

		ErrorCodeHolder(const std::string& type_, ErrorLevel::level_t level_, const std::string& msg)
			: ErrorBase(type_, level_, msg)
		{ }

	public:

#ifndef DISABLE_RTTI
		const std::type_info& get_code_type() const { return typeid(void); }
#endif

};






// Error class. Instantiate this in user code.
template<typename CodeType>
struct Error : public ErrorCodeHolder<CodeType> {

	Error(const std::string& type_, ErrorLevel::level_t level_, const CodeType& code_,
			const std::string& msg)
		: ErrorCodeHolder<CodeType>(type_, level_, code_, msg)
	{ }

	ErrorBase* clone()  // needed for copying by base pointers
	{
		return new Error(ErrorCodeHolder<CodeType>::type, ErrorCodeHolder<CodeType>::level,
				ErrorCodeHolder<CodeType>::code, ErrorCodeHolder<CodeType>::message);
	}
};




// Error class specialization for void (helpful for custom messages).
template<>
struct Error<void> : public ErrorCodeHolder<void> {

	Error(const std::string& type_, ErrorLevel::level_t level_, const std::string& msg)
		: ErrorCodeHolder<void>(type_, level_, msg)
	{ }

	ErrorBase* clone()  // needed for copying by base pointers
	{
		return new Error(ErrorCodeHolder<void>::type, ErrorCodeHolder<void>::level,
				ErrorCodeHolder<void>::message);
	}
};




// int specialization for signal, errno; message is auto-evaluated.
template<>
struct Error<int> : public ErrorCodeHolder<int> {

	Error(const std::string& type_, ErrorLevel::level_t level_, int code_, const std::string& msg)
		: ErrorCodeHolder<int>(type_, level_, code_, msg)
	{ }

	Error(const std::string& type_, ErrorLevel::level_t level_, int code_)
		: ErrorCodeHolder<int>(type_, level_, code)
	{
		if (type == "errno") {
			message = hz::errno_string(code_);

		} else if (type == "signal") {
			// hz::signal_string should be translated already
			message = HZ__("Child exited with signal: ") + hz::signal_to_string(code_);

		} else {  // nothing else supported here. use constructor with a message.
			DBG_ASSERT(0);
		}
	}

	ErrorBase* clone()  // needed for copying by base pointers
	{
		return new Error(ErrorCodeHolder<int>::type, ErrorCodeHolder<int>::level,
				ErrorCodeHolder<int>::code, ErrorCodeHolder<int>::message);
	}
};




/*
namespace {

	Error<int> e1("type1", ErrorLevel::info, 6);
	Error<std::string> e2("type2", ErrorLevel::fatal, "asdasd", "aaa");

	std::bad_alloc ex3;
// 	Error<std::exception&> e3("type3", ErrorBase::info, static_cast<std::exception&>(ex3));
	Error<std::exception&> e3("type3", ErrorLevel::info, ex3);


	Glib::IOChannelError ex4(Glib::IOChannelError::FILE_TOO_BIG, "message4");
	Error<Glib::Error&> e4("type4", ErrorLevel::info, ex4);


}
*/



}  // ns





#endif
