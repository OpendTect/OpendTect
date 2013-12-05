/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"
#include "genc.h"
#include "envvars.h"
#include "odver.h"
#include "msgh.h"
#include <iostream>


extern int ODMain(int,char**);
extern Export_Basic int gLogFilesRedirectCode;


inline static bool isPromised( const char* claim )
{
    return GetEnvVarYN( claim );
}


int main( int argc, char** argv )
{
#if !defined(__win__) || defined(__msvc__)
    gLogFilesRedirectCode = 1;
    // Only od_main should make log files, not od_process_attrib and so forth
    // Didn't fancy putting anything about this in header files
    // Hence the global 'hidden' variable
#endif

    SetProgramArgs( argc, argv );
    const FixedString argv1( argv[1] );
    const bool showversiononly = argv1 == "-v" || argv1 == "--version";

    int ret = 0;
    if ( showversiononly )
	std::cerr << GetFullODVersion() << std::endl;
    else
    {
	if ( !isPromised("OD_I_COMPLY_WITH_THE_LICENSE")	// Good.
	  && !isPromised("OD_I_PAID_MAINT_SUPP_FEE")		// Better.
	  && !isPromised("OD_I_AM_AN_OPENDTECT_DEVELOPER") )	// Yo.
	{
	    const char* msg =
		"OpendTect can be run under one of three licenses:"
		" (GPL, Commercial, Academic).\n"
		"Please consult http://opendtect.org/OpendTect_license.txt.";

	    std::cerr << msg << std::endl;
	    if ( gLogFilesRedirectCode == 1 )
		UsrMsg( msg );
	}

	ret = ODMain( argc, argv );
    }


    ExitProgram( ret );
}
