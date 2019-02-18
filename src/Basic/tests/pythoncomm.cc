/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "oddirs.h"
#include "testprog.h"
#include "oscommand.h"
#include "od_iostream.h"
#include "genc.h"
#include <iostream>

/* TODO activate??

static bool getData( od_istream& pullstrm, float* vals,
			int expectednrvals, const char* whatstr )
{
    int nrvals;
    int nrerrs = 0;
    while ( true )
    {
	nrvals = 0;
	pullstrm.getBin( nrvals );
	const bool badstrm = pullstrm.isBad();
	if ( nrvals == 0 || badstrm )
	{
	    nrerrs++;
	    std::istream& stdstrm = pullstrm.stdStream();
	    if ( stdstrm.bad() || nrerrs > 100 )
	    {
		tstStream(false) << "bad strm, or too many fails" << od_endl;
		BufferString errmsg;
		pullstrm.addErrMsgTo( errmsg );
		tstStream(true) << errmsg << od_endl;
		return false;
	    }
	    stdstrm.clear();
	    Threads::sleep( 0.02 );
	    continue;
	}
	else if ( nrvals == 0 )
	{
	    tstStream(false) << "got 0 vals, continuing loop" << od_endl;
	    Threads::sleep( 0.02 );
	    continue;
	}
	mRunStandardTestWithError( nrvals==expectednrvals,
				   BufferString(whatstr," (nr vals)"),
				   BufferString("nrvals=",nrvals) )
	break;
   }
   mRunStandardTest( pullstrm.getBin(vals,nrvals*sizeof(float)),
			BufferString(whatstr," (slurp)") )
   return true;
}

static bool testComm( od_ostream& pushstrm, od_istream& pullstrm )
{
    int nrvals; float vals[10], outvals[2];

    nrvals = 3; vals[0] = 2.f; vals[1] = vals[2] = 5.f;
    pushstrm.addBin( nrvals );
    mRunStandardTest( pushstrm.addBin(vals,nrvals*sizeof(float)),
			"First vals put" )

    if ( !getData(pullstrm,outvals,2,"first") )
	return false;
    mRunStandardTestWithError( outvals[0]>3.99f&&outvals[0]<4.01f,
	"First Average calculated", BufferString("outvals[0]=",outvals[0]) )

    nrvals = 6; vals[3] = 8.f; vals[4] = vals[5] = 5.f;
    pushstrm.addBin( nrvals );
    mRunStandardTest( pushstrm.addBin(vals,nrvals*sizeof(float)),
			"2nd vals put" )
    if ( !getData(pullstrm,outvals,2,"second") )
	return false;
    mRunStandardTestWithError( outvals[0]>4.99f&&outvals[0]<5.01f,
	"2nd Average calculated", BufferString("outvals[0]=",outvals[0]) )

    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OS::MachineCommand mc( GetPythonCommand() );
    mc.addArg( "-u" );
    mc.addArg( GetPythonScript("test_ioserv.py") );
    OS::CommandExecPars execpars( OS::RunInBG );
    execpars.createstreams( true );
    OS::CommandLauncher cl( mc );
    cl.execute( execpars );
    sleepSeconds( 1 );

    if ( !testComm(*cl.getStdInput(),*cl.getStdOutput()) )
	return 1;

    return 0;
}

*/

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    mRunStandardTest( true, "TODO: enable this test."
	    "\n\t** Why does it succeed when run 'by hand'?"
	    "\n\t** Why does it fail in the continuous integration??" )

    return 0;
}
