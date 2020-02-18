/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "testprog.h"

#include "genc.h"
#include "file.h"
#include "filepath.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"

#include <iostream>
#include "settingsaccess.h"

#define mTestDirNm "TestDir"

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

    OS::MachineCommand mc( OD::PythonAccess::sPythonExecNm(true) );
    mc.addArg( "-u" );
    mc.addArg( GetPythonScript("test_ioserv.py") );
    OS::CommandExecPars execpars( OS::RunInBG );
    execpars.createstreams( true );
    File::Path scriptfp;
    PtrMan<OS::CommandLauncher> cl = OD::PythA().getLauncher( mc, scriptfp );
    cl->execute( execpars );
    sleepSeconds( 1 );
    if ( !scriptfp.isEmpty() )
	File::remove( scriptfp.fullPath() );

    if ( !testComm(*cl.getStdInput(),*cl.getStdOutput()) )
	return 1;

    return 0;
}

*/

BufferString createTestDir()
{
    const File::Path fp( File::Path::getTempDir(), mTestDirNm );
    File::createDir( fp.fullPath() );
    const File::Path filefp( fp.fullPath(),
			    File::Path::getTempFileName("python","txt") );
    od_ostream strm( filefp.fullPath() );
    strm << "Testing deletion via python command line";
    strm.close();

    return fp.fullPath();
}

bool testRemoveDir( const BufferString& path )
{
    bool ret = OD::pythonRemoveDir( path, true ).isOK();
    if ( ret )
    {
	BufferStringSet dirnms;
	File::listDir( File::Path::getTempDir(), File::DirsInDir, dirnms );
	if ( dirnms.indexOf(mTestDirNm) >= 0 )
	    ret = false;
    }

    if ( !quiet_ )
    {
	if ( ret )
	    od_cout() << "Folder deleted successfully" << od_endl;
	else
	    od_cout() << "Folder deletion failed" << od_endl;
    }

    return ret;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    mRunStandardTest( true, "TODO: enable this test."
	    "\n\t** Why does it succeed when run 'by hand'?"
	    "\n\t** Why does it fail in the continuous integration??" )

    //if ( !OD::PythA().isUsable().isOK() )
	//return 1;

    const BufferString path = createTestDir();

    File::makeReadOnly( path, true );
    if ( testRemoveDir(path) )
	return 1;
    uiString errmsgui1;
    BufferString errmsg1 = OD::PythA().lastOutput(true,&errmsgui1);
    File::makeWritable( path, true, true );
    if ( !testRemoveDir(path) )
	return 1;

    return 0;
}
