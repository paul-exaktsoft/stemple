// stemple.cpp : Defines the entry point for the console application.

#include "stdafx.h"

using namespace std;

int main ()
{
	stemple::Expander expander;
	expander.AddMacro("A", "aaa");
	expander.AddMacro("B", "$(C)");
	expander.AddMacro("C", "ccc");
	expander.AddMacro("list", "[$(1), $(2), $(3)]");
	string inputs[] = {
//		"ABC",
//		"$(A)",
//		"$(B)",
//		"This is $$(A): (\"$(A)\")\n\tand $$(B): (\'$(B)\')\n",
		"List: $(list one, two, three)."
	};
	for (string input : inputs) {
		string expansion = expander.Expand(input);
		cout << expansion << endl;
	}
	return 0;
}
