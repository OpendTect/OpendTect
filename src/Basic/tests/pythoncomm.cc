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



static bool testComm( od_ostream& pushstrm, od_istream& pullstrm )
{
    int nrvals; float vals[10], outvals[2];

    nrvals = 3; vals[0] = 2.f; vals[1] = vals[2] = 5.f;
    pushstrm.addBin( nrvals );
    mRunStandardTest( pushstrm.addBin(vals,nrvals*sizeof(float)),
			"First vals put" )
    pullstrm.getBin( nrvals );
    mRunStandardTestWithError( nrvals==2, "First Nr vals got",
				BufferString("nrvals=",nrvals) )
    mRunStandardTest( pullstrm.getBin(outvals,nrvals*sizeof(float)),
			"First vals got" )
    mRunStandardTestWithError( outvals[0]>3.99f&&outvals[0]<4.01f,
	"First Average calculated", BufferString("outvals[0]=",outvals[0]) )

    nrvals = 6; vals[3] = 8.f; vals[4] = vals[5] = 5.f;
    pushstrm.addBin( nrvals );
    mRunStandardTest( pushstrm.addBin(vals,nrvals*sizeof(float)),
			"2nd vals put" )
    pullstrm.getBin( nrvals );
    mRunStandardTest( pullstrm.getBin(outvals,nrvals*sizeof(float)),
			"2nd vals got" )
    mRunStandardTest( nrvals==2, "2nd Nr vals got" )
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
    sleepSeconds( 0.2 );

    if ( !testComm(*cl.getStdInput(),*cl.getStdOutput()) )
	return 1;

    return 0;
}
