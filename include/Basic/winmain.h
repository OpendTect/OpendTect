#ifndef winmain_h
#define winmain_h

#include <windows.h>

int main( int argc, char** argv );

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		      LPTSTR lpCmdLine, int nCmdShow )
{
    int res = main( __argc, __argv ); // __argc and __argv defined in windows.h
    return res;
}

#endif
	
