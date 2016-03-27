// Expander
// ?
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#include "stdafx.h"

using namespace std;

//------------------------------------------------------------------------------
template<typename... Args>
inline void DEBUG (const std::string &fmt, Args... args)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"	// Suppress "warning: format string is not a string literal (potentially insecure)"
	size_t size = ::snprintf(nullptr, 0, fmt.c_str(), args...) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, fmt.c_str(), args...);
#pragma clang diagnostic pop

	OutputDebugStringA(buf.get());
}

//------------------------------------------------------------------------------
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

namespace stemple
{

	//--------------------------------------------------------------------------
	Expander::Expander ()
	{
	}

	//--------------------------------------------------------------------------
	Expander::~Expander ()
	{
	}

	//--------------------------------------------------------------------------
	void Expander::AddMacro (const std::string &name, const std::string &body, bool simple)
	{
		macros[name] = Macro(name, simple ? Expand(body) : body, simple);
	}

	//--------------------------------------------------------------------------
	string Expander::Expand (const string &inputString)
	{
		return expand(inputString, "Input string");
	}

	//--------------------------------------------------------------------------
	void Expander::Expand (ostream &output)
	{
		char c;
		while (get(c)) {
			output.put(c);
		}
	}

	//--------------------------------------------------------------------------
	string Expander::expand (const string &inputString, const string &source)
	{
		inStreams.push_front(InStream(inputString, source));
		ostringstream output;
		Expand(output);
		return output.str();
	}

	//--------------------------------------------------------------------------
	bool Expander::get (char &c, bool expand)
	{
		if (!inStreams.size()) {
			DEBUG("get(): EOF\n");
			return false;
		}

		char x;
		if (!inStream().get(x)) return false;

		DEBUG("get(): x=%s (%s)\n", printchar(x), inStreams.front().GetSource().c_str());

		// If we've reached the end of the current stream, detect it now. We don't
		// wait for the next istream::get() to return eof, since we want the
		// next character to come from the 'parent' stream if there is one.
		if (inStream().peek() == char_traits<char>::eof()) {
			if (inStreams.size()) {
				inStreams.pop_front();
			}
		}

		// Handle macro expansion and stemple directives
		if (expand && x == '$') {
			if (!eof()) {
				if (peek() == '(') {
					// Start of a stemple directive
					if (get(x)) {	// Eat opening '('
						if (processDirective()) {
						}
						get(x);	// Get first character of expansion, or character following ')'
					}
				} else if (peek() == '$') {
					// Escaped '$' cannot start a macro
					get(x, false);	// Eat '$' so it doesn't trigger a macro on the next call
				}
			}
		}
		c = x;
		return true; //good();
	}

	//--------------------------------------------------------------------------
	bool Expander::eof ()
	{
		return !inStreams.size() ? true : inStream().eof();
	}

	//--------------------------------------------------------------------------
	static inline bool is_number (const string &s)
	{
		return !s.empty() && find_if(begin(s), end(s), [](char c) { return !isdigit(c); }) == end(s);
	}

	//--------------------------------------------------------------------------
	bool Expander::processDirective ()
	{
		// We've seen opening "$(", now collect first token
		string word = collectString(" \t:+=)");

		vector<string> args;
		
		// Examine token after first word
		Token tok = getToken();
		switch (tok) {
		case ARGS:
			{
				args = collectArgs();
				tok = getToken();	// Get closing ')'
			}
		break;
		case ASSIGN:
		case SIMPLE_ASSIGN:
		case APPEND:
		case SIMPLE_APPEND:
			{
				bool append = tok == APPEND || tok == SIMPLE_APPEND;
				bool simple = tok == SIMPLE_ASSIGN || tok == SIMPLE_APPEND;
			}
			break;
		case MOD:
			break;
		case CLOSE:
			break;
		case END:
		case ERR:
			break;
		}

		// assert(tok == CLOSE);

		// Lookup macro and insert replacement text
		if (is_number(word)) {
			// Macro is an argument to a previous expansion. Look for the
			// closest 'parent' macro body and get its associated arguments.
			InStream *baseStream = findInStream("Body of ");
			if (baseStream) {
				int index = atoi(word.c_str()) - 1;
				inStreams.push_front(InStream(baseStream->GetArg(index), baseStream->GetSource() + ", arg " + word));
				return true;
			}
		} else {
			auto macro = macros.find(word);
			if (macro != end(macros)) {
				inStreams.push_front(InStream(macro->second.GetBody(), string("Body of ") + word, args));
				return true;
			}
		}
		return false;
	}

	//--------------------------------------------------------------------------
	string Expander::collectString (const string &delims)
	{
		ostringstream output;
		char c;
		while (get(c)) {
			if (delims.find(c) != string::npos) {
				putback(c);
				break;
			}
			output.put(c);
		}
		return output.str();
	}

	//--------------------------------------------------------------------------
	vector<string> Expander::collectArgs ()
	{
		vector<string> args;
		char c;
		do {
			string arg = collectString(",)");
			args.push_back(arg);
			get(c);
		} while (c == ',');
		putback(c);
		return args;
	}

	//--------------------------------------------------------------------------
	Expander::Token Expander::getToken ()
	{
		string whitespace;
		char c;
		while (get(c)) {
			if (c == ' ' || c == '\t') {
				whitespace += c;
			} else if (c == ':') {
				if (get(c)) {
					if (c == '+') {
						if (get(c)) {
							if (c == '=') {
								return SIMPLE_APPEND;
							} else {
								putback(c);
							}
						}
						return ERR;
					} else if (c == '=') {
						return SIMPLE_ASSIGN;
					} else {
						putback(c);
						return MOD;
					}
				}
				return MOD;
			} else if (c == '+') {
				if (get(c)) {
					if (c == '=') {
						return APPEND;
					}
				}
				return ERR;
			} else if (c == '=') {
				return ASSIGN;
			} else if (c == ')') {
				return CLOSE;
			} else {
				if (whitespace.length() > 0) {
					putback(c);
					if (whitespace.length() > 1) {
						inStreams.push_front(InStream(whitespace.substr(1, whitespace.size() - 1), "Whitespace putback"));
					}
					return ARGS;
				}
				return ERR;
			}
		}
		return ERR;
	}

	//--------------------------------------------------------------------------
	InStream &Expander::inStream ()
	{
		return inStreams.front();
	}

	//--------------------------------------------------------------------------
	InStream *Expander::findInStream (const string &prefix)
	{
		for (InStream &is : inStreams) {
			const string &source = is.GetSource();
			if (source.compare(0, prefix.length(), prefix) == 0) {
				return &is;
			}
		}
		return nullptr;
	}

	//--------------------------------------------------------------------------
	int Expander::peek ()
	{
		return inStream().peek();
	}

	//--------------------------------------------------------------------------
	bool Expander::putback (const char &c)
	{
		if (!inStreams.size() || inStream().tellg() == streampos(0)) {
			// If no current stream, or stream is at start (because ch was
			// obtained from a nested stream that has now been popped), then
			// push a new stream
			inStreams.push_front(InStream(string(1, c), "Putback"));
		} else {
			// Putback on the current stream
			inStream().putback(c);
		}
		return good();
	}
}
