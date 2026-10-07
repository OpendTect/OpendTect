/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "odjson.h"
#include "timefun.h"


static bool testReadPar( const char* fnm, IOPar& inpar )
{
    inpar.read( fnm, sKey::Pars() );
    mRunStandardTest( inpar.size()==285, "Read IOPar from file" );

    return true;
}


static bool testIOParClass( const IOPar& inpar )
{
    IOPar testpar;
    testpar.add( "Key1", "Value1" );
    testpar.add( "Key2", "Value2" );
    testpar.add( "Key3", "Value3" );
    testpar.add( "Key5", "Value5" );

    const BufferString strvw1 = testpar.find( "Key1" );
    const BufferString strvw2 = testpar.find( "Key2" );
    const BufferString strvw3 = testpar.find( "Key3" );
    const BufferString strvw4 = testpar.find( "Key4" );
    mRunStandardTest( strvw4.isEmpty(),
				    "Not assigned key returns empty string" );
    const BufferString strvw5 = testpar.find( "Key5" );
    mRunStandardTest( ((!strvw1.isEqual(strvw2)) ||
		      (!strvw2.isEqual(strvw3)) || (!strvw3.isEqual(strvw5))),
				    "Retrieved string from IOPar correct" );
    mRunStandardTest( ((strvw1.buf() != strvw2.buf()) ||
	(strvw1.buf() != strvw2.buf()) || (strvw1.buf() != strvw2.buf())),
	"Retrieved string do not point to same location" );

    return true;
}


static bool testUseJSON( const IOPar& inpar )
{
    OD::JSON::Object jsonobj;
    inpar.fillJSON( jsonobj, false );

    IOPar outpar( inpar.name() );
    outpar.useJSON( jsonobj );
    mRunStandardTest( outpar.size() == inpar.size(),
		      "Retrieve IOPar from JSON (size only)" );
    IOParIterator iter( inpar );
    BufferString key, val;
    while( iter.next(key,val) )
    {
	if ( !outpar.isPresent(key.buf()) )
	    mRunStandardTestWithError( false, "IOPar from JSON (Key presence)",
			BufferString("Key: '",key.buf(),"' is missing") );
	const BufferString newval = outpar.find( key.buf() );
	if ( newval != val )
	{
	    BufferString errmsg( "Expected: '", val, "'; Found: '" );
	    errmsg.add( newval ).add( "'" );
	    mRunStandardTestWithError( false, "IOPar from JSON (Value check)",
				       errmsg );
	}
    }

    mRunStandardTest( outpar.isEqual(inpar), "Retrieve IOPar from JSON" );

    return true;
}


static bool testRemoveWithKeyPatternGlob()
{
    IOPar par;
    par.set( "AAA", "1" );
    par.set( "BBB", "2" );
    mRunStandardTest( par.size()==2, "Setup glob pattern map" );

    // Header documents glob semantics: "*" must match everything
    const bool removed = par.removeWithKeyPattern( "*" );
    mRunStandardTest( removed, "removeWithKeyPattern glob '*' matches" );
    mRunStandardTest( par.isEmpty(), "glob '*' removed all entries" );

    return true;
}


static bool testRemoveWithKeyPattern()
{
    IOPar par;
    for ( int idx=0; idx<500; idx++ )
    {
	BufferString key( "TmpKey", idx );
	par.set( key.buf(), "val" );
    }
    for ( int idx=0; idx<20; idx++ )
    {
	BufferString key( "Keep", idx );
	par.set( key.buf(), "val" );
    }
    mRunStandardTest( par.size()==520, "Setup pattern-removal map" );

    const bool removed = par.removeWithKeyPattern( "TmpKey*" );
    mRunStandardTest( removed, "removeWithKeyPattern found matches" );

    int nleft = 0;
    for ( int idx=0; idx<500; idx++ )
    {
	BufferString key( "TmpKey", idx );
	if ( par.isPresent(key.buf()) )
	    nleft++;
    }
    if ( nleft )
    {
	BufferString errmsg;
	errmsg.add( nleft );
	errmsg.add( " matching keys left after removeWithKeyPattern" );
	mRunStandardTestWithError( false,
	    "removeWithKeyPattern removed all matches", errmsg );
    }
    mRunStandardTest( par.size()==20,
		      "Only non-matching keys remain after pattern remove" );

    for ( int idx=0; idx<20; idx++ )
    {
	BufferString key( "Keep", idx );
	if ( !par.isPresent(key.buf()) )
	{
	    BufferString errmsg( "Lost key: '" );
	    errmsg.add( key ).add( "'" );
	    mRunStandardTestWithError( false,
		"Non-matching keys untouched by pattern remove", errmsg );
	}
    }

    // Exercise the crash path: copy + single-key remove (QHash detach)
    // after a pattern removal. Must not crash or corrupt.
    IOPar copied( par );
    mRunStandardTest( copied.size()==20, "Copy after pattern remove" );
    mRunStandardTest( copied.removeWithKey("Keep0"),
		      "Single removeWithKey after pattern remove" );
    mRunStandardTest( !copied.isPresent("Keep0") &&
		      copied.size()==19,
		      "Map valid after copy + single remove" );

    return true;
}


static bool testRemoveSubSelection()
{
    IOPar par;
    for ( int grp=0; grp<5; grp++ )
	for ( int idx=0; idx<100; idx++ )
	{
	    BufferString key( "Sub." ); key.add( grp ).add( ".Key" ).add( idx );
	    par.set( key.buf(), "val" );
	}
    par.set( "Other", "val" );
    mRunStandardTest( par.size()==501, "Setup subselection map" );

    mRunStandardTest( par.removeSubSelection("Sub.2"),
		      "removeSubSelection found matches" );

    int nleft = 0;
    for ( int idx=0; idx<100; idx++ )
    {
	BufferString key( "Sub.2.Key", idx );
	if ( par.isPresent(key.buf()) )
	    nleft++;
    }
    if ( nleft )
    {
	BufferString errmsg;
	errmsg.add( nleft );
	errmsg.add( " Sub.2 keys left after removeSubSelection" );
	mRunStandardTestWithError( false,
	    "removeSubSelection removed all of Sub.2", errmsg );
    }
    mRunStandardTest( par.size()==401,
		      "Only Sub.2 removed, rest remain" );
    mRunStandardTest( par.isPresent("Other") &&
		      par.isPresent("Sub.0.Key0") &&
		      par.isPresent("Sub.4.Key99"),
		      "Unrelated subselections untouched" );

    IOPar copied( par );
    mRunStandardTest( copied.removeWithKey("Other"),
		      "Single removeWithKey after subselection remove" );
    mRunStandardTest( !copied.isPresent("Other"),
		      "Map valid after copy + single remove" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    FilePath fp( __FILE__ );
    fp.setExtension( "par" );
    if ( !File::exists(fp.fullPath()) )
    {
	errStream() << "Input file not found" << od_endl;
	return 1;
    }

    IOPar inpar;
    if ( !testReadPar(fp.fullPath(),inpar) ||
	 !testIOParClass(inpar) ||
	 !testRemoveWithKeyPatternGlob() ||
	 !testRemoveWithKeyPattern() ||
	 !testRemoveSubSelection() ||
	 !testUseJSON(inpar) )
	return 1;

    return 0;
}
