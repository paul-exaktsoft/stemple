// Filesystem
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple_Filesystem__
#define __stemple_Filesystem__

#include <experimental/filesystem>

namespace std
{
	// Assuming Filesystem will eventually be included in C++1z, so import its
	// definitions into the std namespace for forward compatibility...
	// NOTE: see http://stackoverflow.com/questions/9864125/c11-how-to-alias-a-function for great ideas on aliasing functions
	using path = std::experimental::filesystem::path;
	using std::experimental::filesystem::canonical;
	using std::experimental::filesystem::current_path;
}

#endif	// __stemple_Filesystem__
