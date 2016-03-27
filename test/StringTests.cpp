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
	expander.SetMacro("A", "aaa");
	string expansion = expander.Expand("$(A)");
	ASSERT_EQ("aaa", expansion);
}

TEST_F(StringTests, RecursiveMacro)
{
	expander.SetMacro("A", "$(B)");
	expander.SetMacro("B", "bbb");
	string expansion = expander.Expand("$(A)");
	ASSERT_EQ("bbb", expansion);
}

TEST_F(StringTests, MultipleMacrosAndEscapes)
{
	expander.SetMacro("A", "aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccc");
	string expansion = expander.Expand("This is $$(A): (\"$(A)\")\n\tand $$(B): (\'$(B)\')\n");
	ASSERT_EQ("This is $(A): (\"aaa\")\n\tand $(B): (\'ccc\')\n", expansion);
}

TEST_F(StringTests, MacroWithArguments)
{
	expander.SetMacro("list", "[$(1), $(2), $(3)]");
	string expansion = expander.Expand("List is $(list one,two,three).");
	ASSERT_EQ("List is [one, two, three].", expansion);
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
	// Escape has to be immediately before the newline or it's just output as text
	string expansion = expander.Expand("$(A=aaa)$\n\t  $(B=bbb)$ \n$(C=ccc) $\n $(A)\n");
	ASSERT_EQ("\n\t  $ \n \n aaa\n", expansion);
}

TEST_F(StringTests, SetSpecialChars)
{
	expander.SetSpecialChars('%', '{', '}', ';');
	expander.SetMacro("list", "[%{1}, %{2}, $(3)]");
	string expansion = expander.Expand("%{A=aaa}\n%%{A}: %{A}\nList: %{list one;two;three}.\nOld-style: $(list one,two,three).");
	ASSERT_EQ("%{A}: aaa\nList: [one, two, $(3)].\nOld-style: $(list one,two,three).", expansion);
}

TEST_F(StringTests, SimplyExpandedMacros)
{
	// 1st use of B is empty, 2nd use is 'b', redefining B after use has no effect
	string expansion = expander.Expand("$(A:=$(B))$(B=b)$(A2:=$(B))$(B=x)$(A)$(A2)");
	ASSERT_EQ("b", expansion);
}

TEST_F(StringTests, RecursiveMacros)
{
	// 1st and 2nd use of B are deferred, redefinition of B after use is expanded when A and A2 are used
	string expansion = expander.Expand("$(A=$(B))$(B=b)$(A2=$(B))$(B=x)$(A)$(A2)");
	ASSERT_EQ("xx", expansion);
}

TEST_F(StringTests, SimplyExpandedAppend)
{
	string expansion = expander.Expand("$(A=aaa)$(B=bbb)$(C:=$(A))$(C:+=$(B))$(B=x)$(C)");
	ASSERT_EQ("aaabbb", expansion);
}

TEST_F(StringTests, RecursiveAppend)
{
	string expansion = expander.Expand("$(A=aaa)$(B=bbb)$(C=$(A))$(C+=$(B))$(B=x)$(C)");
	ASSERT_EQ("aaax", expansion);
}

TEST_F(StringTests, InlineIfThenElse)
{
	string expansion = expander.Expand("$(IF=$(if $(A),True,False))\n$(IF)\n$(A=aaa)\n$(IF)\n$(A=0)\n$(IF)\n");
	ASSERT_EQ("False\nTrue\nFalse\n", expansion);
}
