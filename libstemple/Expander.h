// Expander
// Processes input streams using embedded directives and macro expansions.
//
// Copyright � 2016 by Paul Ashdown. All Rights Reserved.

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

		bool Expand (std::shared_ptr<std::istream> &input, const std::string &inputName, std::shared_ptr<std::ostream> &output);

		void SetMacro (const std::string &name, const std::string &body, bool simple = false);

		void SetSpecialChars (char intro, char open, char close, char argSep, char escape);

	protected:
		std::string expand (const std::string &input, const std::string &source);

		void expand (std::ostream &output);

		bool processDirective ();

		std::string collectString (const std::string &delims, bool expand = true);

		std::vector<std::string> collectArgs (bool trim);

		std::string trimWhitespace (const std::string &s);

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

		bool putback (const char &c);

		bool putback (const std::string &s, const std::string &streamName, const std::vector<std::string> &args = {});

		bool do_if (const std::vector<std::string> &args);
		bool do_else (const std::vector<std::string> &args);
		bool do_elseif (const std::vector<std::string> &args);
		bool do_endif (const std::vector<std::string> &args);
		bool do_env (const std::vector<std::string> &args);
		bool do_include (const std::vector<std::string> &args);
		bool do_equal (const std::vector<std::string> &args);
		bool do_notequal (const std::vector<std::string> &args);
		bool do_match (const std::vector<std::string> &args);
		bool do_and (const std::vector<std::string> &args);
		bool do_or (const std::vector<std::string> &args);
		bool do_not (const std::vector<std::string> &args);
		bool do_defined (const std::vector<std::string> &args);

		std::list<std::shared_ptr<InStream>> inStreams;
		std::map<std::string, Macro> macros;
		std::map<std::string, std::function<bool(const std::vector<std::string> &)>> builtins;

		char introChar;				// The start of a directive. Default: '$'
		char openChar;				// The start of a directive body. Default: '('
		char closeChar;				// The end of a directive. Default: ')'
		char argSepChar;			// Separator between arguments. Default: ','
		char escapeChar;			// Escapes other special chars. Default '$'
		std::string nameEndChars;	// Set of terminating chars
		std::string argEndChars;	// Set of terminating chars
		std::string textEndChars;	// Set of terminating chars
		bool trimArgs;				// Trim whitespace from argument strings by default
		bool directiveSeen;			// A directive has been processed on the current line
		int skipping;				// Skipping output and most expansion because we are in a false branch of a block if/else/elseif

		// A stack of descriptors for processing nested block if/else/elseifs
		struct IfContext
		{
			enum Phase { ElseOrEnd, EndOnly};
			Phase phase;
			bool branchTaken;
			bool isSkipping;
		};
		std::stack<IfContext> ifContext;
	};
}

#endif // __stemple__Expander__