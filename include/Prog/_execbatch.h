#ifndef _execbatch_h
#define _execbatch_h
 
/*
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		30-10-2003
 RCS:		$Id$
________________________________________________________________________

 The implementation fo Execute_batch should be in the executable on 
 windows, but can be in a .so on *nix.
 In order not to pollute batchprog.h, I've placed the implementation
 into a separate file, which is included trough batchprog.h on win32
 and included in batchprog.cc on *nix.
 
*/

#include "strmprov.h"
#include "envvars.h"


int Execute_batch( int* pargc, char** argv )
{
    SetEnvVar( "DTECT_ARGV0", argv[0] );

    PIM().setArgs( *pargc, argv ); PIM().loadAuto( false );

    BP().init( pargc, argv );
    if ( !BP().stillok )
	return 1;
    if ( BP().inbg )
	forkProcess();

    BatchProgram& bp = BP();
    bool allok = bp.initOutput();
    if ( allok )
    {
	*bp.sdout.ostrm << "Starting program " << argv[0] << " "
	    << bp.name() << "\n";
	allok = bp.go( *bp.sdout.ostrm );
    }

    bp.stillok = allok;
    BatchProgram::deleteInstance();

    return allok ? 0 : 1;	// never reached.
}

#endif
