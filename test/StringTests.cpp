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
	expander.AddMacro("A", "aaa");
	expander.AddMacro("B", "$(C)");
	expander.AddMacro("C", "ccc");
	string expansion = expander.Expand("$(B)");
	ASSERT_EQ("ccc", expansion);
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
