static const char* rcsID = "$Id: mix_files.cc,v 1.2 2008/11/25 15:35:21 cvsbert Exp $";

#include <iostream>
#include <fstream>
#include <stdlib.h>

#define maxlnlen 100000

int main( int argc, char** argv )
{
    if ( argc < 4 )
    {
	std::cout << "Usage: " << argv[0] << " input1 input2 output\n";
	return 1;
    }

    std::ifstream in1( argv[1] );
    std::ifstream in2( argv[2] );
    std::ofstream out( argv[3] );
    char buf1[maxlnlen];
    char buf2[maxlnlen];
    while ( in1 && in2 )
    {
	in1.getline( buf1, maxlnlen );
	in2.getline( buf2, maxlnlen );
	char* ptr1 = buf1; char* ptr2 = buf2;
	while ( *ptr1 && *ptr2 )
	{
	    while ( *ptr1 && isspace(*ptr1) ) ptr1++;
	    char* valptr1 = ptr1;
	    while ( *ptr1 && !isspace(*ptr1) ) ptr1++;
	    *ptr1++ = '\0';
	    while ( *ptr2 && isspace(*ptr2) ) ptr2++;
	    char* valptr2 = ptr2;
	    while ( *ptr2 && !isspace(*ptr2) ) ptr2++;
	    *ptr2++ = '\0';
	    out << valptr1 << '\t' << valptr2 << '\t';
	}
	out << std::endl;
    }

    return 0;
}
