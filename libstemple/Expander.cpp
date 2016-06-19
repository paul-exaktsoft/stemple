// Expander
// Processes input streams looking for embedded directives and macro expansions.
//
// Copyright � 2016 by Paul Ashdown. All Rights Reserved.

#include "stdafx.h"

using namespace std;
using namespace std::placeholders;

//==============================================================================
//==============================================================================
namespace stemple
{

	//--------------------------------------------------------------------------
	Expander::Expander () :
		trimArgs(true),
		skipping(0)
	{
		SetSpecialChars('$', '$', '(', ',', ')');
		builtins = {
			{ "if",			bind(&Expander::do_if,			this, _1, _2) },
			{ "else",		bind(&Expander::do_else,		this, _1, _2) },
			{ "elseif",		bind(&Expander::do_elseif,		this, _1, _2) },
			{ "endif",		bind(&Expander::do_endif,		this, _1, _2) },
			{ "env",		bind(&Expander::do_env,			this, _1, _2) },
			{ "include",	bind(&Expander::do_include,		this, _1, _2) },
			{ "equal",		bind(&Expander::do_equal,		this, _1, _2) },
			{ "notequal",	bind(&Expander::do_notequal,	this, _1, _2) },
			{ "match",		bind(&Expander::do_match,		this, _1, _2) },
			{ "and",		bind(&Expander::do_and,			this, _1, _2) },
			{ "or",			bind(&Expander::do_or,			this, _1, _2) },
			{ "not",		bind(&Expander::do_not,			this, _1, _2) },
			{ "defined",	bind(&Expander::do_defined,		this, _1, _2) },
		};
	}

	//--------------------------------------------------------------------------
	Expander::~Expander ()
	{
	}

	//--------------------------------------------------------------------------
	void Expander::SetSpecialChars (char escape, char intro, char open, char argSep, char close)
	{
		escapeChar = escape;
		introChar = intro;
		openChar = open;
		argSepChar = argSep;
		closeChar = close;
		modsChar = ':';
		(nameEndChars = " \t:+=") += closeChar;
		(argEndChars = argSepChar) += closeChar;
		textEndChars = closeChar;
		modEndChars = " \t:+=";
		if (modEndChars.find(modsChar) == string::npos) modEndChars += modsChar;
	}

	//--------------------------------------------------------------------------
	void Expander::SetMacro (const std::string &name, const std::string &body, bool simple)
	{
		macros[name] = Macro(name, simple ? Expand(body) : body, simple);
	}

	//--------------------------------------------------------------------------
	string Expander::Expand (const string &inputString)
	{
		return expand(inputString, "Input string");
	}

	//--------------------------------------------------------------------------
	bool Expander::Expand (istream &input, const string &inputName, ostream &output)
	{
		inStreams.push_front(make_shared<CopiedStream>(input, inputName));
		expand(output);
		return true;
	}

	//--------------------------------------------------------------------------
	string Expander::expand (const string &inputString, const string &source)
	{
		inStreams.push_front(make_shared<StringStream>(inputString, source));
		ostringstream output;
		expand(output);
		return output.str();
	}

	//--------------------------------------------------------------------------
	void Expander::expand (ostream &output)
	{
		string leadingWhitespace;
		char c;
		while (get(c)) {
			if (!skipping) {
				if (c == '\n') {
					if (!currentStream().GraphSeen) {
						// Only output a blank line if we haven't processed any non-
						// printing directives on it, otherwise skip.
						if (!currentStream().DirectiveSeen) {
							output << leadingWhitespace;
							DBG("put(): ws='%s'\n", leadingWhitespace.c_str());
							output.put('\n');
							DBG("put(): c=%s\n", printchar(c));
						}
					} else {
						output.put('\n');
						DBG("put(): c=%s\n", printchar(c));
					}
					// Reset for new line...
					leadingWhitespace.clear();
					currentStream().GraphSeen = false;
					currentStream().DirectiveSeen = false;
				} else if (isspace(c) && !currentStream().GraphSeen) {
					// Collect leading whitespace rather than immediately outputting
					// it. Then we can decide later if we want to skip a blank
					// line containing just a non-printing directive or not.
					leadingWhitespace.append(1, c);
				} else {
					if (!isspace(c) && !currentStream().GraphSeen) {	// NOTE: using !isspace() instead of isgraph() - better for Unicode?
						// Flush collected leading whitespace now that we're
						// outputting a printing character on this line.
						currentStream().GraphSeen = true;
						if (leadingWhitespace.length()) {
							output << leadingWhitespace;
							DBG("put(): ws='%s'\n", leadingWhitespace.c_str());
							leadingWhitespace.clear();
						}
					}
					output.put(c);
					DBG("put(): c=%s\n", printchar(c));
				}
			}
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::get (char &c, bool expand)
	{
		wasEscaped = false;

		// If we've reached the end of the current stream, detect it now. We
		// don't want the next istream::get() to return eof, since we want
		// the next character to come from the 'parent' stream if there is one.
		while (inStreams.size() && currentStream().peek() == char_traits<char>::eof()) {
			inStreams.pop_front();
		}

		// End of input?
		if (!inStreams.size()) {
			c = '\0';
			DBG("get(): EOF\n");
			return false;
		}

		char x;
		if (!currentStream().get(x)) {
			c = '\0';
			DBG("get(): Error from InStream::get()\n");
			return false;
		}

		DBG("get(): x=%s gs=%s (%s)\n", printchar(x), currentStream().GraphSeen ? "true" : "false", currentStream().GetPosition().GetCString());

		// Treat single-character (putback) streams as ephemeral
		if (currentStream().IsCharStream()) {
			inStreams.pop_front();
		}

		if (x == escapeChar) {
			// Handle escape
			char p = peek();
			if (p == introChar || p == escapeChar) {
				// Escaped '$' cannot start a macro
				if (!currentStream().get(x)) return false;	// Eat '$' so it doesn't trigger a macro on the next call
				DBG("  esc: x=%s\n", printchar(x));
				wasEscaped = true;
				goto end;
			} else if (p == '\n') {
				// Escaped newline causes blank line to be output, even if it
				// contains only a non-printing directive
				currentStream().DirectiveSeen = false;
				if (!get(x)) return false;	// Get the newline, skipping the escape
				DBG("  esc: x=%s\n", printchar(x));
				wasEscaped = true;
				goto end;
			}
		}
		if (expand && x == introChar) {
			// Handle macro expansion and stemple directives
			if (peek() == openChar) {
				// Start of a stemple directive
				Position introPos = currentStream().GetPosition();
				if (get(x)) {	// Eat opening '('
					if (processDirective(introPos)) {
					}
					// Get first character of expansion, or character following ')'
					if (!get(x)) {
						return false;
					}
				}
			}
		}
end:
		c = x;
		return true; //good();
	}

	//--------------------------------------------------------------------------
	bool Expander::good ()
	{
		return inStreams.size() && currentStream().good();
	}

	//--------------------------------------------------------------------------
	bool Expander::eof ()
	{
		return !inStreams.size() ? true : currentStream().eof();
	}

	//--------------------------------------------------------------------------
	static inline bool is_number (const string &s)
	{
		return !s.empty() && find_if(begin(s), end(s), [](char c) { return !isdigit(c); }) == end(s);
	}

	//--------------------------------------------------------------------------
	bool Expander::processDirective (const Position &introPos)
	{
		// We've seen opening "$(", now collect first token
		string name = collectString(nameEndChars);

		Mods mods(trimArgs);
		ArgList args;

		{
			// If this is an elseif directive, temporarily disable skipping so
			// we can fully expand the argument to see if this branch should be
			// taken or not.
			variable_guard<int> guard(skipping, name == "elseif" ? 0 : skipping);

			// Examine token after first word
			Token tok = getToken();

			if (tok == MOD) {
				tok = collectMods(mods);
			}

			switch (tok) {
			case ARGS:
			{
				args = collectArgs(mods.TrimArgs, mods.ExpandArgs);
				tok = getToken();	// Get closing ')'
				break;
			}
			case ASSIGN:
			case SIMPLE_ASSIGN:
			case APPEND:
			case SIMPLE_APPEND:
			{
				currentStream().DirectiveSeen = true;
				bool append = tok == APPEND || tok == SIMPLE_APPEND;
				bool simple = tok == SIMPLE_ASSIGN || tok == SIMPLE_APPEND;
				string text = collectString(textEndChars, simple);
				tok = getToken();	// Get closing ')'
				if (append) {
					auto macro = macros.find(name);
					if (macro != end(macros)) {
						macro->second.GetBody() += text;
					} else {
						SetMacro(name, text);
					}
				} else {
					SetMacro(name, text);
				}
				return true;
			}
			case CLOSE:
			case END:
			case ERR:
			case MOD:
				break;
			}
		}

		// assert(tok == CLOSE);

		if (!skipping || name == "if" || name == "else" || name == "elseif" || name == "endif") {
			auto builtinEntry = builtins.find(name);
			if (builtinEntry != end(builtins)) {
				currentStream().DirectiveSeen = true;
				// Process builtin directive
				DBG("expanding %s at %s:\n", name.c_str(), introPos.GetCString()); {
					int n = 1; for (string a : args) DBG("    arg %d: %s\n", n++, a.c_str());
				}
				return builtinEntry->second(args, mods);
			} else if (is_number(name)) {
				// An argument to an enclosing expansion. Look for the closest
				// 'enclosing' macro body and get its associated arguments.
				InStream *baseStream = findStreamWithArgs();
				if (baseStream) {
					int index = atoi(name.c_str()) - 1;
					DBG("expanding arg %s of %s at %s\n", name.c_str(), baseStream->GetSource().c_str(), introPos.GetCString());
					putback(baseStream->GetArg(index), string("Expansion of arg ") + name + " of " + baseStream->GetSource());
					return true;
				} else {
					DBG("empty expansion of arg %s at %s\n", name.c_str(), introPos.GetCString());
				}
			} else {
				// Lookup macro and insert replacement text if any
				auto macroEntry = macros.find(name);
				if (macroEntry != end(macros)) {
					string text = macroEntry->second.GetBody();
					DBG("expanding %s at %s:\n", name.c_str(), introPos.GetCString()); {
						int n = 1; for (string a : args) DBG("    arg %d: %s\n", n++, a.c_str());
					}
					if (text.length()) {
						putback(text, string("Expansion of ") + name, args);
					}
					return true;
				} else {
					DBG("empty expansion of %s at %s\n", name.c_str(), introPos.GetCString());
				}
			}
		}
		return false;
	}

	//--------------------------------------------------------------------------
	// The only time this is called with expand==false is when collecting the
	// contents of a normal recursive variable assignment. In this case, nested
	// directives need to be tracked since they are also terminated by the same
	// end-delimiter we are looking for.

	string Expander::collectString (const string &delims, bool expand)
	{
		int nested = 0;
		ostringstream output;
		char c;
		while (get(c, expand)) {
			bool escaped = false;
			if (c == escapeChar) {
				char p = peek();
				// If escaping a delimiter, skip the escape and get the delimiter,
				// else just keep the escape
				if (delims.find(p) != string::npos) {
					get(c, expand);
					escaped = true;
				}
			} else if (!nested && delims.find(c) != string::npos) {
				putback(c);
				break;
			}
			if (!expand && !escaped) {
				// Keep track of nested directives when not expanding
				// TODO: This doesn't really match full directives, just '(' and ')'
				if (c == openChar) {
					++ nested;
				} else if (c == closeChar) {
					-- nested;
				}
			}
			output.put(c);
		}
		return output.str();
	}

	//--------------------------------------------------------------------------
	ArgList Expander::collectArgs (bool trim, bool expand)
	{
		ArgList args;
		char c;
		do {
			string arg = collectString(argEndChars, expand);
			args.push_back(trim ? trimWhitespace(arg) : arg);
			get(c);
		} while (c == argSepChar);
		putback(c);
		return args;
	}

	//--------------------------------------------------------------------------
	Expander::Token Expander::collectMods (Mods &mods)
	{
		Token tok;
		do {
			string mod = collectString(modEndChars);
			if (mod == "n") {
				mods.TrimArgs = false;
			} else if (mod == "i") {
				mods.IgnoreCase = true;
			} else if (mod == "x") {
				mods.ExpandArgs = false;
			}
			tok = getToken();
		} while (tok == MOD);
		return tok;
	}

	//--------------------------------------------------------------------------
	string Expander::trimWhitespace (const string &s)
	{
		if (!s.length()) return s;

		// Look for initial whitespace. If <escape><whitespace> found, stop
		bool escaped = false;
		size_t i;
		for (i = 0; i < s.length(); ++ i) {
			if (!isspace(s[i])) {
				if (s[i] == escapeChar && i + 1 < s.length() && isspace(s[i + 1])) {
					++ i;	// Omit escape, leaving subsequent leading whitespace
					escaped = true;
				}
				break;
			}
		}
		// Look for trailing whitespace
		size_t j = s.length() - 1;
		if (!escaped) {	// TODO: escape before leading whitespace also preserves trailing whitespace - make that configurable?
			for (; j >= i; -- j) {
				if (!isspace(s[j])) {
					break;
				}
			}
		}
		size_t count = i > j ? 0 : j - i + 1;
		if (j == i && escaped) {
			// String will consist only of escaped whitespace
			count = s.length() - i;
		}
		return s.substr(i, count);
	}

	//--------------------------------------------------------------------------
	Expander::Token Expander::getToken ()
	{
		string whitespace;
		char c;
		while (get(c, false)) {
			if (isspace(c)) {
				whitespace += c;
			} else if (c == ':') {
				if (get(c, false)) {
					if (c == '+') {
						if (get(c, false)) {
							if (c == '=') {
								return SIMPLE_APPEND;
							} else {
								putback(c);
							}
						}
						return ERR;
					} else if (c == '=') {
						return SIMPLE_ASSIGN;
					} else if (modsChar == ':') {
						putback(c);
						return MOD;
					}
				}
				if (modsChar == ':') {
					return MOD;
				} else {
					return ERR;
				}
			} else if (c == '+') {
				if (get(c, false)) {
					if (c == '=') {
						return APPEND;
					}
				}
				if (modsChar == '+') {
					return MOD;
				} else {
					return ERR;
				}
			} else if (c == modsChar) {
				return MOD;
			} else if (c == '=') {
				return ASSIGN;
			} else if (c == closeChar) {
				return CLOSE;
			} else {
				if (whitespace.length() > 0) {
					putback(c);
					if (whitespace.length() > 1) {
						putback(whitespace.substr(1, whitespace.size() - 1), "Whitespace putback");
					}
					return ARGS;
				}
				return ERR;
			}
		}
		return ERR;
	}

	//--------------------------------------------------------------------------
	// Returns a pointer rather than a reference because we want the result to
	// be nullable to denote not found.

	InStream *Expander::findStream (function<bool(const shared_ptr<InStream> &ptr)> pred)
	{
		for (auto &ptr : inStreams) {
			if (pred(ptr)) {
				return ptr.get();
			}
		}
		return nullptr;
	}

	//--------------------------------------------------------------------------
	InStream *Expander::findStreamWithNamePrefix (const string &prefix)
	{
		return findStream([prefix](const shared_ptr<InStream> &ptr) {
			return ptr->GetSource().compare(0, prefix.length(), prefix) == 0;
		});
	}

	//--------------------------------------------------------------------------
	InStream *Expander::findStreamWithArgs ()
	{
		return findStream([](const shared_ptr<InStream> &ptr) {
			return ptr->GetArgCount() > 0;
		});
	}

	//--------------------------------------------------------------------------
	InStream *Expander::findStreamWithPath ()
	{
		return findStream([](const shared_ptr<InStream> &ptr) {
			return ptr->GetPath() != nullptr;
		});
	}

	//--------------------------------------------------------------------------
	const path Expander::getCurrentPath ()
	{
		InStream *is = findStreamWithPath();
		return is ? path(*is->GetPath()).remove_filename() : current_path();
	}

	//--------------------------------------------------------------------------
	int Expander::peek ()
	{
		return currentStream().peek();
	}

	//--------------------------------------------------------------------------
	bool Expander::putback (const char &c)
	{
		// Putback for file streams seems to be problematic (in my Mac OS X
		// build, it always fails). So we no longer use std::istream's putback
		// facility and use separate putback storage. The simplest way to do it
		// is to push a new InStream holding a single character.
		// TODO: Optimize single-character putback to use a lighter-weight mechanism?
		DBG("putback(): %s\n", printchar(c));
		Position p = currentStream().GetPutbackPosition();
		inStreams.push_front(make_shared<CharStream>(c, p));
		if (wasEscaped) {
			DBG("putback(): %s\n", printchar(escapeChar));
			inStreams.push_front(make_shared<CharStream>(escapeChar, p.Putback()));
		}
		return good();
	}

	//--------------------------------------------------------------------------
	bool Expander::putback (const string &s, const string &streamName, const ArgList &args)
	{
		inStreams.push_front(make_shared<StringStream>(s, streamName, args));
		return good();
	}

	//--------------------------------------------------------------------------
	bool Expander::do_if (const ArgList &args, const Mods &mods)
	{
		bool testResult = false;
		if (args.size() > 0) {
			testResult = textToBool(args[0]);
		}
		if (args.size() == 2 || args.size() == 3) {
			// Inline form
			if (!skipping) {
				if (testResult) {
					putback(args[1], "True branch");
				} else if (args.size() > 2) {
					putback(args[2], "False branch");
				}
			}
			return true;
		} else {
			if (args.size() != 1) {
				// TODO: Report error
			}
			if (testResult) {
				ifContext.push({ IfContext::Phase::ElseOrEnd, true, false });
			} else {
				ifContext.push({ IfContext::Phase::ElseOrEnd, false, true });
				++ skipping;
			}
			return true;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_else (const ArgList &args, const Mods &mods)
	{
		if (ifContext.size() && ifContext.top().phase == IfContext::Phase::ElseOrEnd) {
			if (!ifContext.top().branchTaken) {
				// assert: must currently be skipping
				ifContext.top().isSkipping = false;
				-- skipping;
			} else {
				if (!ifContext.top().isSkipping) {
					ifContext.top().isSkipping = true;
					++ skipping;
				}
			}
			ifContext.top().phase = IfContext::Phase::EndOnly;
			return true;
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_elseif (const ArgList &args, const Mods &mods)
	{
		if (ifContext.size() && ifContext.top().phase == IfContext::Phase::ElseOrEnd) {
			if (!ifContext.top().branchTaken) {
				bool testResult = false;
				if (args.size() == 1) {
					testResult = textToBool(args[0]);
				} else {
					// TODO: Report error
				}
				if (testResult) {
					if (ifContext.top().isSkipping) {
						ifContext.top().isSkipping = false;
						-- skipping;
					}
					ifContext.top().branchTaken = true;
				} else {
					if (!ifContext.top().isSkipping) {
						ifContext.top().isSkipping = true;
						++ skipping;
					}
				}
			} else {
				if (!ifContext.top().isSkipping) {
					ifContext.top().isSkipping = true;
					++ skipping;
				}
			}
			return true;
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_endif (const ArgList &args, const Mods &mods)
	{
		if (ifContext.size()) {
			if (ifContext.top().isSkipping) {
				-- skipping;
			}
			ifContext.pop();
			return true;
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_env (const ArgList &args, const Mods &mods)
	{
		if (args.size() && args[0].size()) {
			const char *env = getenv(args[0].c_str());
			if (env) {
				putback(env, args[0] + " environment variable");
			}
			return true;
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_include (const ArgList &args, const Mods &mods)
	{
		if (args.size() && args[0].size()) {
			const vector<string> restArgs(args.begin() + 1, args.end());
			path p = canonical(args[0], getCurrentPath());
			inStreams.push_front(make_shared<FileStream>(p.string(), restArgs));
			return currentStream().good();
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_equal (const ArgList &args, const Mods &mods)
	{
		if (args.size() > 1) {
			putback(compare(args[0], args[1], mods.IgnoreCase) ? "1" : "0", "Equal result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "Equal error");
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_notequal (const ArgList &args, const Mods &mods)
	{
		if (args.size() > 1) {
			putback(!compare(args[0], args[1], mods.IgnoreCase) ? "1" : "0", "Notequal result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "Notequal error");
			return false;
		}
	}

	//--------------------------------------------------------------------------
	// Uses modified ECMAScript: http://en.cppreference.com/w/cpp/regex/ecmascript

	bool Expander::do_match (const ArgList &args, const Mods &mods)
	{
		if (args.size() > 1) {
			regex::flag_type flags = regex_constants::ECMAScript;
			if (mods.IgnoreCase) flags |= regex_constants::icase;
			regex pattern(args[1], flags);
			bool match = regex_search(args[0], pattern);
			putback(match ? "1" : "0", "Match result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "Match error");
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_and (const ArgList &args, const Mods &mods)
	{
		if (args.size() > 1) {
			putback(textToBool(args[0]) && textToBool(args[1]) ? "1" : "0", "And result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "And error");
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_or (const ArgList &args, const Mods &mods)
	{
		if (args.size() > 1) {
			putback(textToBool(args[0]) || textToBool(args[1]) ? "1" : "0", "Or result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "Or error");
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_not (const ArgList &args, const Mods &mods)
	{
		if (args.size() == 1) {
			putback(!textToBool(args[0]) ? "1" : "0", "Not result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "Not error");
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_defined (const ArgList &args, const Mods &mods)
	{
		if (args.size() == 1) {
			bool defined = false;
			if (is_number(args[0])) {
				// Lookup argument to an enclosing expansion. Look for the
				// closest 'parent' macro body and get its associated arguments.
				InStream *baseStream = findStreamWithArgs();
				if (baseStream) {
					int index = atoi(args[0].c_str()) - 1;
					defined = index >= 0 && index < baseStream->GetArgCount();
				}
			} else {
				// Lookup macro
				auto macroEntry = macros.find(args[0]);
				defined = macroEntry != end(macros);
			}
			putback(defined ? "1" : "0", "Defined result");
			return true;
		} else {
			// TODO: Report error
			putback("0", "Defined error");
			return false;
		}
	}
}
