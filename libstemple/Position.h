// Position
// Stream position, including source (file or macro), line, column, etc.
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__Position__
#define __stemple__Position__

#include "stdafx.h"

namespace stemple
{
	struct Position
	{
		const std::string Source;
		int Offset;
		int Line;
		int Column;			// Takes into account tabs, UTF-8, etc.
		static int TabSize;

		//----------------------------------------------------------------------
		Position (const std::string &source):
			Source(source),
			Offset(-1),
			Line(0),
			Column(0),
			nextLine(1),
			nextColumn(1)
		{
		}

		//----------------------------------------------------------------------
		Position (const Position &other):
			Source(other.Source),
			Offset(other.Offset),
			Line(other.Line),
			Column(other.Column),
			nextLine(other.nextLine),
			nextColumn(other.nextColumn)
		{
		}

		//----------------------------------------------------------------------
		void Update (char c)
		{
			++ Offset;
			Line = nextLine;
			Column = nextColumn;
			if (c == '\n') {
				++ nextLine;
				nextColumn = 1;
			} else if (c == '\t') {
				nextColumn = (nextColumn / TabSize + 1) * TabSize - 1;
			} else {
				// TODO: Take UTF-8, etc, into account - need to use 32-bit chars?
				++ nextColumn;
			}
		}

		//----------------------------------------------------------------------
		Position &Putback ()
		{
			-- Offset;
			nextLine = Line;
			nextColumn = Column;
			return *this;
		}

		//----------------------------------------------------------------------
		std::string GetString () const
		{
			return stringf("%s, line %d, column %d", Source.c_str(), Line, Column);
		}

		//----------------------------------------------------------------------
		const char *GetCString () const
		{
			static std::string s;
			s = GetString();
			return s.c_str();
		}

	private:
		int nextLine;
		int nextColumn;
	};
}

#endif	// __stemple__Position__
