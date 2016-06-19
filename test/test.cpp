// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <gtest/gtest.h>

std::string dataPath;

int main (int argc, char **argv)
{
#if 0
	char **argv2 = new char*[argc + 1];
	for (int i = 0; i < argc; ++ i) argv2[i] = argv[i];
	argv2[argc] = (char *)"--gtest_filter=StringTests.NestedIncludes";
	argc +=1;
	argv = argv2;
#endif
	::testing::InitGoogleTest(&argc, argv);
	if (argc > 1) {
		dataPath = argv[1];
		std::string slashes("/\\");
		if (slashes.find(dataPath[dataPath.length() - 1]) != std::string::npos) {
			dataPath.erase(dataPath.length() - 1);
		}
	}
	return RUN_ALL_TESTS();
}
