// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined _WIN32
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#endif	// _WIN32

#include <algorithm>
#include <experimental/filesystem>
namespace std {
	// Assuming Filesystem will eventually be included in C++1z, so import its
	// definitions into the std namespace for forward compatibility...
	// NOTE: see http://stackoverflow.com/questions/9864125/c11-how-to-alias-a-function for great ideas on aliasing functions
	using path = std::experimental::filesystem::path;
	using std::experimental::filesystem::canonical;
	using std::experimental::filesystem::current_path;
}
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <string>
#include <vector>

#include <cstring>
#include <cstdlib>

#include "Utility.h"
#include "ArgList.h"
#include "Position.h"
#include "InStream.h"
#include "Macro.h"
#include "Expander.h"
