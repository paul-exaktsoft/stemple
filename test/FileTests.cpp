#include "stdafx.h"

using namespace std;

class FileTests: public ::testing::Test
{
protected:
	void SetUp ()
	{
	}

	void TearDown()
	{
		if (!tempInPathname.empty()) {
			unlink(tempInPathname.c_str());
		}
		if (!tempOutPathname.empty()) {
			unlink(tempOutPathname.c_str());
		}
	}

	stemple::Expander expander;
	string tempInPathname;
	string tempOutPathname;
};

TEST_F(FileTests, Expand)
{
	// Create input file and rewind
	tempInPathname = tmpnam(nullptr);
	fstream ifs(tempInPathname, ios::in|ios::out|ios::trunc);
	if (!ifs) FAIL() << "Can't create input file.";
	ifs << "$(A)" << endl;
	ifs.seekg(0, ios_base::beg);

	// Create output file
	tempOutPathname = tmpnam(nullptr);
	fstream ofs(tempOutPathname, ios::in|ios::out|ios::trunc);
	if (!ofs) FAIL() << "Can't create output file.";

	// Do expansion
	expander.SetMacro("A", "aaa");
	bool result = expander.Expand(ifs, tempInPathname, ofs);
	ASSERT_TRUE(result);

	// Rewind and read output file
	ofs.seekg(0, ios_base::beg);
	string expansion((istreambuf_iterator<char>(ofs)), (istreambuf_iterator<char>()));

	// Check result
	ASSERT_EQ("aaa\n", expansion);
}
