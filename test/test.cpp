// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <gtest/gtest.h>

int main (int argc, char **argv)
{
#if 0
	argv[1] = (char *)"--gtest_filter=StringTests.Include";
	argc = 2;
#endif
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
