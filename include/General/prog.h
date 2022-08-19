#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// Include this file in any executable program you make.

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
