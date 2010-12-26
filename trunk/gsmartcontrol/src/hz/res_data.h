/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_RES_DATA_H
#define HZ_RES_DATA_H

#include "hz_config.h"  // feature macros

#include <stdint.h>
#include <string>

#include "data_file.h"
#include "fs_file.h"


// When loading file resources, this file allows you to choose between
// compiled-in buffers (e.g. generated by file2csource.sh script and linked in),
// and runtime-loaded data files (searched and loaded dynamically at runtime).

// Configuration macros: HZ_ENABLE_COMPILED_RES_DATA (0 | 1).




// Compiled-in buffers:

// Declare that we have a binary chunk res_name.
// (You may use this e.g. with glade xml contents, putting it into your window class).
#define HZ_RES_DATA_COMPILED_INIT_NAMED(res_name, dummy, class_name) \
	struct class_name { \
		class_name() \
		{ \
			extern const unsigned char res_name[]; \
			extern const unsigned int res_name##_size; \
			buf = res_name; \
			size = res_name##_size; \
			root_name = #res_name; \
		} \
		std::string get_string() const \
		{ \
			if (!buf || !size) \
				return std::string(); \
			return std::string(reinterpret_cast<const char*>(buf), size); \
		} \
		const unsigned char* buf; \
		unsigned int size; \
		const char* root_name; \
	}


// Same as above, with the default name as ResData.
#define HZ_RES_DATA_COMPILED_INIT(res_name, dummy) \
	HZ_RES_DATA_COMPILED_INIT_NAMED(res_name, dummy, ResData)




// Use this for auto-loaded data files.
// The generated API is the same as with compiled-in buffers.
// Don't forget to call hz::data_file_add_search_directory() somewhere
// to register the data file search paths.

#define HZ_RES_DATA_LOADED_INIT_NAMED(res_name, data_file, class_name) \
	struct class_name { \
		class_name() : buf(0), size(0), root_name(0) \
		{ \
			std::string path = hz::data_file_find(data_file); \
			if (path.empty()) \
				return; \
			hz::File file(path); \
			hz::file_size_t file_size = 0, loaded_size = 0; \
			if (!file.get_size(file_size)) \
				return; \
			unsigned char* tmpbuf = new unsigned char[file_size + 1]; \
			if (!file.get_contents_noalloc(tmpbuf, file_size + 1, loaded_size)) { \
				delete[] tmpbuf; \
				return; \
			} \
			tmpbuf[file_size] = '\0';  /* add 0-termination, like with compiled-in buffers */ \
			buf = tmpbuf; \
			size = static_cast<unsigned int>(loaded_size); \
			root_name = #res_name; \
		} \
		~class_name() { \
			delete[] const_cast<unsigned char*>(buf); \
			buf = 0; \
		} \
		std::string get_string() const \
		{ \
			if (!buf || !size) \
				return std::string(); \
			return std::string(reinterpret_cast<const char*>(buf), size); \
		} \
		const unsigned char* buf; \
		unsigned int size; \
		const char* root_name; \
	}


#define HZ_RES_DATA_LOADED_INIT(res_name, data_file) \
	HZ_RES_DATA_LOADED_INIT_NAMED(res_name, (data_file), ResData)



// Dummy

#define HZ_RES_DATA_DUMMY_INIT_NAMED(res_name, dummy, class_name) \
	struct class_name { \
		class_name() : buf(0), size(0), root_name(#res_name) \
		{ } \
		std::string get_string() const \
		{ \
			return std::string(); \
		} \
		const unsigned char* buf; \
		unsigned int size; \
		const char* root_name; \
	}

#define HZ_RES_DATA_DUMMY_INIT(res_name, dummy) \
	HZ_RES_DATA_DUMMY_INIT_NAMED(res_name, (dummy), ResData)




// Use these to automatically enable the selected method.

#if HZ_ENABLE_COMPILED_RES_DATA

	#define HZ_RES_DATA_INIT_NAMED(res_name, dummy, class_name) \
			HZ_RES_DATA_COMPILED_INIT_NAMED(res_name, dummy, class_name)

	#define HZ_RES_DATA_INIT(res_name, dummy) \
			HZ_RES_DATA_COMPILED_INIT(res_name, dummy)

#else  // loaded buffers

	#define HZ_RES_DATA_INIT_NAMED(res_name, data_file, class_name) \
			HZ_RES_DATA_LOADED_INIT_NAMED(res_name, data_file, class_name)

	#define HZ_RES_DATA_INIT(res_name, data_file) \
			HZ_RES_DATA_LOADED_INIT(res_name, data_file)

#endif






#endif
