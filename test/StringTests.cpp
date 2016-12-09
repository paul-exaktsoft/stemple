#include "stdafx.h"

using namespace std;

extern string dataPath;

class StringTests: public ::testing::Test
{
protected:
	void SetUp ()
	{
	}

	void TearDown()
	{
		if (!tempPathname.empty()) {
			unlink(tempPathname.c_str());
		}
	}

	stemple::Expander expander;
	string tempPathname;
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

TEST_F(StringTests, SetCustomSpecialChars)
{
	expander.SetSpecialChars('\\', '%', '{', ';', '}');
	expander.SetMacro("list", "[%{1}, %{2}, $(3)]");
	string expansion = expander.Expand("%{A=aaa}\n\\%{A}: %{A}\nList: %{list one;two;three}.\nOld-style: $(list one,two,three).");
	ASSERT_EQ("%{A}: aaa\nList: [one, two, $(3)].\nOld-style: $(list one,two,three).", expansion);
}

TEST_F(StringTests, PreventLineSkippingWithCustomEscape)
{
	expander.SetSpecialChars('\\', '$', '(', ',', ')');
	// Escape has to be immediately before the newline or it's just output as text
	string expansion = expander.Expand("$(A=aaa)\\\n\t  $(B=bbb)\\ \n$(C=ccc) \\\n $(A)\n");
	ASSERT_EQ("\n\t  \\ \n \n aaa\n", expansion);
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

TEST_F(StringTests, InlineIfOnly)
{
	string expansion = expander.Expand("$(IF=$(if $(A),$(A)=>True))\n$(IF)\n$(A=aaa)\n$(IF)\n$(A=0)\n$(IF)\n");
	ASSERT_EQ("\naaa=>True\n\n", expansion);
}

TEST_F(StringTests, SimpleBlockIfTrue)
{
	string expansion = expander.Expand("Before\n$(if 1)\nTrue\n$(endif)\nAfter\n");
	ASSERT_EQ("Before\nTrue\nAfter\n", expansion);
}

TEST_F(StringTests, SimpleBlockIfFalse)
{
	string expansion = expander.Expand("Before\n$(if 0)\nTrue\n$(endif)\nAfter\n");
	ASSERT_EQ("Before\nAfter\n", expansion);
}

TEST_F(StringTests, SimpleBlockIfElseTrue)
{
	string expansion = expander.Expand("Before\n$(if 1)\nTrue\n$(else)\nFalse\n$(endif)\nAfter\n");
	ASSERT_EQ("Before\nTrue\nAfter\n", expansion);
}

TEST_F(StringTests, SimpleBlockIfElseFalse)
{
	string expansion = expander.Expand("Before\n$(if 0)\nTrue\n$(else)\nFalse\n$(endif)\nAfter\n");
	ASSERT_EQ("Before\nFalse\nAfter\n", expansion);
}

TEST_F(StringTests, NestedBlockIfElse)
{
	string input = "Before\n"
		"$(if $(A))\n"
		"  A True\n"
		"  $(if $(B))\n"
		"    B True\n"
		"  $(else)\n"
		"    B False\n"
		"  $(endif)\n"
		"$(else)\n"
		"  A False\n"
		"$(endif)\n"
		"After\n";
	expander.SetMacro("A", "1");
	expander.SetMacro("B", "0");
	string expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A True\n    B False\nAfter\n", expansion);
	expander.SetMacro("A", "0");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A False\nAfter\n", expansion);
}

TEST_F(StringTests, MultipleNestedBlockIfs)
{
	string input = "Before\n"
		"$(if $(A))\n"
		"  A True\n"
		"  $(if $(B))\n"
		"    B True\n"
		"  $(else)\n"
		"    B False\n"
		"  $(endif)\n"
		"  D=$(if $(D),True,False)\n"
		"$(else)\n"
		"  A False\n"
		"  $(if $(C))\n"
		"    C True\n"
		"  $(else)\n"
		"    C False\n"
		"  $(endif)\n"
		"  E=$(if $(E),True,False)\n"
		"$(endif)\n"
		"After\n";
	expander.SetMacro("A", "1");
	expander.SetMacro("B", "1");
	expander.SetMacro("C", "1");
	expander.SetMacro("D", "1");
	expander.SetMacro("E", "1");
	string expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A True\n    B True\n  D=True\nAfter\n", expansion);
	expander.SetMacro("B", "0");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A True\n    B False\n  D=True\nAfter\n", expansion);
	expander.SetMacro("D", "0");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A True\n    B False\n  D=False\nAfter\n", expansion);
	expander.SetMacro("A", "0");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A False\n    C True\n  E=True\nAfter\n", expansion);
	expander.SetMacro("C", "0");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A False\n    C False\n  E=True\nAfter\n", expansion);
	expander.SetMacro("E", "0");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A False\n    C False\n  E=False\nAfter\n", expansion);
}

TEST_F(StringTests, BlockElseifs)
{
	string input = "Before\n"
		"$(if $(A))\n"
		"  A True\n"
		"$(elseif $(B))\n"
		"  B True\n"
		"$(elseif $(C))\n"
		"  C True\n"
		"$(else)\n"
		"  Nothing True\n"
		"$(endif)\n"
		"After\n";
	expander.SetMacro("A", "1");
	expander.SetMacro("B", "1");
	expander.SetMacro("C", "1");
	string expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  A True\nAfter\n", expansion);
	expander.SetMacro("A", "false");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  B True\nAfter\n", expansion);
	expander.SetMacro("B", "");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  C True\nAfter\n", expansion);
	expander.SetMacro("C", "no");
	expansion = expander.Expand(input);
	ASSERT_EQ("Before\n  Nothing True\nAfter\n", expansion);
}

TEST_F(StringTests, MacroWithArgumentsEscapingDelimters)
{
	expander.SetMacro("args", "'$(1)'; '$(2)'");
	string expansion = expander.Expand("Args: $(args one$,two,three$)).");
	ASSERT_EQ("Args: 'one,two'; 'three)'.", expansion);
}

TEST_F(StringTests, MacroWithArgumentsEscapingDelimtersInsideAssignment)
{
	expander.SetMacro("args", "'$(1)'; '$(2)'");
	string expansion = expander.Expand("$(M=Args: $(args one$,two,three$$$)).)$(M)");
	ASSERT_EQ("Args: 'one,two'; 'three)'.", expansion);
}

TEST_F(StringTests, MacroWithTrimmedArguments)
{
	expander.SetMacro("args", "'$(1)'; '$(2)'; '$(3)'; '$(4)'");
	string expansion = expander.Expand("Args: $(args\tone,  two ,\nthree\t ,four \t).");
	ASSERT_EQ("Args: 'one'; 'two'; 'three'; 'four'.", expansion);
}

TEST_F(StringTests, MacroWithNonTrimmedArguments)
{
	expander.SetMacro("args", "'$(1)'; '$(2)'; '$(3)'");
	// NOTE: Doubling the last escape stops get() from seeing $\n and eating the escape before collectArgs() can see it
	string expansion = expander.Expand("Args: $(args\tone,   $ two ,$$\nthree\t ).");
	ASSERT_EQ("Args: 'one'; ' two '; '\nthree\t '.", expansion);
}

TEST_F(StringTests, PathEnvVar)
{
	string expansion = expander.Expand("$(env PATH)");
	ASSERT_TRUE(expansion.length() > 0);
}

TEST_F(StringTests, Include)
{
	tempPathname = tmpnam(nullptr);
	ofstream ofs(tempPathname);
	ofs << "$(A)$(1)" << endl;
	ofs.close();
	expander.SetMacro("A", "aaa");
	string expansion = expander.Expand("$(include " + tempPathname + ", bbb)");
	ASSERT_EQ("aaabbb\n", expansion);
	expansion = expander.Expand("$(include " + tempPathname + ")");
	ASSERT_EQ("aaa\n", expansion);
}

TEST_F(StringTests, NestedIncludes)
{
	string expansion = expander.Expand("$(include " + dataPath + "/test1.txt)");
	ASSERT_EQ("This is test4.txt\n", expansion);
}

TEST_F(StringTests, Equal)
{
	expander.SetMacro("A", "aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccc");
	string expansion = expander.Expand("$(if $(equal $(A), aaax), True, False), $(if $(equal $(B), ccc), True, False)");
	ASSERT_EQ("False, True", expansion);
}

TEST_F(StringTests, NotEqual)
{
	expander.SetMacro("A", "aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccc");
	string expansion = expander.Expand("$(if $(notequal $(A), aaax), True, False), $(if $(notequal $(B), ccc), True, False)");
	ASSERT_EQ("True, False", expansion);
}

TEST_F(StringTests, Match)
{
	expander.SetMacro("A", "aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccc");
	string expansion = expander.Expand("$(if $(match $(A), a*.x), True, False), $(if $(match $(B), c+.[a-z]), True, False)");
	ASSERT_EQ("False, True", expansion);
}

TEST_F(StringTests, And)
{
	string expansion = expander.Expand("$(if $(and yes, yes), True, False), "
									   "$(if $(and yes, no), True, False), "
									   "$(if $(and no, yes), True, False), "
									   "$(if $(and no, no), True, False)");
	ASSERT_EQ("True, False, False, False", expansion);
}

TEST_F(StringTests, Or)
{
	string expansion = expander.Expand("$(if $(or yes, yes), True, False), "
									   "$(if $(or yes, no), True, False), "
									   "$(if $(or no, yes), True, False), "
									   "$(if $(or no, no), True, False)");
	ASSERT_EQ("True, True, True, False", expansion);
}

TEST_F(StringTests, Not)
{
	string expansion = expander.Expand("$(if $(not yes), True, False), "
									   "$(if $(not no), True, False)");
	ASSERT_EQ("False, True", expansion);
}

TEST_F(StringTests, Defined)
{
	expander.SetMacro("A", "aaa");
	string expansion = expander.Expand("$(if $(defined A), True, False), $(if $(defined B), True, False)");
	ASSERT_EQ("True, False", expansion);
}

TEST_F(StringTests, ArgDefined)
{
	expander.SetMacro("A", "$(if $(defined 1), True, False), $(if $(defined 2), True, False)");
	string expansion = expander.Expand("$(A aaa)");
	ASSERT_EQ("True, False", expansion);
}

TEST_F(StringTests, NoNoExpandModifier)
{
	expander.SetMacro("A", "$(1), '$(2)'");
	expander.SetMacro("B", ")");
	string expansion = expander.Expand("($(A $(B),aaa)");
	ASSERT_EQ("(, '',aaa)", expansion);
}

TEST_F(StringTests, NoExpandModifier)
{
	expander.SetMacro("A", "$(1), '$(2)'");
	expander.SetMacro("B", ")");
	string expansion = expander.Expand("($(A:x $(B),aaa)");
	ASSERT_EQ("(), 'aaa'", expansion);
}

TEST_F(StringTests, NoExpandModifierRecursive)
{
	expander.SetMacro("A", "$(1), '$(2)'");
	expander.SetMacro("B", ")");
	expander.SetMacro("C", "$(1)");
	string expansion = expander.Expand("($(A:x $(C $(B)),aaa)");
	ASSERT_EQ("(), 'aaa'", expansion);
}

TEST_F(StringTests, EqualWithIgnoreCaseModifier)
{
	expander.SetMacro("A", "Aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccC");
	string expansion = expander.Expand("$(if $(equal:i $(A), aaAx), True, False), $(if $(equal:i $(B), Ccc), True, False)");
	ASSERT_EQ("False, True", expansion);
}

TEST_F(StringTests, NotEqualWithIgnoreCaseModifier)
{
	expander.SetMacro("A", "Aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccC");
	string expansion = expander.Expand("$(if $(notequal:i $(A), Aaax), True, False), $(if $(notequal:i $(B), Ccc), True, False)");
	ASSERT_EQ("True, False", expansion);
}

TEST_F(StringTests, MatchWithIgnoreCaseModifier)
{
	expander.SetMacro("A", "Aaa");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccC");
	string expansion = expander.Expand("$(if $(match:i $(A), a*.x), True, False), $(if $(match:i $(B), C+.[a-z]), True, False)");
	ASSERT_EQ("False, True", expansion);
}

TEST_F(StringTests, NoTrimModifier)
{
	expander.SetMacro("args", "'$(1)'; '$(2)'; '$(3)'; '$(4)'");
	string expansion = expander.Expand("Args: $(args:n\tone,  two ,\nthree\t ,four \t).");
	ASSERT_EQ("Args: 'one'; '  two '; '\nthree\t '; 'four \t'.", expansion);
}

TEST_F(StringTests, MultipleModifiers)
{
	expander.SetMacro("A", " aaax");
	expander.SetMacro("B", "$(C)");
	expander.SetMacro("C", "ccC");
	string expansion = expander.Expand("$(if $(notequal:i:n $(A), Aaax), True, False), $(if $(notequal:i:n:x $(B), Ccc), True, False)");
	ASSERT_EQ("False, True", expansion);
}

TEST_F(StringTests, NoQuoteModifier)
{
	expander.SetMacro("A", "1='$(1)'; 2='$(2)'; 3='$(3)'");
	expander.SetMacro("B", ")");
	expander.SetMacro("C", "bbb, ccc");
	expander.SetMacro("D", "$(E xxx,yyy)");
	string expansion = expander.Expand("$(A $(B), $(C), $(D)) - B='$(B)', C='$(C)' D='$(D)'");
	ASSERT_EQ("1=''; 2=''; 3='', bbb, ccc, ) - B=')', C='bbb, ccc' D=''", expansion);
}

TEST_F(StringTests, QuoteModifier)
{
	expander.SetMacro("A", "1='$(1)'; 2='$(2)'; 3='$(3:q)'");
	expander.SetMacro("B", ")");
	expander.SetMacro("C", "bbb, ccc");
	expander.SetMacro("D", "$(E xxx,yyy)");
	string expansion = expander.Expand("$(A $(B:q), $(C:q), $(D:q)) - B='$(B:q)', C='$(C:q)' D='$(D:q)'");
	ASSERT_EQ("1=')'; 2='bbb, ccc'; 3='$(E xxx,yyy)' - B=')', C='bbb, ccc' D='$(E xxx,yyy)'", expansion);
}
