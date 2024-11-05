#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <windows.h>

int main( int argc, char** argv );

#ifdef UNICODE
int APIENTRY wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		       LPTSTR lpCmdLine, int nCmdShow )
#else
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		      LPTSTR lpCmdLine, int nCmdShow )
#endif
{
    return main( __argc, __argv ); // __argc and __argv defined in windows.h;
}
