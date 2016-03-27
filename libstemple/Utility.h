// Utility
// Useful functions
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__Utility__
#define __stemple__Utility__

#include "stdafx.h"

namespace stemple
{

	//--------------------------------------------------------------------------
	// Creates a std::string using sprintf()-like syntax

	template<typename... Args>
	inline std::string stringf (const char *fmt, Args... args)
	{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"	// Suppress "warning: format string is not a string literal (potentially insecure)"

		size_t size = snprintf(nullptr, 0, fmt, args...) + 1; // Extra space for '\0'
		std::unique_ptr<char[]> buf(new char[size]);
		snprintf(buf.get(), size, fmt, args...);
		return std::string(buf.get(), buf.get() + size - 1); // Strip the '\0'

#pragma clang diagnostic pop
	}

	//--------------------------------------------------------------------------
	// Creates a std::string using sprintf()-like syntax

	template<typename... Args>
	inline std::string stringf (const std::string &fmt, Args... args)
	{
		return stringf(fmt.c_str(), args...);
	}

	//--------------------------------------------------------------------------
	// Creates a std::string using vsprintf()-like syntax

	inline std::string vstringf (const char *fmt, va_list ap)
	{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"	// Suppress "warning: format string is not a string literal (potentially insecure)"

		size_t size = vsnprintf(nullptr, 0, fmt, ap) + 1; // Extra space for '\0'
		std::unique_ptr<char[]> buf(new char[size]);
		vsnprintf(buf.get(), size, fmt, ap);
		return std::string(buf.get(), buf.get() + size - 1); // Strip the '\0'

#pragma clang diagnostic pop
	}

	//--------------------------------------------------------------------------
	template<typename... Args>
	inline void DBG (const std::string &fmt, Args... args)
	{
		string s = stringf(fmt, args...);

#if defined _WIN32
		OutputDebugStringA(s.c_str());
#else
		//	printf("%s", s.c_str());
#endif
	}

	//--------------------------------------------------------------------------
	inline char *printchar (const char &c)
	{
		static char buf[5];
		if (c == '\t') {
			strcpy(buf, "\\t");
		} else if (c == '\n') {
			strcpy(buf, "\\n");
		} else if (c < ' ') {
			snprintf(buf, 5, "0x%02X", c);
		} else {
			buf[0] = c;
			buf[1] = '\0';
		}
		return buf;
	}

	//--------------------------------------------------------------------------
	inline bool compare (const std::string &a, const std::string &b, bool ignoreCase = false)
	{
		if (a.size() != b.size()) {
			return false;
		}
		if (ignoreCase) {
			for (auto ia = begin(a), ib = begin(b); ia != end(a); ++ ia, ++ ib) {
				if (tolower(*ia) != tolower(*ib)) return false;
			}
			return true;
		} else {
			return a.compare(b) == 0;
		}
	}

	//--------------------------------------------------------------------------
	inline bool textToBool (const std::string &text)
	{
		// TODO: trim text before comparison?
		return 	!(text == "" ||
				  text == "0" ||
				  compare(text, "false", true) ||
				  compare(text, "no", true));
	}

	//--------------------------------------------------------------------------
	// An RAII helper class that temporarily sets a variable and restores its old
	// value when the helper goes out of scope.

	template<typename T>
	class variable_guard
	{
	public:
		variable_guard (T &variable, const T &new_value) :
			variable(variable),
			initial_value(variable)
		{
			variable = new_value;
		}
		~variable_guard ()
		{
			variable = initial_value;
		}
	private:
		T &variable;
		T initial_value;
	};

}

#endif	// __stemple__Utility__