#ifndef _execbatch_h
#define _execbatch_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Lammertink
 Date:		30-10-2003
 RCS:		$Id: _execbatch.h,v 1.4 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________

 The implementation fo Execute_batch should be in the executable on 
 windows, but can be in a .so on *nix.
 In order not to pullute batchprog.h, I've placed the implementation
 into a separate file, which is included trough batchprog.h on win32
 and included in batchprog.cc on *nix.
 
*/

#include "strmprov.h"
#include "strmdata.h"


int Execute_batch( int* pargc, char** argv )
{
    BufferString envarg("DTECT_ARGV0=");
    envarg += argv[0];
    putenv( envarg.buf() );

    PIM().setArgs( *pargc, argv ); PIM().loadAuto( false );

    BatchProgram::inst_ = new BatchProgram( pargc, argv );
    if ( !BP().stillok_ )
	return 1;

    if ( BP().inbg_ )
    {
#ifndef __win__
	switch ( fork() )
	{
	case -1:
	    cerr << argv[0] <<  "cannot fork:\n" << errno_message() << endl;
	case 0: break;
	default:
	    Time_sleep( 0.1 );
	    exit( 0 );
	break;
	}
#endif
    }

    BatchProgram& bp = *BatchProgram::inst_;
    bool allok = bp.initOutput() && bp.go( *bp.sdout_.ostrm );
    bp.stillok_ = allok;
    delete BatchProgram::inst_;
    return allok ? 0 : 1;
}

#endif
