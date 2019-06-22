/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
 * FUNCTION :
-*/


#include "od_iostream.h"
#include "testprog.h"
#include "filesystemaccess.h"
#include "filepath.h"
#include "ptrman.h"
#include "strmprov.h"
#include "thread.h"

#include <string.h>


static BufferString getTestTempFileName()
{
    static BufferString tmpfnm(
                    File::Path::getTempFullPath("iostrm_test","txt") );
    return tmpfnm;
}

#define mRetFail(s) { od_cout() << "Failed " << s << od_endl; }

#define mImplNumberTestFn(fnnm,decls,act,cond) \
static bool fnnm() \
{ \
    od_istream strm( getTestTempFileName() ); \
    decls; \
    act; \
    const bool isok = (cond); \
    if ( !quiet_ ) \
    { \
	od_cout() << #fnnm << "[" << #cond << "]" \
		  << (isok ? " OK" : " Fail") << od_endl; \
	if ( strm.isBad() ) \
		od_cout() << "\terrmsg=" << toString(strm.errMsg()) \
			  << od_endl; \
    } \
    return isok; \
}


mImplNumberTestFn( testIfNumberIsNormal, int i; float f,
	   strm >> i >> f,
	   i == 123 && mIsEqual(f,44.5f,1e-8) )

mImplNumberTestFn( testIfNumberIsBad, int i; float f,
	   strm >> i >> f,
	   strm.isBad() )

mImplNumberTestFn( testIfFNumberIs0, int i; float f,
	   strm >> i >> f,
	   i == 123 && f == 0 && strm.isOK() )

mImplNumberTestFn( testOnlyIntRead, int i; float f,
	   strm >> i >> f,
	   i == 123 && !strm.isOK() )


bool testPipeInput()
{
    FixedString message = "OpendTect rules";
    const BufferString command( "@echo ", message );

    {
	StreamData streamdata = StreamProvider( command ).makeIStream();
	mRunStandardTest( streamdata.iStrm(),
			  "Creation of standard input stream");
	PtrMan<od_istream> istream = new od_istream(streamdata.iStrm());

	BufferString streaminput;
	mRunStandardTest( istream->getAll( streaminput ) , "Read from pipe" );

	//If EOF and too short message, retry ten times
	for ( int idx=0;
	      idx<10 && istream->atEOF() && streaminput.size()<message.size();
	      idx++ )
	{
	    Threads::sleep(1);

	    BufferString newmsg;
	    if ( istream->getAll(newmsg))
	    {
		streaminput += newmsg;
	    }
	}

	mRunStandardTest( streaminput==message, "Pipe content check (Input)" );

	istream->getAll(streaminput); //Try to read beyond
	mRunStandardTest(istream->atEOF() && streaminput.isEmpty(),
			"Force read at end of stream");
    }
    {
	StreamData streamdata = StreamProvider( command ).makeIStream();
	mRunStandardTest( streamdata.iStrm(),
			  "Creation of standard input stream (binary)");
	PtrMan<od_istream> istream = new od_istream(streamdata.iStrm());

#define mSize	(100)
	char streaminput[mSize];
	memset( streaminput, 0, mSize );

	mRunStandardTest( !istream->getBin( streaminput, mSize-1 ) ,
			  "Read from pipe to binary buffer should fail" );

	//If EOF and too short message, retry ten times
	for ( int idx=0;
	      idx<10 && istream->atEOF() && strlen(streaminput)<message.size();
	      idx++ )
	{
	    Threads::sleep(1);

	    const int sz = strlen(streaminput);
	    istream->getBin(streaminput+sz,mSize-sz-1);
	}

	mRunStandardTest( !strncmp(message.buf(),streaminput,message.size()),
			  "Pipe content check (Input-binary)" );

	streaminput[0] = 0; //Try to read beyond
	istream->getBin(streaminput,mSize-1);
	mRunStandardTest(istream->atEOF() && !strlen(streaminput),
			"Force read at end of stream - binary");
    }

    return true;
}


bool testPipeOutput()
{
    FixedString message = "OpendTect rules";
    int num = 54637;
    BufferString originpstr( message );
    originpstr.add( " " ).add( num );

    BufferString command = "@";
#ifdef __win__
    command.add( "more" );
#else
    command.add( "cat" );
#endif

    command.add( " > " ).add( getTestTempFileName() );
    StreamProvider prov( command );
    StreamData ostreamdata = prov.makeOStream();
    mRunStandardTest( ostreamdata.oStrm(),
		      "Creation of standard output stream");
    PtrMan<od_ostream> ostream = new od_ostream(ostreamdata.oStrm());

    *ostream << message << " ";
    *ostream << num;
    ostream->close();

    ostream = 0; //Deletes everything
    Threads::sleep( 1 );

    od_istream istream( getTestTempFileName() );
    mRunStandardTest( istream.isOK(), "Opening temporary file");
    BufferString streaminput;

    istream.getAll( streaminput );
    istream.close();
    File::remove( getTestTempFileName() );

    mRunStandardTest( streaminput==originpstr, "Pipe content check (Output)" );

    return true;
}


bool testPrefix()
{
    mRunStandardTest(
	    File::SystemAccess::withoutProtocol( "file://")=="",
	    "Remove protocol from emtpy file name");

    mRunStandardTest(
	    File::SystemAccess::withoutProtocol( "file://abcde")=="abcde",
	    "Remove protocol file name with protocol");

    mRunStandardTest(
	     File::SystemAccess::withoutProtocol( "abcde")=="abcde",
	     "Remove protocol file name without protocol");

    mRunStandardTest(
	     File::SystemAccess::getProtocol( "s3://abcd" )=="s3",
	     "Get protocol" );
    mRunStandardTest(
		File::SystemAccess::getProtocol( "abcde" )=="file",
		"Get non-existing protocol" );

    return true;
}


int doExit( int retval )
{
    if ( File::exists( getTestTempFileName() ) )
	File::remove( getTestTempFileName() );

    return retval;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    //TODO remove when we understand why the CI crash happens
    quiet_ = false;

    DBG::turnOn( 0 ); //Turn off all debug-stuff as it screws the pipes

    bool isok;
#define mDoTest(strm,content,tstfn) \
    od_ostream strm( getTestTempFileName() ); \
    strm << content; \
    strm.close(); \
    isok = tstfn(); \
    File::remove( getTestTempFileName() ); \
    if ( !isok ) \
	return doExit( 1 )


    od_cout() << "-> Number tests." << od_endl;

    mDoTest(strm1,"123 44.5",testIfNumberIsNormal);
    mDoTest(strm2,"\t\n123\t\t44.5\n\n",testIfNumberIsNormal);
    mDoTest(strm3,"aap 44.5",testIfNumberIsBad);
    mDoTest(strm4," 123 noot",testIfNumberIsBad);
    mDoTest(strm5,"123\n-0.0e-22\n888",testIfFNumberIs0);
    mDoTest(strm6,"123",testOnlyIntRead);
    mDoTest(strm7,"\n123\n \n",testOnlyIntRead);

    od_cout() << "-> Pipe input test." << od_endl;

    if ( !testPipeInput() )
	return doExit( 1 );

    od_cout() << "-> Pipe output test." << od_endl;
    if ( !testPipeOutput() )
    {
	if ( File::exists(getTestTempFileName()) )
	    File::remove( getTestTempFileName() );

	return doExit(1);
    }

    od_cout() << "-> Prefix test." << od_endl;
    if ( !testPrefix() )
	return doExit( 1 );

    od_cout() << "-> No problem." << od_endl;
    return doExit( 0 );
}
