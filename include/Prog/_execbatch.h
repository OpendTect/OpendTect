#ifndef _execbatch_h
#define _execbatch_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Lammertink
 Date:		30-10-2003
 RCS:		$Id: _execbatch.h,v 1.1 2003-10-30 15:22:32 arend Exp $
________________________________________________________________________

 Batch programs should include this header, and define a BatchProgram::go().
 If program args are needed outside this method, BP() can be accessed.
 
*/

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
