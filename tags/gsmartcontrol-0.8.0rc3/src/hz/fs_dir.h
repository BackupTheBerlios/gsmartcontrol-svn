/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_DIR_H
#define HZ_FS_DIR_H

#include "hz_config.h"  // feature macros

#include <string>
#include <sys/types.h>  // dirent needs this
#include <dirent.h>

#include "local_algo.h"  // shell_sort
#include "string_wcmatch.h"  // string_wcmatch

#include "fs_path.h"  // Path


// Filesystem directory access


namespace hz {



// -------------------------------------- Iterators


class Dir;

namespace internal {

	// Notes:
	// There is no const version of this because of technical limitations.
	// All iterators to one Dir point to the same entry.

	// Note: Don't forget that there are "." and ".." entries.
	// Use is_special() to verify.

	class DirIterator {
		public:

			typedef void iterator_category;
			typedef std::string value_type;
			typedef int difference_type;
			typedef void pointer;
			typedef value_type reference;

			typedef struct dirent* handle_type;


			DirIterator() : dir_(NULL), entry_(NULL)
			{ }

			DirIterator(Dir* dir, struct dirent* entry) : dir_(dir), entry_(entry)
			{ }

			DirIterator(const DirIterator& other)
			{
				dir_ = other.dir_;
				entry_ = other.entry_;
			}

			DirIterator& operator= (const DirIterator& other)
			{
				dir_ = other.dir_;
				entry_ = other.entry_;
				return *this;
			}


			bool operator== (const DirIterator& other) const
			{
				return entry_ == other.entry_;
			}

			bool operator!= (const DirIterator& other) const
			{
				return !(*this == other);
			}


			// Advance to next entry
			inline DirIterator& operator++ ();  // prefix

			// Advance to next entry
			inline void operator++ (int);  // postfix. returns void because of technical limitations.

			// Same as name()
			inline std::string operator* () const;  // dereference

			// Provides operations on return value of name()
			inline std::string operator-> () const;


			// Get entry name
			inline std::string name() const;

			// Get full path
			inline std::string path() const;

			// Returns true if it's "." or "..".
			inline bool is_special() const;

			// Get native handle
			inline handle_type get_handle();


		private:

			Dir* dir_;
			struct dirent* entry_;

	};


}  // ns




// -------------------------------------- Sorting and filtering functors


// ----------------- Filtering


// No filtering - leave all entries.

struct DirFilterNone {
	bool operator() (const std::string& dir, const std::string& entry_name)
	{
		return true;  // no filtering
	}
};



// Flag-based filtering

enum {  // flags
	// these may be ORed
	DIR_LEAVE_FILE = 1 << 0,
	DIR_LEAVE_DIR = 1 << 1,
	DIR_LEAVE_REGULAR = 1 << 2,
	DIR_LEAVE_SYMLINK = 1 << 3,
	DIR_LEAVE_ALL = DIR_LEAVE_FILE | DIR_LEAVE_DIR  // any entry is either file or directory
};


// Note: If an error occurs while checking flags, the entry is filtered out.
struct DirFilterByFlags {

	DirFilterByFlags(int flags) : flags_(flags)
	{ }

	bool operator() (const std::string& dir, const std::string& entry_name)
	{
		FsPath p(dir);
		p.append(entry_name);

		if ((flags_ & DIR_LEAVE_FILE) && p.is_file())
			return true;
		if ((flags_ & DIR_LEAVE_DIR) && p.is_dir())
			return true;
		if ((flags_ & DIR_LEAVE_REGULAR) && p.is_regular())
			return true;
		if ((flags_ & DIR_LEAVE_SYMLINK) && p.is_symlink())
			return true;

		return false;
	}

	private:
		int flags_;
};



// glob (aka wildcard: ?, *, []) filtering.

struct DirFilterWc {

	DirFilterWc(const std::string pattern) : pattern_(pattern)
	{ }

	bool operator() (const std::string& dir, const std::string& entry_name)
	{
		return hz::string_wcmatch(pattern_, entry_name);
	}

	private:
		std::string pattern_;
};




// ----------------- Sorting


enum {  // flags
	DIR_SORT_DIRS_FIRST,
	DIR_SORT_FILES_FIRST,
	DIR_SORT_MIXED
};


// No sorting
struct DirSortNone {

	// this is called before the less function
	void set_dir(const std::string& dir)
	{ }

	// "less" function
	bool operator() (const std::string& entry_name1, const std::string& entry_name2)
	{
		return true;  // always return "less", which should cause no sorting.
	}
};



// Base class for various sorters
template<class Child>
struct DirSortBase {
	DirSortBase(int flag) : flag_(flag)
	{ }

	// this is called before the less function
	void set_dir(const std::string& dir)
	{
		dir_ = dir;
	}

	// "less" function
	bool operator() (const std::string& entry_name1, const std::string& entry_name2)
	{
		if (flag_ == DIR_SORT_MIXED)
			return static_cast<Child*>(this)->compare(entry_name1, entry_name2);

		FsPath p1(dir_), p2(dir_);
		p1.append(entry_name1);
		p2.append(entry_name2);

		bool e1_dir = p1.is_dir(), e2_dir = p2.is_dir();

		if (e1_dir == e2_dir)  // same type
			return static_cast<Child*>(this)->compare(entry_name1, entry_name2);

		if (flag_ == DIR_SORT_DIRS_FIRST)
			return e1_dir;  // (e1 < e2) if e1 is dir

		if (flag_ == DIR_SORT_FILES_FIRST)
			return !e1_dir;  // (e1 < e2) if e1 is file

		return true;  // won't reach this
	}

	protected:
		int flag_;
		std::string dir_;
};


// Alphanumeric sorting
struct DirSortAlpha : public DirSortBase<DirSortAlpha> {

	DirSortAlpha(int flag = DIR_SORT_DIRS_FIRST) : DirSortBase<DirSortAlpha>(flag)
	{ }

	// "less" function (called by parent)
	bool compare(const std::string& entry_name1, const std::string& entry_name2)
	{
		return entry_name1 < entry_name2;
	}

};


// Timestamp (descending order) sorting
struct DirSortMTime : public DirSortBase<DirSortMTime> {

	DirSortMTime(int flag = DIR_SORT_DIRS_FIRST) : DirSortBase<DirSortMTime>(flag)
	{ }

	// "less" function (called by parent)
	bool compare(const std::string& entry_name1, const std::string& entry_name2)
	{
		FsPath p1(dir_), p2(dir_);
		p1.append(entry_name1);
		p2.append(entry_name2);

		time_t e1_ts = 0, e2_ts = 0;
		if (!p1.get_last_modified(e1_ts) || !p2.get_last_modified(e2_ts) || (e1_ts == e2_ts))
			entry_name1 < entry_name2;  // error or similar timestamps, fall back to this.

		return e1_ts < e2_ts;
	}

};





// -------------------------------------- Main Dir class


// This class represents a directory which is opened in
// constructor and closed in destructor.
class Dir : public FsPath {
	public:

		typedef internal::DirIterator iterator;

		typedef DIR* handle_type;
		typedef struct dirent* entry_handle_type;


		// Create a Dir object. This will NOT open the directory.
		Dir() : dir_(NULL), entry_(NULL)
		{ }

		// Create a Dir object. This will NOT open the directory.
		Dir(const FsPath& path) : dir_(NULL), entry_(NULL)
		{
			set_path(path.get_path());
		}

		// Create a Dir object. This will NOT open the directory.
		Dir(const std::string& path) : dir_(NULL), entry_(NULL)
		{
			set_path(path);
		}


		// Copy constructor. Move semantics are implemented -
		// the handle ownership is transferred exclusively.
		Dir(Dir& other) : dir_(NULL), entry_(NULL)
		{
			*this = other;  // operator=
		}

		// Move semantics, as with copy constructor.
		inline Dir& operator= (Dir& other);


		// Destructor which invokes close() if needed.
		~Dir()
		{
			if (dir_)
				close();
		}


		// Open the directory. The path must be already set.
		inline bool open();

		// Open the directory with path "path".
		inline bool open(const std::string& path);

		// Close the directory manually (automatically invoked in destructor).
		inline bool close();

		// Get native handle of a directory
		inline handle_type get_handle();


		// ------------ directory entry functions

		// Read the next entry. Returns false when the end is reached
		// or if the directory is not open. If entry read error occurs, _true_
		// is returned - to check for error, see bad().
		// This function will open the directory if needed.
		inline bool entry_next();

		// Rewind the entry pointer to the beginning, so that entry_next()
		// will read the first entry.
		// If the directory was changed while open, this should re-read it.
		// This function will open the directory if needed.
		inline bool entry_reset();

		// Get the name of current entry
		inline std::string entry_get_name();

		// Get full path of current entry
		inline std::string entry_get_path();

		// Get native handle of a directory entry
		inline entry_handle_type entry_get_handle();


		// ------------- entry iterator functions

		// Returns an iterator pointing to the first entry.
		// This function will open the directory if needed.
		iterator begin()  // this resets the position!
		{
			entry_reset();
			entry_next();  // set to first entry
			return iterator(this, entry_);
		}

		// Returns an iterator pointing to the entry one past the last one.
		iterator end()
		{
			return iterator(this, NULL);
		}


		// -------------- entry listing

		// Put directory entries into container. Each entry will be filtered through
		// filter_func. The final list will be sorted using sort_func.
		// Container must be a random-access container supporting push_back() method.
		// put_with_path indicates whether the Dir's path should be prepended to
		// entry name before putting it into the container.
		// This function will open the directory if needed.
		template<class Container, class SortFunctor, class FilterFunctor> inline
		bool entry_list(Container& put_here, bool put_with_path, SortFunctor sort_func, FilterFunctor filter_func);

		// Same as above, but defaulting to no filtering.
		template<class Container, class SortFunctor>
		bool entry_list(Container& put_here, bool put_with_path, SortFunctor sort_func)
		{
			return entry_list(put_here, put_with_path, sort_func, DirFilterNone());
		}

		// Same as above, but with default sorting.
		template<class Container, class FilterFunctor>
		bool entry_list_filtered(Container& put_here, bool put_with_path, FilterFunctor filter_func)
		{
			return entry_list(put_here, put_with_path, DirSortAlpha(DIR_SORT_DIRS_FIRST), filter_func);
		}

		// Same as above, but defaulting to no filtering and alphanumeric sort (dirs first).
		template<class Container>
		bool entry_list(Container& put_here, bool put_with_path = false)
		{
			return entry_list(put_here, put_with_path, DirSortAlpha(DIR_SORT_DIRS_FIRST), DirFilterNone());
		}


	private:

		Dir(const Dir& other);  // don't allow it. allow only from non-const.

		const Dir& operator=(const Dir& other);  // don't allow it. allow only from non-const.


		handle_type dir_;  // DIR handle
		entry_handle_type entry_;  // current entry, dirent*

};





// ------------------------------------------- Implementation


namespace internal {


	inline DirIterator& DirIterator::operator++ ()  // prefix
	{
		if (dir_) {
			dir_->entry_next();
			entry_ = dir_->entry_get_handle();
		} else {
			entry_ = NULL;
		}
		return *this;
	}

	inline void DirIterator::operator++ (int)  // postfix. returns void because of technical limitations.
	{
		++(*this);
	}

	inline std::string DirIterator::operator* () const  // dereference
	{
		return name();
	}

	inline std::string DirIterator::operator-> () const
	{
		return name();
	}


	inline std::string DirIterator::name() const
	{
		if (!entry_ || !dir_)
			return std::string();
		return dir_->entry_get_name();
	}

	inline std::string DirIterator::path() const
	{
		if (!entry_ || !dir_)
			return std::string();
		return dir_->entry_get_path();
	}

	inline bool DirIterator::is_special() const
	{
		std::string n = name();
		return (n == "." || n == "..");
	}

	inline DirIterator::handle_type DirIterator::get_handle()
	{
		if (!entry_ || !dir_)
			return NULL;
		return dir_->entry_get_handle();
	}


}  // ns



// --------------------------------------------


// move semantics, as with copy constructor.
inline Dir& Dir::operator= (Dir& other)
{
	dir_ = other.dir_;
	entry_ = other.entry_;

	// clear other's members, so everything is clear.
	other.dir_ = NULL;
	other.entry_ = NULL;
	other.set_path("");
	other.set_error(HZ__("The directory handle ownership was transferred from this object."));

	return *this;
}



inline bool Dir::open()
{
	if (dir_) {  // already open
		set_error(std::string(HZ__("Error while opening directory \"/path1/\": "))
				+ HZ__("Another directory is open already. Close it first."), 0, path_);
		return false;
	}

	if (path_.empty()) {
		set_error(std::string(HZ__("Error while opening directory: ")) + HZ__("Supplied path is empty."));
		return false;
	}

	clear_error();

	dir_ = opendir(path_.c_str());
	if (!dir_) {
		set_error(HZ__("Error while opening directory \"/path1/\": /errno/."), errno, path_);
		return false;
	}

	entry_ = NULL;

	return true;
}



inline bool Dir::open(const std::string& path)
{
	set_path(path);
	return open();
}



inline bool Dir::close()
{
	clear_error();

	if (dir_ && closedir(dir_) != 0) {  // error
		set_error(HZ__("Error while closing directory \"/path1/\": /errno/."), errno, path_);
		dir_ = NULL;
		entry_ = NULL;
		return false;
	}
	dir_ = NULL;
	entry_ = NULL;

	return true;  // even if already closed
}


inline Dir::handle_type Dir::get_handle()
{
	return dir_;
}



// -------------- directory entry functions

inline bool Dir::entry_next()
{
	clear_error();
	entry_ = NULL;  // just in case

	if (!dir_) {  // open if needed
		if (!open())
			return false;
	}

// 	if (!dir_) {
// 		set_error(HZ__("Error while reading directory entry: Directory is not open.");
// 		return false;  // an indicator to stop the caller's loop.
// 	}

	errno = 0;  // reset errno, because readdir may return NULL even if it's OK.
	entry_ = readdir(dir_);

	if (errno != 0) {
		set_error(HZ__("Error while reading directory entry of \"/path1/\": /errno/."), errno, path_);
		return true;  // you may continue reading anyway
	}

	if (entry_)
		return true;
	return false;  // end is reached
}


// Rewind the entry pointer to the beginning, so that entry_next() will read the first entry.
// If the directory was changed while open, this should re-read it.
inline bool Dir::entry_reset()
{
	clear_error();
	entry_ = NULL;  // since it's invalid anyway

	if (!dir_) {  // open if needed
		if (!open())
			return false;
	}

// 	if (!dir_) {
// 		set_error(HZ__("Error while reading directory entry: Directory is not open.");
// 		return false;
// 	}

	rewinddir(dir_);  // no return value here

	return true;
}


// get the name of current entry
inline std::string Dir::entry_get_name()
{
	clear_error();
	if (!dir_) {
		set_error(std::string(HZ__("Error while reading directory entry of \"/path1/\": "))
				+ HZ__("Directory is not open."), 0, path_);
		return std::string();
	}

	if (!entry_) {
		set_error(std::string(HZ__("Error while reading directory entry of \"/path1/\": "))
				+ HZ__("Entry is not set."), 0, path_);
		return std::string();
	}

	return (entry_->d_name ? std::string(entry_->d_name) : std::string());
}


// get full path of current entry
inline std::string Dir::entry_get_path()
{
	return get_path() + DIR_SEPARATOR_S + entry_get_name();
}


// Get native handle of a directory entry
inline Dir::entry_handle_type Dir::entry_get_handle()
{
	return entry_;
}



template<class Container, class SortFunctor, class FilterFunctor> inline
bool Dir::entry_list(Container& put_here, bool put_with_path, SortFunctor sort_func, FilterFunctor filter_func)
{
	clear_error();

	if (!dir_) {  // open if needed
		if (!open())
			return false;
	}

// 	if (!dir_) {
// 		set_error(HZ__("Error while reading directory entry: Directory is not open.");
// 		return false;
// 	}

	entry_reset();

	Container v;
	while (entry_next()) {
		if (bad())
			continue;

		std::string name = entry_get_name();
		if (!bad() && filter_func(path_, name))
			v.push_back(name);
	}

	sort_func.set_dir(path_);
	shell_sort(v.begin(), v.end(), sort_func);

	for(typename Container::const_iterator iter = v.begin(); iter != v.end(); ++iter) {
		if (put_with_path) {
			FsPath p(path_);
			p.append(*iter);
			put_here.push_back(p.str());

		} else {
			put_here.push_back(*iter);
		}
	}

	return true;
}






}  // ns hz



#endif
