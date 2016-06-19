// stemple.h
// Top-level C++ or C API header for the stemple library
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__
#define __stemple__

#if defined __cplusplus

// The C++ API...
#include "Expander.h"

extern "C" {
#endif	// __cplusplus

// The C API...
#include <stdio.h>
#include <stdbool.h>	// Requires C99

typedef struct stemple_Expander stemple_Expander;

stemple_Expander *stemple_CreateExpander ();

void stemple_DestroyExpander (stemple_Expander *expander);

char *stemple_ExpandString (stemple_Expander *expander, const char *input);

bool stemple_ExpandFile (stemple_Expander *expander, FILE *input, const char *inputName, FILE *output);

void stemple_SetMacro (stemple_Expander *expander, const char *name, const char *body);

void stemple_SetMacroSimple (stemple_Expander *expander, const char *name, const char *body);

void stemple_SetSpecialChars (stemple_Expander *expander, char escape, char intro, char open, char argSep, char close);

#if defined __cplusplus
}
#endif	// __cplusplus

#endif	// __stemple__
