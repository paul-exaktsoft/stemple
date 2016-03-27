// Expander
// ?
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__Expander__
#define __stemple__Expander__

#include "stdafx.h"

namespace stemple
{
	class Expander
	{
	public:
		Expander ();

		virtual ~Expander ();

		std::string Expand (const std::string &input);

		void Expand (std::ostream &output);

		void AddMacro (const std::string &name, const std::string &body, bool simple = false);

	protected:
		std::string expand (const std::string &input, const std::string &source);

		bool processDirective ();

		std::string collectString (const std::string &delims);

		std::vector<std::string> collectArgs ();

		enum Token { ARGS, ASSIGN, APPEND, MOD, SIMPLE_ASSIGN, SIMPLE_APPEND, CLOSE, END, ERR };

		Token getToken ();

		InStream &inStream ();

		InStream *findInStream (const std::string &prefix);

		bool get (char &c, bool expand = true);

		int peek ();

		bool good ()
		{
			return inStreams.size() && inStream().good();
		}

		bool eof ();

		bool fail ()
		{
			return !inStreams.size() || inStream().fail();
		}

		bool bad ()
		{
			return !inStreams.size() || inStream().bad();
		}

		bool putback (const char &ch);

//		std::stack<InStream> streams;
		std::list<InStream> inStreams;
		std::map<std::string, Macro> macros;
	};
}

#endif // __stemple__Expander__