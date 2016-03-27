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
		//----------------------------------------------------------------------
		InStream (const std::string &input, const std::string &source,
				  const ArgList &args = {}):
			base(std::make_shared<std::istringstream>(input)),
			position(source),
			args(args)
		{
		}

		//----------------------------------------------------------------------
		InStream (const std::string &input, const Position &position):
			base(std::make_shared<std::istringstream>(input)),
			position(position),
			args(args)
		{
		}

		//----------------------------------------------------------------------
		InStream (const std::string &pathname, const ArgList &args = {},
				  std::ios_base::openmode mode = std::ios_base::in):
			base(std::make_shared<std::ifstream>(pathname, mode)),
			position(pathname),
			args(args)
		{
		}

		//----------------------------------------------------------------------
		InStream (std::istream &stream, const std::string &source,
				  const ArgList &args = {}) :
			base(std::make_shared<std::istream>(stream.rdbuf())),
			position(source),
			args(args)
		{
		}

		//----------------------------------------------------------------------
		virtual ~InStream ()
		{
		}

		//----------------------------------------------------------------------
		std::istream &GetStream ()
		{
			return *base;
		}

		//----------------------------------------------------------------------
		const Position &GetPosition ()
		{
			return position;
		}

		//----------------------------------------------------------------------
		const std::string &GetSource ()
		{
			return position.Source;
		}

		//----------------------------------------------------------------------
		const int GetArgCount ()
		{
			return (int)args.size();
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
			position.Update(c);
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
		Position						position;
		const ArgList					args;
	};
}

#endif	// __stemple__InStream__
