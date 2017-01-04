// txt2c.cpp : This simple app takes a text file
// and generates a .cpp file which will embed the
// contents of the text file as plain text.
//

#include "StdAfx.h"

#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

void
quoteLine(char *d, const char *s)
{
    while (*s)
    {
	switch (*s)
	{
	    case '"':
	    case '\\':
	    {
		*d++ = '\\';
		*d++ = *s++;
		break;
	    }
	    default:
		*d++ = *s++;
		break;
	}
    }
    *d++ = '\0';
}

int 
main(int argc, char* argv[])
{
    if (argc < 3)
    {
	cerr << "Usage is " << argv[0] << " source.txt result.cpp" << endl;
	cerr << "This will transform source.txt into an embeddable string." << endl;
	return -2;
    }

    ifstream		is(argv[1], ios::in);

    if (!is)
    {
	cerr << "Couldn't open " << argv[1] << endl;
	return -1;
    }

    ofstream	os(argv[2]);
    if (!os)
    {
	cerr << "Couldn't open for write " << argv[2] << endl;
    }

    char		line[2005];
    char		quote[4000];

    // Build the name to write to from the name of the file..
    char		*name;

    name = strdup(argv[2]);
    if (strchr(name, '.'))
	*strchr(name, '.') = '\0';
    
    os << "const char *glb_txt_" << name << " =" << endl;
	
    while (is)
    {
	is.getline(line, 2000);
	if (!is)
	    break;

	quoteLine(quote, line);

	os << "\"" << quote << "\\n\"" << endl;
    }
    os << ";" << endl;

    return 0;
}

