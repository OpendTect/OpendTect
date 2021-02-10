#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		5-12-1995
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "debug.h"
#include "genc.h"
#include "od_ostream.h"
#include "odruncontext.h"
#include "plugins.h"


// Use this in stand-alone program
# define mProgMainFnName doMain

int doMain( int, char** );

# ifndef mMainIsDefined
# define mMainIsDefined
int main( int argc, char** argv )
{
    ExitProgram( doMain(argc,argv) );
}
# endif

# define mInitProg( ctxt ) \
    OD::SetRunContext( ctxt ); \


static inline mUsedVar od_ostream& gtStream( bool err=false )
{
    return err ? od_ostream::logStream()
	       : od_ostream::nullStream();
}

static inline mUnusedVar od_ostream& logStream()
{
    return gtStream( false );
}

static inline mUnusedVar od_ostream& errStream()
{
    return gtStream( true );
}
