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
#include "ptrman.h"
#include "strmprov.h"
#include "thread.h"
#include "strmoper.h"

#include <string.h>


static const BufferString tmpfnm( FilePath::getTempName("txt") );


#define mRetFail(s) { od_cout() << "Failed " << s << od_endl; }

#define mImplNumberTestFn(fnnm,decls,act,cond) \
static bool fnnm() \
{ \
    od_istream strm( tmpfnm ); \
    decls; \
    act; \
    const bool isok = (cond); \
    if ( !quiet ) \
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


bool testPipeInput()
{
    FixedString message = "OpendTect rules";
    const BufferString command( "@echo ", message );

    {
	StreamData streamdata = StreamProvider( command ).makeIStream();
	mRunStandardTest( streamdata.istrm,"Creation of standard input stream");
	PtrMan<od_istream> istream = new od_istream(streamdata.istrm);

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
	mRunStandardTest( streamdata.istrm,
			  "Creation of standard input stream (binary)");
	PtrMan<od_istream> istream = new od_istream(streamdata.istrm);

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

    command.add( " > " ).add( tmpfnm );
    StreamProvider prov( command );
    StreamData ostreamdata = prov.makeOStream();
    mRunStandardTest( ostreamdata.ostrm,  "Creation of standard output stream");
    PtrMan<od_ostream> ostream = new od_ostream(ostreamdata.ostrm);

    *ostream << message << ' ';
    *ostream << num;
    ostream->close();

    ostream = 0; //Deletes everything
    Threads::sleep( 1 );

    od_istream istream( tmpfnm );
    mRunStandardTest( istream.isOK(), "Opening temporary file");
    BufferString streaminput;

    istream.getAll( streaminput );
    istream.close();
    File::remove( tmpfnm );

    mRunStandardTest( streaminput==originpstr, "Pipe content check (Output)" );

    return true;
}


int doExit( int retval )
{
    if ( File::exists( tmpfnm ) )
	File::remove( tmpfnm );

    return ExitProgram( retval );
}


int main( int argc, char** argv )
{
    mInitTestProg();
    DBG::turnOn(0); //Turn off all debug-stuff as it screwes the pipes

    bool isok;
#define mDoTest(strm,content,tstfn) \
    od_ostream strm( tmpfnm ); \
    strm << content; \
    strm.close(); \
    isok = tstfn(); \
    File::remove( tmpfnm ); \
    if ( !isok ) \
	doExit( 1 )

    mDoTest(strm1,"123 44.5",testIfNumberIsNormal);
    mDoTest(strm2,"\t\n123\t\t44.5\n\n",testIfNumberIsNormal);
    mDoTest(strm3,"aap 44.5",testIfNumberIsBad);
    mDoTest(strm4," 123 noot",testIfNumberIsBad);
    mDoTest(strm5,"123\n-0.0e-22\n888",testIfFNumberIs0);
    mDoTest(strm6,"123",testOnlyIntRead);
    mDoTest(strm7,"\n123\n \n",testOnlyIntRead);

    if (!testPipeInput())
        doExit(1);

    if (!testPipeOutput())
    {
	if (File::exists(tmpfnm))
	    File::remove(tmpfnm);

	doExit(1);
    }

    return doExit( 0 );
}
