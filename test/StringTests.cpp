#include "stdafx.h"

using namespace std;

class StringTests: public ::testing::Test
{
protected:
	void SetUp ()
	{
	}

	void TearDown()
	{
	}

	stemple::Expander expander;
};

TEST_F(StringTests, PassTextUnaltered)
{
	string expansion = expander.Expand("Abc $def");
	ASSERT_EQ("Abc $def", expansion);
}

TEST_F(StringTests, SimplestMacro)
{
	expander.AddMacro("A", "aaa");
	string expansion = expander.Expand("$(A)");
	ASSERT_EQ("aaa", expansion);
}

TEST_F(StringTests, RecursiveMacro)
{
	expander.AddMacro("A", "$(B)");
	expander.AddMacro("B", "bbb");
	string expansion = expander.Expand("$(A)");
	ASSERT_EQ("bbb", expansion);
}

TEST_F(StringTests, MultipleMacrosAndEscapes)
{
	expander.AddMacro("A", "aaa");
	expander.AddMacro("B", "$(C)");
	expander.AddMacro("C", "ccc");
	string expansion = expander.Expand("This is $$(A): (\"$(A)\")\n\tand $$(B): (\'$(B)\')\n");
	ASSERT_EQ("This is $(A): (\"aaa\")\n\tand $(B): (\'ccc\')\n", expansion);
}

TEST_F(StringTests, MacroWithArguments)
{
	expander.AddMacro("list", "[$(1), $(2), $(3)]");
	string expansion = expander.Expand("List: $(list one,two,three).");
	ASSERT_EQ("List: [one, two, three].", expansion);
}

TEST_F(StringTests, Assignment)
{
	string expansion = expander.Expand("$(A=aaa)$(A)");
	ASSERT_EQ("aaa", expansion);
}

TEST_F(StringTests, PassBlankLines)
{
	string expansion = expander.Expand("\n \n\t \n  Hello\n");
	ASSERT_EQ("\n \n\t \n  Hello\n", expansion);
}

TEST_F(StringTests, SkipNonPrintingDirectiveLines)
{
	string expansion = expander.Expand("$(A=aaa)\n\t  $(B=bbb) \n$(C=ccc) \n $(A)\n");
	ASSERT_EQ(" aaa\n", expansion);
}

TEST_F(StringTests, PreventLineSkipping)
{
	string expansion = expander.Expand("$(A=aaa)$\n\t  $(B=bbb)$ \n$(C=ccc) $\n $(A)\n");
	ASSERT_EQ("\n\t  $ \n \n aaa\n", expansion);	// NOTE: Escape has to be immediately before the newline
}

TEST_F(StringTests, SetSpecialChars)
{
	expander.SetSpecialChars('%', '{', '}', ';');
	expander.AddMacro("list", "[%{1}, %{2}, $(3)]");
	string expansion = expander.Expand("%{A=aaa}\n%%{A}: %{A}\nList: %{list one;two;three}.\nOld-style: $(list one,two,three).");
	ASSERT_EQ("%{A}: aaa\nList: [one, two, $(3)].\nOld-style: $(list one,two,three).", expansion);
}
