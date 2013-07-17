#ifndef winmain_h
#define winmain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          March 2009
 RCS:		$Id$
_______________________________________________________________________

-*/


#include <windows.h>

int main( int argc, char** argv );

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		      LPTSTR lpCmdLine, int nCmdShow )
{
    return main( __argc, __argv ); // __argc and __argv defined in windows.h
}

#endif
	
