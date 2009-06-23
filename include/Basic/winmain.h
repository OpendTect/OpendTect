#ifndef winmain_h
#define winmain_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Ranojay Sen
 Date:          March 2009
 RCS:		$Id: winmain.h,v 1.2 2009-06-23 05:25:47 cvsranojay Exp $
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
	
