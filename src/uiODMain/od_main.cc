/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.8 2004-12-21 13:18:02 bert Exp $
________________________________________________________________________

-*/


#include "prog.h"
#include "genc.h"
#include <iostream>

// TODO : Is there a better way to force linking with attribute factory?
#ifdef __mac__
# include "attribfact.h"
#endif

extern int ODMain(int,char**);


inline static bool isPromised( const char* claim )
{
    const char* answer = getenv( claim );
    return answer && *answer && (*answer == 'y' || *answer == 'Y');
}


int main( int argc, char** argv )
{
    if ( !isPromised("OD_I_AM_JUST_EVALUATING")		 // Hmmm.
      && !isPromised("OD_I_AM_NOT_A_COMMERCIAL_USER")	 // Good.
      && !isPromised("OD_I_PAID_MAINT_SUPP_FEE")	 // Better.
      && !isPromised("OD_I_AM_AN_OPENDTECT_DEVELOPER") ) // Yo.
    {
	std::cerr << "OpendTect is free for R&D, education or evaluation only."
		     "\nIn doubt, consult http://opendtect.org/rel/LICENSE.txt."
		     << std::endl;
    }

    od_putProgInfo( argc, argv );

    int ret = ODMain( argc, argv );
    exitProgram( ret );
}
