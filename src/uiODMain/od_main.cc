/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.17 2006-09-14 10:10:46 cvsdgb Exp $
________________________________________________________________________

-*/


#include "prog.h"
#include "genc.h"
#include "envvars.h"
#include "errh.h"
#include <iostream>

// TODO : Is there a better way to force linking with attribute factory?
#ifdef __mac__
# include "attribfactory.h"
#endif

extern int ODMain(int,char**);
extern int gLogFilesRedirectCode;


inline static bool isPromised( const char* claim )
{
    return GetEnvVarYN( claim );
}


int main( int argc, char** argv )
{
#ifndef __win__
    gLogFilesRedirectCode = 1;
	// Only odmain should make log files, not process_attrib and so forth
	// Didn't fancy putting anything about this in header files
	// Hence the global 'hidden' variable
#endif

    if ( !isPromised("OD_I_AM_JUST_EVALUATING")		 // Hmmm.
      && !isPromised("OD_I_AM_NOT_A_COMMERCIAL_USER")	 // Good.
      && !isPromised("OD_I_PAID_MAINT_SUPP_FEE")	 // Better.
      && !isPromised("OD_I_AM_AN_OPENDTECT_DEVELOPER") ) // Yo.
    {
	static const char* msg =
	    "OpendTect is free for R&D, education or evaluation only.\n"
	    "In doubt, consult http://opendtect.org/rel/LICENSE.txt.";

	std::cerr << msg << std::endl;
	if ( gLogFilesRedirectCode == 1 )
	    UsrMsg( msg );
    }

    od_putProgInfo( argc, argv );

    int ret = ODMain( argc, argv );
    ExitProgram( ret );
}
