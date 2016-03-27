// InStream
// Input stream tagged with annotation like source (file or macro), line, column, etc.
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__InStream__
#define __stemple__InStream__

#include "stdafx.h"

namespace stemple
{
	class InStream
	{
	public:
		struct Position
		{
			int Offset;
			int Line;
			int Column;	// Takes into account tabs, UTF-8, etc.
			static int TabSize;
			Position (): Offset(0), Line(0), Column(0)
			{
			}
			Position (const Position &other) : Offset(other.Offset), Line(other.Line), Column(other.Column)
			{
			}
			void Increment (char c)
			{
				++ Offset;
				if (c == '\n') {
					++ Line;
					Column = 0;
				} else if (c == '\t') {
					Column = (Column / TabSize + 1) * TabSize - 1;
				} else {
					++ Column;
				}
			}
		};

		//----------------------------------------------------------------------
		InStream (const std::string &input, const std::string &source,
				  const std::vector<std::string> &args = {}, bool isolated = false) :
			base(std::make_shared<std::istringstream>(input)),
			source(source),
			args(args),
			isolated(isolated)
		{
		}

		//----------------------------------------------------------------------
		InStream (const std::string &pathname, const std::vector<std::string> &args = {},
				  std::ios_base::openmode mode = std::ios_base::in, bool isolated = false):
			base(std::make_shared<std::ifstream>(pathname, mode)),
			source(pathname),
			args(args),
			isolated(isolated)
		{
		}

		//----------------------------------------------------------------------
		virtual ~InStream ()
		{
		}

		//----------------------------------------------------------------------
		bool IsIsolated ()
		{
			return isolated;
		}

		//----------------------------------------------------------------------
		std::istream &GetStream ()
		{
			return *base;
		}

		//----------------------------------------------------------------------
		const std::string &GetSource ()
		{
			return source;
		}

		//----------------------------------------------------------------------
		const std::string GetArg (int index)
		{
			return index >= 0 && (size_t)index < args.size() ? args[index] : "";
		}

		//----------------------------------------------------------------------
		bool get (char &c)
		{
			base->get(c);
			position.Increment(c);
			return base->good();
		}

		//----------------------------------------------------------------------
		int peek ()
		{
			return base->peek();
		}

		//----------------------------------------------------------------------
		bool good ()
		{
			return base->good();
		}

		//----------------------------------------------------------------------
		bool eof ()
		{
			return base->eof();
		}

		//----------------------------------------------------------------------
		bool fail ()
		{
			return base->fail();
		}

		//----------------------------------------------------------------------
		bool bad ()
		{
			return base->bad();
		}

		//----------------------------------------------------------------------
		bool putback (const char &ch)
		{
			base->putback(ch);
			return base->good();
		}

		//----------------------------------------------------------------------
		std::streampos tellg ()
		{
			return base->tellg();
		}

	protected:
		std::shared_ptr<std::istream>	base;
		const std::string				source;
		bool							isolated;
		Position						position;
		std::vector<std::string>		args;
	};
}

#endif	// __stemple__InStream__
