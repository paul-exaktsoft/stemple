// ctest.c : Defines the entry point for the console application.
//

#include "Unity/src/unity.h"

extern void test_SetMacro (void);
extern void test_SetMacroSimple (void);
extern void test_SetSpecialChars (void);
extern void test_ExpandFile (void);

int main (int argc, char **argv)
{
	UNITY_BEGIN();
	RUN_TEST(test_SetMacro);
	RUN_TEST(test_SetMacroSimple);
	RUN_TEST(test_SetSpecialChars);
	RUN_TEST(test_ExpandFile);
	return UNITY_END();
}
