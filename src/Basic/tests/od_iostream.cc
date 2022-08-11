/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
 * FUNCTION :
-*/


#include "od_iostream.h"
#include "testprog.h"
#include "file.h"
#include "filepath.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "strmprov.h"
#include "thread.h"

#include <string.h>


static BufferString getTestTempFileName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret = FilePath::getTempFullPath( "iostrm test", "txt" );

    return ret;
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
		od_cout() << "\terrmsg=" << strm.errMsg().getFullString() \
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


bool testLineEndings()
{
    const char* dgbstr = "dGB Earth Sciences";
    const BufferString testfnm = getTestTempFileName();
    od_ostream ostrm( testfnm );
    mRunStandardTestWithError( ostrm.isOK(),
		      "Opening temporary file for write",
		       ostrm.errMsg().getFullString().str() );

    int idx = 0;
    int size = 0;
    while ( idx < 4 )
    {
	BufferString str;
	str.add( dgbstr[idx++] );
	str.add( idx ).add( " " ).add( idx*idx ).add( "\t" ).add( idx*idx*idx );
	str.add( "\r" ); // CR only (Some exostic OS or apps)
	ostrm << str;
	size += str.size();
    }

    while ( idx < 10 )
    {
	BufferString str;
	str.add( dgbstr[idx++] );
	str.add( idx ).add( " " ).add( idx*idx ).add( "\t" ).add( idx*idx*idx );
	str.add( "\n" ); // LF only (Unix)
	ostrm << str;
	size += str.size();
    }

    while ( idx < 18 )
    {
	BufferString str;
	str.add( dgbstr[idx++] );
	str.add( idx ).add( " " ).add( idx*idx ).add( "\t" ).add( idx*idx*idx );
	str.add( "\r\n" ); // CR+LF (Windows)
	ostrm << str;
	size += str.size();
    }

    ostrm.close();
    mRunStandardTestWithError( File::getFileSize(testfnm) == size,
		      "Checking size of temporary file",
		      "File size is incorrect" );

    od_istream istrm( testfnm );
    mRunStandardTestWithError( istrm.isOK(),
		      "Opening temporary file for read",
		       istrm.errMsg().getFullString().str() );

    BufferString line, firstchars;
    while ( istrm.getLine(line) )
	firstchars.add( line[0] );

    mRunStandardTestWithError( firstchars == dgbstr,
		      "Checking line sanity",
		      "Error reading some line endings" );

    return true;
}


bool testPipeInput()
{
    StringView message = "OpendTect rules";
    OS::MachineCommand mc( "echo" );
    mc.addArg( "OpendTect" ).addArg( "rules" );
    {
	PtrMan<od_istream> istream = new od_istream( mc );

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
	PtrMan<od_istream> istream = new od_istream( mc );

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
    StringView message = "OpendTect rules";
    int num = 54637;
    BufferString originpstr( message );
    originpstr.add( " " ).add( num );

    BufferString prog;
#ifdef __win__
    OS::MachineCommand mc( "more" );
#else
    OS::MachineCommand mc( "cat" );
#endif
    mc.addFileRedirect( getTestTempFileName() );

    PtrMan<od_ostream> ostream = new od_ostream( mc );
    mRunStandardTestWithError( ostream && ostream->isOK(),
		      "Opening temporary file for write",
		       ostream->errMsg().getFullString().str() );

    *ostream << message << " ";
    *ostream << num;
    ostream->close();

    ostream = nullptr; //Deletes everything
    Threads::sleep( 1 );
    mRunStandardTestWithError( File::exists(getTestTempFileName()),
				"Temporary file is written",
	BufferString("File '", getTestTempFileName(),"' does not exist").str() )

    od_istream istream( getTestTempFileName() );
    mRunStandardTestWithError( istream.isOK(),
				"Opening temporary file for read",
				istream.errMsg().getFullString().str() );
    BufferString streaminput;

    istream.getAll( streaminput );
    istream.close();
    File::remove( getTestTempFileName() );

    mRunStandardTest( streaminput==originpstr, "Pipe content check (Output)" );

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

    od_cout() << "-> Line endings test." << od_endl;

    if ( !testLineEndings() )
	return doExit( 1 );

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

    od_cout() << "-> No problem." << od_endl;
    return doExit( 0 );
}
