#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 For the uiVSG test programs on Windows: instead of the Debug Assertion
 dialog of the C runtime, print the failed assertion and the call stack to
 stderr and stop the program.

-*/

#ifdef _WIN32

#include <windows.h>
#include <crtdbg.h>
#include <dbghelp.h>

#include <cstdio>

#pragma comment(lib, "dbghelp.lib")

namespace vsgTest
{

inline void printCallStack()
{
    HANDLE process = GetCurrentProcess();
    SymSetOptions( SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
    SymInitialize( process, nullptr, TRUE );

    void* frames[64];
    const USHORT nrframes = CaptureStackBackTrace( 0, 64, frames, nullptr );
    for ( USHORT idx=0; idx<nrframes; idx++ )
    {
	const DWORD64 addr = DWORD64( frames[idx] );
	char buffer[sizeof(SYMBOL_INFO)+256];
	SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>( buffer );
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);
	sym->MaxNameLen = 255;
	DWORD64 symdisp = 0;
	const bool hassym = SymFromAddr( process, addr, &symdisp, sym );

	IMAGEHLP_LINE64 line;
	line.SizeOfStruct = sizeof(line);
	DWORD linedisp = 0;
	const bool hasline = SymGetLineFromAddr64( process, addr, &linedisp,
						   &line );
	std::fprintf( stderr, "  #%d %s  %s:%lu\n", int(idx),
		      hassym ? sym->Name : "?",
		      hasline ? line.FileName : "", hasline ? line.LineNumber : 0 );
    }

    std::fflush( stderr );
}


inline int assertReportHook( int type, char* message, int* retval )
{
    if ( type != _CRT_ASSERT && type != _CRT_ERROR )
	return FALSE;

    std::fprintf( stderr, "ASSERTION: %s\n", message ? message : "" );
    printCallStack();
    *retval = 0;
    TerminateProcess( GetCurrentProcess(), 3 );
    return TRUE;
}


inline void installAssertTrace()
{
    _CrtSetReportHook( assertReportHook );
}

} // namespace vsgTest

#else

namespace vsgTest
{
inline void installAssertTrace() {}
}

#endif
