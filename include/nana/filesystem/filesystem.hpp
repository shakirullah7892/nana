/*
 *	A filesystem Implementation
 *	Copyright(C) 2003 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: stdex/filesystem/filesystem.hpp
 *	@description:
 *		file_iterator is a toolkit for applying each file and directory in a
 *	specified path.
 */

// http://en.cppreference.com/w/cpp/experimental/fs
// http://cpprocks.com/introduction-to-tr2-filesystem-library-in-vs2012/  --- TR2 filesystem in VS2012
// https://msdn.microsoft.com/en-us/library/hh874694%28v=vs.140%29.aspx   ---  C++ 14, the <filesystem> header VS2015
// https://msdn.microsoft.com/en-us/library/hh874694%28v=vs.120%29.aspx   --- <filesystem> header VS2013
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf      --- last pdf of std draft N4100    2014-07-04
// http://cplusplus.github.io/filesystem-ts/working-draft.html            --- in html format
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4099.html     --- in html format
// http://article.gmane.org/gmane.comp.lib.boost.devel/256220             --- The filesystem TS unanimously approved by ISO.
// http://theboostcpplibraries.com/boost.filesystem                       --- Boost docs
// http://www.boost.org/doc/libs/1_58_0/libs/filesystem/doc/index.htm     --- 
// http://www.boost.org/doc/libs/1_34_0/libs/filesystem/doc/index.htm
// http://www.boost.org/doc/libs/1_58_0/boost/filesystem.hpp 
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.200x --- Table 1.4. g++ C++ Technical Specifications Implementation Status

#ifndef NANA_FILESYSTEM_HPP
#define NANA_FILESYSTEM_HPP
#include <iterator>
#include <memory>

#include <nana/deploy.hpp>

#ifdef NANA_WINDOWS
	#include <windows.h>
	typedef HANDLE find_handle_t;
#elif defined(NANA_LINUX)
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <dirent.h>
	typedef DIR* find_handle_t;
#endif

 // namespace std { namespace experimental { namespace filesystem { inline namespace v1 {

namespace nana
{
namespace filesystem
{
    enum class file_type 
    { 
        none = 0,   ///< has not been determined or an error occurred while trying to determine
        not_found = -1, ///< Pseudo-type: file was not found. Is not considered an error
        regular = 1,
        directory = 2  ,
        symlink =3, ///< Symbolic link file
        block =4,  ///< Block special file
        character= 5 ,  ///< Character special file
        fifo = 6 ,  ///< FIFO or pipe file
        socket =7,
        unknown= 8  ///< The file does exist, but is of an operating system dependent type not covered by any of the other
    };
    
    enum class error  	{	none = 0 		}; 

	struct attribute
	{
		long long bytes;
		bool is_directory;
		tm modified;
	};

	bool file_attrib(const nana::string& file, attribute&);
	long long filesize(const nana::string& file);

	bool mkdir(const nana::string& dir, bool & if_exist);
	bool modified_file_time(const nana::string& file, struct tm&);

	nana::string path_user();
	nana::string path_current();

	bool rmfile(const nana::char_t* file);
	bool rmdir(const nana::char_t* dir, bool fails_if_not_empty);
	nana::string root(const nana::string& path);
    
    /// concerned only with lexical and syntactic aspects and does not necessarily exist in
    /// external storage, and the pathname is not necessarily valid for the current operating system 
    /// or for a particular file system 
    /// A sequence of elements that identify the location of a file within a filesystem. 
    /// The elements are the:
    /// rootname (opt), root-directory (opt), and an optional sequence of filenames. 
    /// The maximum number of elements in the sequence is operating system dependent.
	class path
	{
	public:
		path();
		path(const nana::string&);

		bool empty() const;
		path root() const;
		file_type what() const;

		nana::string name() const;
	private:
#if defined(NANA_WINDOWS)
		nana::string text_;
#else
		std::string text_;
#endif
	};

	struct directory_entry
	{
		path m_path;
		unsigned long size;
		bool directory;

		directory_entry();
		directory_entry(const nana::string& filename, bool is_directory, unsigned long size)
            :m_path{filename}, size{size}, directory{is_directory}
        {}
        operator const path&() const noexcept;
        const path& path() const noexcept{return m_path;}

	};


    /// an iterator for a sequence of directory_entry elements representing the files in a directory, not an recursive_directory_iterator
	//template<typename FileInfo>
	class directory_iterator 		:public std::iterator<std::input_iterator_tag, directory_entry>
	{
	public:
		using value_type = directory_entry ;

		directory_iterator():end_(true), handle_(nullptr){}

		directory_iterator(const nana::string& file_path)
			:end_(false), handle_(nullptr)
		{
			_m_prepare(file_path);
		}

		const value_type&
		operator*() const { return value_; }

		const value_type*
		operator->() const { return &(operator*()); }

		directory_iterator& operator++()
		{ _m_read(); return *this; }

		directory_iterator operator++(int)
		{
			directory_iterator tmp = *this;
			_m_read();
			return tmp;
		}

		bool equal(const directory_iterator& x) const
		{
			if(end_ && (end_ == x.end_)) return true;
			return (value_.path().name() == x.value_.path().name());
		}
	private:
		template<typename Char>
		static bool _m_ignore(const Char * p)
		{
			while(*p == '.')
				++p;
			return (*p == 0);
		}

		void _m_prepare(const nana::string& file_path)
		{
		#if defined(NANA_WINDOWS)
			path_ = file_path;
			auto pat = file_path;
			DWORD attr = ::GetFileAttributes(pat.data());
			if((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY))
				pat += STR("\\*");

			::HANDLE handle = ::FindFirstFile(pat.data(), &wfd_);

			if(handle == INVALID_HANDLE_VALUE)
			{
				end_ = true;
				return;
			}

			while(_m_ignore(wfd_.cFileName))
			{
				if(::FindNextFile(handle, &wfd_) == 0)
				{
					end_ = true;
					::FindClose(handle);
					return;
				}
			}

			value_ = value_type(wfd_.cFileName, 
                               (FILE_ATTRIBUTE_DIRECTORY & wfd_.dwFileAttributes) == FILE_ATTRIBUTE_DIRECTORY,
                                wfd_.nFileSizeLow);

		#elif defined(NANA_LINUX)
			path_ = nana::charset(file_path);
			if(path_.size() && (path_[path_.size() - 1] != '/'))
				path_ += '/';
			find_handle_t handle = opendir(path_.c_str());
			end_ = true;
			if(handle)
			{
				struct dirent * dnt = readdir(handle);
				if(dnt)
				{
					while(_m_ignore(dnt->d_name))
					{
						dnt = readdir(handle);
						if(dnt == 0)
						{
							closedir(handle);
							return;
						}
					}

					struct stat fst;
					if(stat((path_ + dnt->d_name).c_str(), &fst) == 0)
					{
						value_ = value_type(nana::charset(dnt->d_name), 0 != S_ISDIR(fst.st_mode), fst.st_size);
					}
					else
					{
						value_.name = nana::charset(dnt->d_name);
						value_.size = 0;
						value_.directory = false;
					}
					end_ = false;
				}
			}
		#endif
			if(false == end_)
			{
				find_ptr_ = std::shared_ptr<find_handle_t>(new find_handle_t(handle), inner_handle_deleter());
				handle_ = handle;
			}
		}

		void _m_read()
		{
			if(handle_)
			{
			#if defined(NANA_WINDOWS)
				if(::FindNextFile(handle_, &wfd_) != 0)
				{
					while(_m_ignore(wfd_.cFileName))
					{
						if(::FindNextFile(handle_, &wfd_) == 0)
						{
							end_ = true;
							return;
						}
					}
			        value_ = value_type(wfd_.cFileName, 
                               (FILE_ATTRIBUTE_DIRECTORY & wfd_.dwFileAttributes) == FILE_ATTRIBUTE_DIRECTORY,
                                wfd_.nFileSizeLow);
				}
				else
					end_ = true;
			#elif defined(NANA_LINUX)
				struct dirent * dnt = readdir(handle_);
				if(dnt)
				{
					while(_m_ignore(dnt->d_name))
					{
						dnt = readdir(handle_);
						if(dnt == 0)
						{
							end_ = true;
							return;
						}
					}
					struct stat fst;
					if(stat((path_ + "/" + dnt->d_name).c_str(), &fst) == 0)
			            value_ = value_type(wfd_.cFileName, 
                               (FILE_ATTRIBUTE_DIRECTORY & wfd_.dwFileAttributes) == FILE_ATTRIBUTE_DIRECTORY,
                                wfd_.nFileSizeLow);
					else
						value_.name = nana::charset(dnt->d_name);
				}
				else
					end_ = true;
			#endif
			}
		}
	private:
		struct inner_handle_deleter
		{
			void operator()(find_handle_t * handle)
			{
				if(handle && *handle)
				{
  				#if defined(NANA_WINDOWS)
					::FindClose(*handle);
            	#elif defined(NANA_LINUX)
					::closedir(*handle);
				#endif
				}
				delete handle;
			}
		};
	private:
		bool	end_;

#if defined(NANA_WINDOWS)
		WIN32_FIND_DATA		wfd_;
		nana::string	path_;
#elif defined(NANA_LINUX)
		std::string	path_;
#endif
		std::shared_ptr<find_handle_t> find_ptr_;

		find_handle_t	handle_;
		value_type	value_;
	};

	//template<typename Value_Type>
	inline bool operator==(const directory_iterator/*<Value_Type>*/ & x, const directory_iterator/*<Value_Type>*/ & y)
	{
	   return x.equal(y);
	}

	//template<typename Value_Type>
	inline bool operator!=(const directory_iterator/*<Value_Type>*/ & x, const directory_iterator/*<Value_Type>*/ & y)
	{
	   return !x.equal(y);
	}

	//using directory_iterator = directory_iterator<directory_entry> ;
}//end namespace filesystem
}//end namespace nana

#endif
