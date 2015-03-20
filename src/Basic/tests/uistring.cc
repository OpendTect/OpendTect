/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"
#include "testprog.h"

#include <QString>

bool testArg()
{
    uiString composite = toUiString( "%1 plus %2 is %3")
		.arg( 4 )
		.arg( 5 )
		.arg( 9 );

    mRunStandardTest( composite.getFullString()=="4 plus 5 is 9",
		      "Composite test" );

    const char* desoutput = "Hello Dear 1";

    uiString string = toUiString( "Hello %1 %2").arg( "Dear" ).arg( toString(1) );
    mRunStandardTest( string.getQtString()==QString( desoutput ),
		     "Standard argument order");

    string = toUiString( "Hello %2 %1").arg( toString( 1 ) ).arg( "Dear" );
    mRunStandardTest( string.getQtString()==QString(desoutput),
		     "Reversed argument order");

    string = toUiString( "Hello %1 %2");
    string.arg( "Dear" ).arg( toString(1) );
    mRunStandardTest( string.getQtString()==QString(desoutput),
		     "In-place");


    BufferString expargs = string.getFullString();

    mRunStandardTest( expargs==desoutput, "Argument expansion" );

    uiString cloned;
    cloned = string;
    cloned.makeIndependent();

    mRunStandardTest( string.getQtString()==cloned.getQtString(), "copyFrom" );

    uiString part1 = toUiString( "Part 1" );
    part1.append( ", Part 2", false );
    mRunStandardTest(
	    FixedString(part1.getFullString())=="Part 1, Part 2", "append" );
    part1.append( ", Part 2", true );
    mRunStandardTest(
	    FixedString(part1.getFullString())=="Part 1, Part 2\n, Part 2",
			"append with newline" );

    return true;
}


bool testSharedData()
{
    uiString a = toUiString("Hello %1%2").arg( "World" );
    uiString b = a;

    b.arg( "s" );
    mRunStandardTest( b.getFullString()=="Hello Worlds" &&
		      BufferString(a.getFullString())!=
		      BufferString(b.getFullString()), "arg on copy" );

    uiString c = b;
    c = "Another message";
    mRunStandardTest( BufferString(c.getFullString())!=
		      BufferString(b.getFullString()),
		      "assignment of copy" );

    uiString d = b;
    mRunStandardTest( d.getOriginalString()==b.getOriginalString(),
		      "Use of same buffer on normal operations" );

    return true;
}


bool testQStringAssignment()
{
    const char* message = "Hello World";
    uiString string;
    string.setFrom( QString( message ) );

    BufferString res = string.getFullString();
    mRunStandardTest( res==message, "QString assignment" );

    return true;
}


bool testOptionStrings()
{
    uiStringSet options;
    options += toUiString( "One" );
    options += toUiString( "Two" );
    options += toUiString( "Three" );
    options += toUiString( "Four" );

    mRunStandardTest(
	    options.createOptionString( true, -1, ' ').getFullString()==
	              "One, Two, Three, and Four", "createOptionString and" );
    mRunStandardTest(
	    options.createOptionString( false, -1, ' ').getFullString()==
	              "One, Two, Three, or Four", "createOptionString or" );

    mRunStandardTest(
	    options.createOptionString( false, 3, ' ').getFullString()==
	              "One, Two, Three, ...", "createOptionString limited" );

    return true;
}


bool testHexEncoding()
{
    uiString str;
    mRunStandardTest( str.setFromHexEncoded("517420697320677265617421") &&
	              str.getFullString()=="Qt is great!",
	   	      "Reading hard-coded string" );


    BufferString msg( "Espana" );
    ((unsigned char*) msg.buf() )[4] = 241;

    uiString original( msg );
    BufferString encoding;
    original.getHexEncoded( encoding );
    
    mRunStandardTest( str.setFromHexEncoded( encoding ) &&
	              original.isEqualTo(str), "Reading encoded string" );
    

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testArg() || !testSharedData() || !testQStringAssignment() ||
	 !testOptionStrings() || !testHexEncoding() )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
