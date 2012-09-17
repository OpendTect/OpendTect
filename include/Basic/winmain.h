#ifndef winmain_h
#define winmain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          March 2009
 RCS:		$Id: winmain.h,v 1.3 2009/07/22 16:01:14 cvsbert Exp $
_______________________________________________________________________

-*/


#include <windows.h>

int main( int argc, char** argv );

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		      LPTSTR lpCmdLine, int nCmdShow )
{
    int res = main( __argc, __argv ); // __argc and __argv defined in windows.h
    return res;
}

#endif
	
