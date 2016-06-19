#include <libstemple/stemple.h>
#include "Unity/src/unity.h"
#include <malloc.h>
#include <string.h>

#ifdef _WIN32
#define strdup _strdup
#endif

static stemple_Expander *expander = NULL;
static char *tempInPathname = NULL;
static char *tempOutPathname = NULL;

void setUp (void)
{
	expander = stemple_CreateExpander();
}

void tearDown (void)
{
	stemple_DestroyExpander(expander);
	expander = NULL;
	if (tempInPathname) {
		_unlink(tempInPathname);
		free(tempInPathname);
		tempInPathname = NULL;
	}
	if (tempOutPathname) {
		_unlink(tempOutPathname);
		free(tempOutPathname);
		tempOutPathname = NULL;
	}
}

void test_SetMacro (void)
{
	stemple_SetMacro(expander, "A", "aaa");
	stemple_SetMacro(expander, "B", "$(C)");
	stemple_SetMacro(expander, "C", "ccc");
	char *expansion = stemple_ExpandString(expander, "This is $$(A): (\"$(A)\")\n\tand $$(B): (\'$(B)\')\n");
	TEST_ASSERT_EQUAL_STRING("This is $(A): (\"aaa\")\n\tand $(B): (\'ccc\')\n", expansion);
	free(expansion);
}

void test_SetMacroSimple (void)
{
	stemple_SetMacro(expander, "A", "aaa");
	stemple_SetMacroSimple(expander, "B", "$(A)");
	stemple_SetMacro(expander, "A", "ccc");
	char *expansion = stemple_ExpandString(expander, "This is $$(A): (\"$(A)\")\n\tand $$(B): (\'$(B)\')\n");
	TEST_ASSERT_EQUAL_STRING("This is $(A): (\"ccc\")\n\tand $(B): (\'aaa\')\n", expansion);
	free(expansion);
}

void test_SetSpecialChars (void)
{
	stemple_SetSpecialChars(expander, '\\', '%', '{', ';', '}');
	stemple_SetMacro(expander, "list", "[%{1}, %{2}, $(3)]");
	char *expansion = stemple_ExpandString(expander, "%{A=aaa}\n\\%{A}: %{A}\nList: %{list one;two;three}.\nOld-style: $(list one,two,three).");
	TEST_ASSERT_EQUAL_STRING("%{A}: aaa\nList: [one, two, $(3)].\nOld-style: $(list one,two,three).", expansion);
	free(expansion);
}

void test_ExpandFile (void)
{
	char *input, *expansion;
	FILE *fin, *fout;
	int numBytes;

	// Create input file
	tempInPathname = strdup(tmpnam(NULL));	// tmpnam returns ptr to internal static buffer - subsequent calls overwrite, so copy
	fin = fopen(tempInPathname, "w+");
	TEST_ASSERT_NOT_NULL(fin);
	input = "$(A)\n";
	fwrite(input, sizeof(char), strlen(input), fin);
	// Rewind to beginning of input
	fseek(fin, 0, SEEK_SET);

	// Create output file
	tempOutPathname = strdup(tmpnam(NULL));
	fout = fopen(tempOutPathname, "w+");
	TEST_ASSERT_NOT_NULL(fout);

	// Do expansion
	stemple_SetMacro(expander, "A", "aaa");
	stemple_ExpandFile(expander, fin, tempInPathname, fout);

	// Rewind output file and load contents
	fseek(fout, 0, SEEK_SET);
	expansion = malloc(1024);
	numBytes = fread(expansion, sizeof(char), 1024, fout);
	expansion[numBytes] = '\0';

	// Check result
	TEST_ASSERT_EQUAL_STRING("aaa\n", expansion);

	// Release resources
	free(expansion);
	fclose(fin);
	fclose(fout);
}
