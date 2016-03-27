// stemple.cpp : Defines the entry point for the console application.

#include "stdafx.h"

using namespace std;

void usage ();
void version ();
void specialChars (int, char **, int &);

static stemple::Expander expander;
static std::string program;

//------------------------------------------------------------------------------
int main (int argc, char **argv)
{
	std::string input;
	std::string output;
	std::unique_ptr<std::istream> inputStream;
	std::unique_ptr<std::ostream> outputStream;

	if (argc) program = argv[0];

	// Parse arguments
	for (int i = 1; i < argc; ++ i) {
		std::string arg = argv[i];
		if (arg.length() > 1 && arg[0] == '-') {
			if ((arg[1] == 'd' || arg[1] == 'D') || arg == "--define") {
				std::string definition;
				if (arg != "--define" && arg.length() > 2) {
					definition = arg.substr(2);
				} else {
					++ i;
					if (i < argc) definition = argv[i];
				}
				std::string name;
				std::string body;
				size_t eq = definition.find('=');
				if (eq != std::string::npos) {
					name = definition.substr(0, eq);
					if (definition.length() > eq + 1) {
						body = definition.substr(eq + 1);
					}
				}
				expander.SetMacro(name, body);
			} else if (arg == "--help") {
				usage();
			} else if (arg == "--version") {
				version();
			} else if (arg == "--chars") {
				++ i;
				specialChars(argc, argv, i);
			} else if (arg[1] != '-') {
				for (size_t c = 1; c < arg.length(); ++ c) {
					if (arg[c] == 'h') {
						usage();
					} else if (arg[c] == 'v') {
						version();
					} else if (arg[c] == 'c') {
						++ i;
						specialChars(argc, argv, i);
					} else {
						usage();
					}
				}
			} else {
				usage();
			}
		} else if (input.empty()) {
			input = arg;
		} else if (output.empty()) {
			output = arg;
		} else {
			usage();
		}
	}

	// Open input
	if (input.empty() || input == "-") {
		inputStream = std::make_unique<std::istream>(std::cin.rdbuf());
		input = "Standard Input";
	} else {
		inputStream = std::make_unique<std::ifstream>(input);
	}
	if (!inputStream->good()) {
		std::cerr << "Cannot open " << input << std::endl;
		exit(1);
	}

	// Open output
	if (output.empty() || output == "-") {
		outputStream = std::make_unique<std::ostream>(std::cout.rdbuf());
		output = "Standard Output";
	} else {
		outputStream = std::make_unique<std::ofstream>(output);
	}
	if (!outputStream->good()) {
		std::cerr << "Cannot open " << output << std::endl;
		exit(1);
	}

	// Process
	try {
		if (!expander.Expand(*inputStream, input, *outputStream)) {
			std::cerr << "Error!" << std::endl;
			exit(1);
		}
	} catch (const std::exception &e) {
		std::cerr << "Error! " << e.what() << std::endl;
		exit(1);
	}

	return 0;
}

//------------------------------------------------------------------------------
void specialChars (int argc, char **argv, int &index)
{
	if (index < argc) {
		std::string c = argv[index];
		if (c.length() != 5) {
			std::cout << "Must define 5 special characters" << std::endl;
			exit(1);
		}
		expander.SetSpecialChars(c[0], c[1], c[2], c[3], c[4]);
	}
}

//------------------------------------------------------------------------------
void usage ()
{
	size_t slash;
#if defined _WIN32
	slash = program.rfind('\\');
	if (slash == std::string::npos)
#endif
	slash = program.rfind('/');
	if (slash != std::string::npos) {
		program = program.substr(slash + 1);
	}

	std::cout << "Usage: " << program << " [options] [<input>|- [<output>|-]]" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "-d[name[=body]], --define name[=body]\tDefine a macro." << std::endl;
	std::cout << "-c,--chars <special_chars>\t\tDefine special chars (default: \"$(),$\")" << std::endl;
	std::cout << "-h,--help\t\t\t\tThis help." << std::endl;
	std::cout << "-v,--version\t\t\t\tPrint version information." << std::endl;
	exit(0);
}

//------------------------------------------------------------------------------
void version ()
{
	std::cout << "stemple version 1.0" << std::endl;
	exit(0);
}
