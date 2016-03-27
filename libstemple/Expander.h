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

		void SetMacro (const std::string &name, const std::string &body, bool simple = false);

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

		InStream *findInStreamWithArgs ();

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

		bool Expander::do_if (const std::vector<std::string> &args);
		bool Expander::do_else (const std::vector<std::string> &args);
		bool Expander::do_elseif (const std::vector<std::string> &args);
		bool Expander::do_endif (const std::vector<std::string> &args);

		std::list<std::shared_ptr<InStream>> inStreams;
		std::map<std::string, Macro> macros;
		std::map<std::string, std::function<bool(const std::vector<std::string> &)>> builtins;

		char introChar;				// The start of a directive. Default: '$'
		char openChar;				// The start of a directive body. Default: '('
		char closeChar;				// The end of a directive. Default: ')'
		char argSepChar;			// Separator between arguments. Default: ','
		std::string nameEndChars;	// Set of terminating chars
		std::string argEndChars;	// Set of terminating chars
		std::string textEndChars;	// Set of terminating chars
		bool directiveSeen;			// A directive has been processed on the current line

		// The current character was the last character of this stream, which
		// was popped off the stack because it was at EOF. If we need to put
		// back the current character, this stream will have to be pushed back
		// onto the stack again.
		std::shared_ptr<InStream> putbackStream;
	};
}

#endif // __stemple__Expander__