// stemple.cpp
// Top-level C API for the stemple library
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#include "stdafx.h"

//------------------------------------------------------------------------------
stemple_Expander *stemple_CreateExpander ()
{
	try {
		return reinterpret_cast<stemple_Expander *>(new stemple::Expander());
	} catch (...) {
	}
	return 0;
}

//------------------------------------------------------------------------------
void stemple_DestroyExpander (stemple_Expander *expander)
{
	if (expander) {
		try {
			delete reinterpret_cast<stemple::Expander *>(expander);
		} catch (...) {
		}
	}
}

//------------------------------------------------------------------------------
char *stemple_ExpandString (stemple_Expander *expander, const char *input)
{
	if (expander) {
		try {
			return strdup(reinterpret_cast<stemple::Expander *>(expander)->Expand(input ? input : "").c_str());
		} catch (...) {
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
bool stemple_ExpandFile (stemple_Expander *expander, FILE *input, const char *inputName, FILE *output)
{
	if (expander) {
		try {
			return reinterpret_cast<stemple::Expander *>(expander)->Expand(stemple::cstream(input), inputName, stemple::cstream(output));
		} catch (...) {
		}
	}
	return false;
}

//------------------------------------------------------------------------------
void stemple_SetMacro (stemple_Expander *expander, const char *name, const char *body)
{
	if (expander) {
		try {
			reinterpret_cast<stemple::Expander *>(expander)->SetMacro(name, body, false);
		} catch (...) {
		}
	}
}

//------------------------------------------------------------------------------
void stemple_SetMacroSimple (stemple_Expander *expander, const char *name, const char *body)
{
	if (expander) {
		try {
			reinterpret_cast<stemple::Expander *>(expander)->SetMacro(name, body, true);
		} catch (...) {
		}
	}
}

//------------------------------------------------------------------------------
void stemple_SetSpecialChars (stemple_Expander *expander, char escape, char intro, char open, char argSep, char close)
{
	if (expander) {
		try {
			reinterpret_cast<stemple::Expander *>(expander)->SetSpecialChars(escape, intro, open, argSep, close);
		} catch (...) {
		}
	}
}
