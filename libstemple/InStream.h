// InStream
// Input stream tagged with annotation like source name (file or macro), line,
// column, etc. An InStream is created for each expansion of a macro body and
// holds the list of arguments given to the macro directive.
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__InStream__
#define __stemple__InStream__

#include "stdafx.h"

namespace stemple
{
	//==========================================================================
	//==========================================================================
	class InStream
	{
	public:
		//----------------------------------------------------------------------
		InStream (const Position &position, const ArgList &args) :
			position(position),
			args(args)
		{
		}

		//----------------------------------------------------------------------
		virtual ~InStream ()
		{
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
		virtual bool get (char &c) = 0;

		//----------------------------------------------------------------------
		virtual int peek () = 0;

		//----------------------------------------------------------------------
		virtual bool good () = 0;

		//----------------------------------------------------------------------
		virtual bool eof () = 0;

		//----------------------------------------------------------------------
		virtual bool putback (const char &ch) = 0;

	protected:
		Position		position;
		const ArgList	args;
	};

	//==========================================================================
	//==========================================================================
	class StreamStream : public InStream
	{
	public:
		//----------------------------------------------------------------------
		StreamStream (std::istream	&base, const Position &position, const ArgList &args) :
			InStream(position, args),
			base(base)
		{
		}

		//----------------------------------------------------------------------
		virtual ~StreamStream ()
		{
		}

		//----------------------------------------------------------------------
		bool get (char &c)
		{
			base.get(c);
			position.Update(c);
			return base.good();
		}

		//----------------------------------------------------------------------
		int peek ()
		{
			return base.peek();
		}

		//----------------------------------------------------------------------
		bool good ()
		{
			return base.good();
		}

		//----------------------------------------------------------------------
		bool eof ()
		{
			return base.eof();
		}

		//----------------------------------------------------------------------
		bool putback (const char &ch)
		{
			base.putback(ch);
			return base.good();
		}

	protected:
		std::istream	&base;
	};

	//==========================================================================
	//==========================================================================
	class FileStream : public StreamStream
	{
	public:
		//----------------------------------------------------------------------
		FileStream (const std::string &pathname, const ArgList &args = {},
					std::ios_base::openmode mode = std::ios_base::in) :
			stream(pathname, mode),
			StreamStream(stream, pathname, args)
		{
		}

		//----------------------------------------------------------------------
		virtual ~FileStream ()
		{
		}

	protected:
		std::ifstream	stream;
	};

	//==========================================================================
	//==========================================================================
	class StringStream : public StreamStream
	{
	public:
		//----------------------------------------------------------------------
		StringStream (const std::string &input, const Position &position,
					  const ArgList &args = {}) :
			stream(input),
			StreamStream(stream, position, args)
		{
		}

		//----------------------------------------------------------------------
		virtual ~StringStream ()
		{
		}

	protected:
		std::istringstream	stream;
	};

	//==========================================================================
	//==========================================================================
	class CopiedStream : public StreamStream
	{
	public:
		//----------------------------------------------------------------------
		CopiedStream (std::istream &input, const Position &position,
					  const ArgList &args = {}) :
			stream(input.rdbuf()),
			StreamStream(stream, position, args)
		{
		}

		//----------------------------------------------------------------------
		virtual ~CopiedStream ()
		{
		}

	protected:
		std::istream	stream;
	};

	//==========================================================================
	//==========================================================================
	class CharStream : public InStream
	{
	public:
		//----------------------------------------------------------------------
		CharStream (char c, const Position &position) :
			InStream(position, {}),
			pbc(c),
			done(false)
		{
		}

		//----------------------------------------------------------------------
		virtual ~CharStream ()
		{
		}

		//----------------------------------------------------------------------
		bool get (char &c)
		{
			if (!done) {
				c = pbc;
				done = true;
				position.Update(pbc);
				return true;
			} else {
				c = std::char_traits<char>::eof();
				return false;
			}
		}

		//----------------------------------------------------------------------
		int peek ()
		{
			return !done ? pbc : std::char_traits<char>::eof();
		}

		//----------------------------------------------------------------------
		bool good ()
		{
			return !done;
		}

		//----------------------------------------------------------------------
		bool eof ()
		{
			return !done;
		}

		//----------------------------------------------------------------------
		bool putback (const char &ch)
		{
			if (done) {
				done = false;
				return true;
			} else {
				return false;
			}
		}

	protected:
		char pbc;
		bool done;
	};
}

#endif	// __stemple__InStream__
