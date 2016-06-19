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

#include "ArgList.h"
#include "cstream.h"
#include "Expander.h"
#include "Filesystem.h"
#include "InStream.h"
#include "Macro.h"
#include "Position.h"
#include "stemple.h"
#include "Utility.h"
