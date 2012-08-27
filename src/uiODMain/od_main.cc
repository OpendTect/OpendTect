/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: od_main.cc,v 1.28 2012-08-27 13:16:50 cvskris Exp $";

#include "prog.h"
#include "genc.h"
#include "envvars.h"
#include "errh.h"
#include "odver.h"
#include <iostream>


extern int ODMain(int,char**);
extern Export_Basic int gLogFilesRedirectCode;


inline static bool isPromised( const char* claim )
{
    return GetEnvVarYN( claim );
}


int main( int argc, char** argv )
{
    const bool showversiononly = argv[1]
	    && (!strcmp(argv[1],"-v") || !strcmp(argv[1],"--version"));

#if !defined(__win__) || defined(__msvc__)
    gLogFilesRedirectCode = 1;
    // Only od_main should make log files, not od_process_attrib and so forth
    // Didn't fancy putting anything about this in header files
    // Hence the global 'hidden' variable
#endif

    int ret = 0;
    if ( showversiononly )
	std::cerr << GetFullODVersion() << std::endl;
    else
    {
	if ( !isPromised("OD_I_COMPLY_WITH_THE_LICENSE")	// Good.
	  && !isPromised("OD_I_PAID_MAINT_SUPP_FEE")		// Better.
	  && !isPromised("OD_I_AM_AN_OPENDTECT_DEVELOPER") )	// Yo.
	{
	    static const char* msg =
		"OpendTect can be run under one of three licenses:"
		" (GPL, Commercial, Academic).\n"
		"Please consult http://opendtect.org/OpendTect_license.txt.";

	    std::cerr << msg << std::endl;
	    if ( gLogFilesRedirectCode == 1 )
		UsrMsg( msg );
	}

	od_putProgInfo( argc, argv );
   
	ret = ODMain( argc, argv );
    }

   
    ExitProgram( ret );
}
