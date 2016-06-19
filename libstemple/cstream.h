// cstream
// Derived C++ streambuf & iostream classes to provide a wrapper around C-style
// FILE* handles, allowing FILE* to be used wherever a C++ iostream is required.
//
// Based on Dr. Dobbs article "The Standard Librarian: IOStreams and Stdio" by
// Matthew H. Austern, November 01, 2000
// http://www.drdobbs.com/the-standard-librarian-iostreams-and-std/184401305
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__cstream__
#define __stemple__cstream__

#include <cstdio>
#include <iostream>

namespace stemple
{
	class cstreambuf: public std::streambuf
	{
	public:
		cstreambuf (FILE *f):
			std::streambuf(),
			fptr(f)
		{
		}

	protected:
		virtual int overflow (int c = EOF)
		{
			return c != EOF ? fputc(c, fptr) : EOF;
		}

		virtual int underflow ()
		{
			int c = getc(fptr);
			if (c != EOF) {
				ungetc(c, fptr);
			}
			return c;
		}

		virtual int uflow ()
		{
			return getc(fptr);
		}

		virtual int pbackfail (int c = EOF)
		{
			return c != EOF ? ungetc(c, fptr) : EOF;
		}

		virtual int sync ()
		{
			return fflush(fptr);
		}

		virtual pos_type seekoff (off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
		{
			fseek(fptr, (long)off, dir == std::ios_base::beg ? SEEK_SET : (dir == std::ios_base::end ? SEEK_END : SEEK_CUR));
			return ftell(fptr);
		}

		virtual pos_type seekpos (pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
		{
			fpos_t fpos = pos;
			fsetpos(fptr, &fpos);
			return ftell(fptr);
		}

	private:
		FILE *fptr;
	};

	//--------------------------------------------------------------------------
	class cstream: public std::iostream
	{
	public:
		cstream ():
			std::iostream(&buf),
			buf(0)
		{
		}

		cstream (FILE *fptr):
			std::iostream(&buf),
			buf(fptr)
		{
		}

		cstreambuf *rdbuf () const
		{
			return &buf;
		}

	private:
		mutable cstreambuf buf;
	};
}

#endif	// __stemple__cstream__
