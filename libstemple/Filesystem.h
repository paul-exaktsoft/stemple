// Filesystem
//
// Copyright � 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple_Filesystem__
#define __stemple_Filesystem__

#if !defined __APPLE__

#include <experimental/filesystem>

namespace std
{
	// We assume filesystem will eventually be included in C++1z, so import its
	// definitions into the std namespace for forward compatibility...
	// NOTE: see http://stackoverflow.com/questions/9864125/c11-how-to-alias-a-function for great ideas on aliasing functions
	using path = std::experimental::filesystem::path;
	using std::experimental::filesystem::canonical;
	using std::experimental::filesystem::current_path;
}

#else

// Xcode doesn't have <experimental/filesystem>, so here we implement only the
// functionality that we actually use. Define it in the std namespace for
// forward compatibility, since we assume filesystem will eventually be included
// in C++1z.

#define _DARWIN_BETTER_REALPATH

#include <string>
#include <unistd.h>
#include <stdlib.h>

namespace std
{
	class path
	{
	public:
		path ()
		{
		}

		path (const string &source)
		{
			pathname = source;
		}

		std::string string () const
		{
			return pathname;
		}

		path &remove_filename ()
		{
			auto slash = pathname.find_last_of("/\\");
			if (slash != std::string::npos) {
				while (slash > 0 && (pathname[slash - 1] == '/' || pathname[slash - 1] == '\\')) -- slash;
				if (slash == 0) slash = 1;	// "/xyz" should return "/"
				pathname.resize(slash);
			}
			return *this;
		}
	private:
		std::string pathname;
	};

	inline path current_path ()
	{
		char *buf = getcwd(nullptr, 0);
		std::string cwd(buf);
		free(buf);
		return cwd;
	}

	inline path canonical (const path &p, const path &base = current_path())
	{
		char *buf;
		if (std::string("/\\").find(p.string()[0])  == std::string::npos) {
			std::string abs = base.string() + '/' + p.string();
			buf = realpath(abs.c_str(), nullptr);
		} else {
			buf = realpath(p.string().c_str(), nullptr);
		}
		std::string pathname(buf);
		free(buf);
		return pathname;
	}
}

#endif	// !__APPLE__

#endif	// __stemple_Filesystem__
