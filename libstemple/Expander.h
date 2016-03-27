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

		void SetSpecialChars (char intro, char open, char close, char argSep);

	protected:
		std::string expand (const std::string &input, const std::string &source);

		bool processDirective ();

		std::string collectString (const std::string &delims, bool expand = true);

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

		std::list<InStream> inStreams;
		std::map<std::string, Macro> macros;
		char introChar;				// The start of a directive. Default: '$'
		char openChar;				// The start of the directive body. Default: '('
		char closeChar;				// The end of the directive. Default: ')'
		char argSepChar;			// Separator between arguments. Default: ','
		std::string nameEndChars;	// Set of terminating chars
		std::string argEndChars;	// Set of terminating chars
		std::string textEndChars;	// Set of terminating chars
		bool directiveSeen;			// A directive has been processed on the current line
	};
}

#endif // __stemple__Expander__