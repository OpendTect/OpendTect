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
#include <QString>

int APIENTRY wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		       LPTSTR lpCmdLine, int nCmdShow )
{
    static std::vector<std::string> argvlist;
    static std::unique_ptr<char*> argv_( new char*[__argc] );
    char** test = argv_.get();
    for ( int idx=0; idx<__argc; idx++ )
    {
	argvlist.push_back( QString( __wargv[idx] ).toStdString() );
	argv_.get()[idx] = (char*)( argvlist[idx].c_str() );
    }

    return main( __argc, argv_.get() ); // __argc and __wargv defined in windows.h;
}
#else
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		      LPTSTR lpCmdLine, int nCmdShow )
{
    return main( __argc, __argv ); // __argc and __argv defined in windows.h;
}
#endif
