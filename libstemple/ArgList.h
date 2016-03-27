// ArgList
// Argument list passed to directives
//
// Copyright © 2016 by Paul Ashdown. All Rights Reserved.

#pragma once
#ifndef __stemple__ArgList__
#define __stemple__ArgList__

#include "stdafx.h"

namespace stemple
{
	// NOTE: This should really be vector<const string> but that causes
	// compiler errors.

	typedef std::vector<std::string> ArgList;
}

#endif // __stemple__ArgList__
