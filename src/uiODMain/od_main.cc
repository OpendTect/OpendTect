/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_main.cc,v 1.21 2009-07-22 16:01:40 cvsbert Exp $";



#include "prog.h"
#include "genc.h"
#include "envvars.h"
#include "errh.h"
#include "odver.h"
#include <iostream>


// TODO : Is there a better way to force linking with attribute factory?
#ifdef __mac__
# include "attribfactory.h"
#endif




extern int ODMain(int,char**);
mBasicExtern int gLogFilesRedirectCode;


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
	// Only odmain should make log files, not process_attrib and so forth
	// Didn't fancy putting anything about this in header files
	// Hence the global 'hidden' variable
#endif

    int ret = 0;
    if ( showversiononly )
	std::cerr << GetFullODVersion() << std::endl;
    else
    {
	if ( !isPromised("OD_I_AM_JUST_EVALUATING")		// Hmmm.
	  && !isPromised("OD_I_AM_NOT_A_COMMERCIAL_USER")	// Good.
	  && !isPromised("OD_I_PAID_MAINT_SUPP_FEE")		// Better.
	  && !isPromised("OD_I_AM_AN_OPENDTECT_DEVELOPER") )	// Yo.
	{
	    static const char* msg =
		"OpendTect is free for R&D, education or evaluation only.\n"
		"In doubt, consult http://opendtect.org/rel/LICENSE.txt.";

	    std::cerr << msg << std::endl;
	    if ( gLogFilesRedirectCode == 1 )
		UsrMsg( msg );
	}

	od_putProgInfo( argc, argv );
   
	ret = ODMain( argc, argv );
    }

   
    ExitProgram( ret );
}
