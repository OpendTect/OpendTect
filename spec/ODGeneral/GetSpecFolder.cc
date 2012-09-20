/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : October 2003
 * FUNCTION : get special folder location
-*/

static const char* rcsID = "$Id$";


#include "prog.h"
#include "winutils.h"

#include <iostream>

using namespace std;

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	cerr << "Usage: " << argv[0]
	     << " {MyDocs,AppData,UsrProf,CommonApp,CommonDoc}"<< endl;
	return 1;
    }

    if ( !strcmp(argv[1], "MyDocs") )
    {
	cout <<  GetSpecialFolderLocation( CSIDL_PERSONAL ) << endl;
	return 0;
    }
    else if ( !strcmp(argv[1], "AppData") )
    {
	cout <<  GetSpecialFolderLocation( CSIDL_APPDATA ) << endl;
	return 0;
    }
    else if ( !strcmp(argv[1], "UsrProf") )
    {
	cout <<  GetSpecialFolderLocation( CSIDL_PROFILE ) << endl;
	return 0;
    }
    else if ( !strcmp(argv[1], "CommonApp") )
    {
	cout <<  GetSpecialFolderLocation( CSIDL_COMMON_APPDATA ) << endl;
	return 0;
    }
    else if ( !strcmp(argv[1], "CommonDoc") )
    {
	cout <<  GetSpecialFolderLocation( CSIDL_COMMON_DOCUMENTS ) << endl;
	return 0;
    }

    return 1;
}

