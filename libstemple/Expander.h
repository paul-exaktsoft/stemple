// Expander
// Processes input streams using embedded directives and macro expansions.
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

		bool Expand (std::istream &input, const std::string &inputName, std::ostream &output);

		void SetMacro (const std::string &name, const std::string &body, bool simple = false);

		void SetSpecialChars (char intro, char open, char close, char argSep, char escape);

	protected:
		std::string expand (const std::string &input, const std::string &source);

		void expand (std::ostream &output);

		bool processDirective ();

		std::string collectString (const std::string &delims, bool expand = true);

		ArgList collectArgs (bool trim);

		std::string trimWhitespace (const std::string &s);

		enum Token { ARGS, ASSIGN, APPEND, MOD, SIMPLE_ASSIGN, SIMPLE_APPEND, CLOSE, END, ERR };

		Token getToken ();

		inline InStream &inStream ()
		{
			return *inStreams.front();
		}

		InStream *findInStream (std::function<bool(const std::shared_ptr<InStream> &ptr)> pred);
		InStream *findInStreamWithNamePrefix (const std::string &prefix);
		InStream *findInStreamWithArgs ();
		InStream *findInStreamWithPath ();

		const std::path getCurrentPath ();

		bool get (char &c, bool expand = true);

		int peek ();

		bool good ();

		bool eof ();

		bool putback (const char &c);

		bool putback (const std::string &s, const std::string &streamName, const ArgList &args = {});

		bool do_if (const ArgList &args);
		bool do_else (const ArgList &args);
		bool do_elseif (const ArgList &args);
		bool do_endif (const ArgList &args);
		bool do_env (const ArgList &args);
		bool do_include (const ArgList &args);
		bool do_equal (const ArgList &args);
		bool do_notequal (const ArgList &args);
		bool do_match (const ArgList &args);
		bool do_and (const ArgList &args);
		bool do_or (const ArgList &args);
		bool do_not (const ArgList &args);
		bool do_defined (const ArgList &args);

		std::list<std::shared_ptr<InStream>> inStreams;
		std::map<std::string, Macro> macros;
		std::map<std::string, std::function<bool(const ArgList &)>> builtins;

		char introChar;				// The start of a directive. Default: '$'
		char openChar;				// The start of a directive body. Default: '('
		char closeChar;				// The end of a directive. Default: ')'
		char argSepChar;			// Separator between arguments. Default: ','
		char escapeChar;			// Escapes other special chars. Default '$'
		std::string nameEndChars;	// Set of terminating chars
		std::string argEndChars;	// Set of terminating chars
		std::string textEndChars;	// Set of terminating chars
		bool trimArgs;				// Trim whitespace from argument strings by default
		int skipping;				// Skipping output and most expansion because we are in a false branch of a block if/elseif/else

		// A stack of descriptors for processing nested block ifs/elseifs/elses
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