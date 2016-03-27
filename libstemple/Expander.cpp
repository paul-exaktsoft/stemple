// Expander
// Processes input streams looking for embedded directives and macro expansions.
//
// Copyright � 2016 by Paul Ashdown. All Rights Reserved.

#include "stdafx.h"

using namespace std;
using namespace std::placeholders;

//------------------------------------------------------------------------------
template<typename... Args>
inline void DBG (const std::string &fmt, Args... args)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"	// Suppress "warning: format string is not a string literal (potentially insecure)"
	size_t size = ::snprintf(nullptr, 0, fmt.c_str(), args...) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, fmt.c_str(), args...);
#pragma clang diagnostic pop

#if defined _WIN32
	OutputDebugStringA(buf.get());
#else
//	printf("%s", buf.get());
#endif
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

//------------------------------------------------------------------------------
inline bool compare (const string &a, const string &b, bool ignoreCase = false)
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

//------------------------------------------------------------------------------
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

//==============================================================================
//==============================================================================
namespace stemple
{

	//--------------------------------------------------------------------------
	Expander::Expander () :
		trimArgs(true),
		directiveSeen(false),
		skipping(0)
	{
		SetSpecialChars('$', '(', ')', ',', '$');
		builtins = {
			{ "if",			bind(&Expander::do_if,			this, _1) },
			{ "else",		bind(&Expander::do_else,		this, _1) },
			{ "elseif",		bind(&Expander::do_elseif,		this, _1) },
			{ "endif",		bind(&Expander::do_endif,		this, _1) },
			{ "env",		bind(&Expander::do_env,			this, _1) },
			{ "include",	bind(&Expander::do_include,		this, _1) },
			{ "equal",		bind(&Expander::do_equal,		this, _1) },
			{ "notequal",	bind(&Expander::do_notequal,	this, _1) },
			{ "match",		bind(&Expander::do_match,		this, _1) },
			{ "and",		bind(&Expander::do_and,			this, _1) },
			{ "or",			bind(&Expander::do_or,			this, _1) },
			{ "not",		bind(&Expander::do_not,			this, _1) },
			{ "defined",	bind(&Expander::do_defined,		this, _1) },
		};
	}

	//--------------------------------------------------------------------------
	Expander::~Expander ()
	{
	}

	//--------------------------------------------------------------------------
	void Expander::SetSpecialChars (char intro, char open, char close, char argSep, char escape)
	{
		introChar = intro;
		openChar = open;
		closeChar = close;
		argSepChar = argSep;
		escapeChar = escape;
		(nameEndChars = " \t:+=") += closeChar;
		(argEndChars = argSepChar) += closeChar;
		textEndChars = closeChar;
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
	bool Expander::Expand (shared_ptr<istream> &input, const string &inputName, shared_ptr<ostream> &output)
	{
		inStreams.push_front(make_shared<InStream>(input, inputName));
		expand(*output);
		return true;
	}

	//--------------------------------------------------------------------------
	void Expander::expand (ostream &output)
	{
		string leadingWhitespace;
		bool graphSeen = false;
		directiveSeen = false;
		char c;
		while (get(c)) {
			if (!skipping) {
				if (c == '\n') {
					if (!graphSeen) {
						// Only output a blank line if we haven't processed any non-
						// printing directives on it, otherwise skip.
						if (!directiveSeen) {
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
					graphSeen = false;
					directiveSeen = false;
				} else if (isspace(c) && !graphSeen) {
					// Collect leading whitespace rather than immediately outputting
					// it. Then we can decide later if we want to skip a blank
					// line containing just a non-printing directive or not.
					leadingWhitespace.append(1, c);
				} else {
					if (!isspace(c) && !graphSeen) {	// NOTE: using !isspace() instead of isgraph() - better for Unicode?
						// Flush collected leading whitespace now that we're
						// outputting a printing character on this line.
						graphSeen = true;
						if (leadingWhitespace.length()) {
							output << leadingWhitespace;
						}
					}
					output.put(c);
					DBG("put(): c=%s\n", printchar(c));
				}
			}
		}
	}

	//--------------------------------------------------------------------------
	string Expander::expand (const string &inputString, const string &source)
	{
		inStreams.push_front(make_shared<InStream>(inputString, source));
		ostringstream output;
		expand(output);
		return output.str();
	}

	//--------------------------------------------------------------------------
	bool Expander::get (char &c, bool expand)
	{
		if (!inStreams.size()) {
			c = '\0';
			DBG("get(): EOF\n");
			return false;
		}

		char x;
		if (!inStream().get(x)) {
			DBG("get(): Error from InStream::get()\n");
			return false;
		}

		DBG("get(): x=%s (%s)\n", printchar(x), inStreams.front()->GetSource().c_str());

		// If we've reached the end of the current stream, detect it now. We
		// won't wait for the next istream::get() to return eof, since we want
		// the next character to come from the 'parent' stream if there is one.
		if (inStream().peek() == char_traits<char>::eof()) {
			if (inStreams.size()) {
				putbackStream = inStreams.front();
				inStreams.pop_front();
			}
		} else {
			putbackStream = nullptr;
		}

		if (!eof()) {
			if (x == escapeChar) {
				// Handle escape
				char p = peek();
				if (p == introChar || p == escapeChar) {
					// Escaped '$' cannot start a macro
					if (!inStream().get(x)) return false;	// Eat '$' so it doesn't trigger a macro on the next call
					goto end;
				} else if (p == '\n') {
					// Escaped newline causes blank line to be output, even if
					// it contains only a non-printing directive
					directiveSeen = false;
					get(x);	// Get the newline, skipping the escape
					goto end;
				}
			}
			if (expand && x == introChar) {
				// Handle macro expansion and stemple directives
				if (peek() == openChar) {
					// Start of a stemple directive
					if (get(x)) {	// Eat opening '('
						if (processDirective()) {
						}
						// Get first character of expansion, or character following ')'
						if (!get(x)) {
							return false;
						}
					}
				}
			}
		}
end:
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
		directiveSeen = true;

		// We've seen opening "$(", now collect first token
		string name = collectString(nameEndChars);

		vector<string> args;

		{
			// If this is an elseif directive, temporarily disable skipping so
			// we can fully expand the argument to see if this branch should be
			// taken or not.
			variable_guard<int> guard(skipping, name == "elseif" ? 0 : skipping);

			// Examine token after first word
			Token tok = getToken();
			switch (tok) {
			case ARGS:
			{
				args = collectArgs(trimArgs);
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
			case MOD:
				break;
			case CLOSE:
				break;
			case END:
			case ERR:
				break;
			}
		}

		// assert(tok == CLOSE);

		if (!skipping || name == "if" || name == "else" || name == "elseif" || name == "endif") {
			auto builtinEntry = builtins.find(name);
			if (builtinEntry != end(builtins)) {
				// Process builtin directive
				return builtinEntry->second(args);
			} else if (is_number(name)) {
				// Macro is an argument to an enclosing expansion. Look for the
				// closest 'parent' macro body and get its associated arguments.
				InStream *baseStream = findInStreamWithArgs();
				if (baseStream) {
					int index = atoi(name.c_str()) - 1;
					inStreams.push_front(make_shared<InStream>(baseStream->GetArg(index), baseStream->GetSource() + ", arg " + name));
					return true;
				}
			} else {
				// Lookup macro and insert replacement text if any
				auto macroEntry = macros.find(name);
				if (macroEntry != end(macros)) {
					string text = macroEntry->second.GetBody();
					if (text.length()) {
						inStreams.push_front(make_shared<InStream>(macroEntry->second.GetBody(), string("Body of ") + name, args));
					}
					return true;
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
	vector<string> Expander::collectArgs (bool trim)
	{
		vector<string> args;
		char c;
		do {
			string arg = collectString(argEndChars);
			args.push_back(trim ? trimWhitespace(arg) : arg);
			get(c);
		} while (c == argSepChar);
		putback(c);
		return args;
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
		if (!escaped) {	// TODO: escape before leading whitespace also preserves trailing whitespace - make it configurable?
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
		while (get(c)) {
			if (isspace(c)) {
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
			} else if (c == closeChar) {
				return CLOSE;
			} else {
				if (whitespace.length() > 0) {
					putback(c);
					if (whitespace.length() > 1) {
						inStreams.push_front(make_shared<InStream>(whitespace.substr(1, whitespace.size() - 1), "Whitespace putback"));
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
		return *inStreams.front();
	}

	//--------------------------------------------------------------------------
	// Returns a pointer rather than a reference because we want the result to
	// be nullable to denote not found.

	InStream *Expander::findInStream (const string &prefix)
	{
		for (auto &is : inStreams) {
			const string &source = is->GetSource();
			if (source.compare(0, prefix.length(), prefix) == 0) {
				return is.get();
			}
		}
		return nullptr;
	}

	//--------------------------------------------------------------------------
	// Returns a pointer rather than a reference because we want the result to
	// be nullable to denote not found.

	InStream *Expander::findInStreamWithArgs ()
	{
		for (auto &is : inStreams) {
			if (is->GetArgCount()) {
				return is.get();
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
		if (putbackStream) {
			// current character came from a stream that has since been popped
			// (because c was its last character). We keep track of the most
			// recently popped stream until the next call to get() clears it.
			inStreams.push_front(putbackStream);
			inStream().putback(c);
			putbackStream = nullptr;
		} else if (!inStreams.size() || inStream().tellg() == streampos(0)) {
			// If no current stream, or stream is at start (because c was
			// obtained from a nested stream that has now been popped), then
			// push a new stream
			inStreams.push_front(make_shared<InStream>(string(1, c), "Putback"));
		} else {
//			// Putback on the current stream
//			inStream().putback(c);
//			if (!inStream().good()) {
//				DBG("putback() c=%s failed for %s\n", printchar(c), inStream().GetSource().c_str());
//			}
			inStreams.push_front(make_shared<InStream>(string(1, c), "Putback"));
		}
		return good();
	}

	//--------------------------------------------------------------------------
	bool Expander::do_if (const vector<string> &args)
	{
		bool testResult = false;
		if (args.size() > 0) {
			testResult = textToBool(args[0]);
		}
		if (args.size() == 2 || args.size() == 3) {
			// Inline form
			if (!skipping) {
				if (testResult) {
					inStreams.push_front(make_shared<InStream>(args[1], "True branch"));
				} else if (args.size() > 2) {
					inStreams.push_front(make_shared<InStream>(args[2], "False branch"));
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
	bool Expander::do_else (const vector<string> &args)
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
	bool Expander::do_elseif (const vector<string> &args)
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
	bool Expander::do_endif (const vector<string> &args)
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
	bool Expander::do_env (const vector<string> &args)
	{
		if (args.size() && args[0].size()) {
			const char *env = getenv(args[0].c_str());
			if (env) {
				inStreams.push_front(make_shared<InStream>(env, args[0] + " environment variable"));
			}
			return true;
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_include (const vector<string> &args)
	{
		if (args.size() && args[0].size()) {
			const vector<string> restArgs(args.begin() + 1, args.end());
			inStreams.push_front(make_shared<InStream>(args[0], restArgs));
			return inStream().GetStream().good();
		} else {
			// TODO: Report error
			return false;
		}
	}

	//--------------------------------------------------------------------------
	// TODO: support :i input modifier

	bool Expander::do_equal (const vector<string> &args)
	{
		if (args.size() > 1) {
			inStreams.push_front(make_shared<InStream>(compare(args[0], args[1]) ? "1" : "0", "Equal result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "Equal error"));
			return false;
		}
	}

	//--------------------------------------------------------------------------
	// TODO: support :i input modifier

	bool Expander::do_notequal (const vector<string> &args)
	{
		if (args.size() > 1) {
			inStreams.push_front(make_shared<InStream>(!compare(args[0], args[1]) ? "1" : "0", "Notequal result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "Notequal error"));
			return false;
		}
	}

	//--------------------------------------------------------------------------
	// Uses modified ECMAScript: http://en.cppreference.com/w/cpp/regex/ecmascript
	// TODO: support :i input modifier

	bool Expander::do_match (const vector<string> &args)
	{
		if (args.size() > 1) {
			regex pattern(args[1] /*, regex_constants::icase*/);
			bool match = regex_search(args[0], pattern);
			inStreams.push_front(make_shared<InStream>(match ? "1" : "0", "Match result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "Match error"));
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_and (const vector<string> &args)
	{
		if (args.size() > 1) {
			inStreams.push_front(make_shared<InStream>(textToBool(args[0]) && textToBool(args[1]) ? "1" : "0", "And result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "And error"));
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_or (const vector<string> &args)
	{
		if (args.size() > 1) {
			inStreams.push_front(make_shared<InStream>(textToBool(args[0]) || textToBool(args[1]) ? "1" : "0", "Or result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "Or error"));
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_not (const vector<string> &args)
	{
		if (args.size() == 1) {
			inStreams.push_front(make_shared<InStream>(!textToBool(args[0]) ? "1" : "0", "Not result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "Not error"));
			return false;
		}
	}

	//--------------------------------------------------------------------------
	bool Expander::do_defined (const vector<string> &args)
	{
		if (args.size() == 1) {
			bool defined = false;
			if (is_number(args[0])) {
				// Lookup argument to an enclosing expansion. Look for the
				// closest 'parent' macro body and get its associated arguments.
				InStream *baseStream = findInStreamWithArgs();
				if (baseStream) {
					int index = atoi(args[0].c_str()) - 1;
					defined = index >= 0 && index < baseStream->GetArgCount();
				}
			} else {
				// Lookup macro
				auto macroEntry = macros.find(args[0]);
				defined = macroEntry != end(macros);
			}
			inStreams.push_front(make_shared<InStream>(defined ? "1" : "0", "Defined result"));
			return true;
		} else {
			// TODO: Report error
			inStreams.push_front(make_shared<InStream>("0", "Defined error"));
			return false;
		}
	}
}
