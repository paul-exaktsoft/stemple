// Macro
// Represents a named macro and its replacement text.
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__Macro__
#define __stemple__Macro__

#include "stdafx.h"

namespace stemple
{
	class Macro
	{
	public:
		//----------------------------------------------------------------------
		Macro ():
			name(""),
			body(""),
			simple(true)
		{
		}

		//----------------------------------------------------------------------
		Macro (const std::string &name, const std::string &body, bool simple = false):
			name(name),
			body(body),
			simple(simple)
		{
		}

		//----------------------------------------------------------------------
		virtual ~Macro ()
		{
		}

		//----------------------------------------------------------------------
		std::string &GetBody ()
		{
			return body;
		}

		//----------------------------------------------------------------------
		bool IsSimple ()
		{
			return simple;
		}

	protected:
		std::string name;
		std::string body;
		bool simple;
	};
}

#endif	// __stemple__Macro__
