#ifndef prog_h
#define prog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.15 2009-03-17 13:41:22 cvsbert Exp $
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "plugins.h"
#include "debug.h"

#ifdef _win__
#ifndef _CONSOLE
# include <windows.h>
#endif
#endif

#ifdef __cpp__
extern "C" {
#endif
    const char*		errno_message();
    			/*!< Will not return meaningful string on Windows */
#ifdef __cpp__
}
#endif


#ifndef _CONSOLE

int main( int argc, char** argv );

#ifdef _win__
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		      LPTSTR lpCmdLine, int nCmdShow )
{
    int res = main( __argc, __argv ); // __argc and __argv defined in windows.h
    return res;
}
#endif

#endif


#endif
