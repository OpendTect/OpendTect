/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/


#include "uistring.h"
#include "testprog.h"
#include "uistrings.h"
#include "oddirs.h"

#include <QString>
#include <QByteArray>
#include <QTranslator>


bool testSetEmpty()
{
    uiString str = toUiString("Hello");
    str.setEmpty();
    mRunStandardTest( str.isEmpty(), "isEmpty" );

    str.setFrom( QString("Hello" ) );
    str.setEmpty();
    mRunStandardTest( str.isEmpty(), "isEmpty after setFrom(QString)" );

    return true;
}


bool testArg()
{
    uiString composite = toUiString( "%1 plus %2 is %3")
		.arg( 4 )
		.arg( 5 )
		.arg( 9 );

    mRunStandardTest( composite.getString() == "4 plus 5 is 9",
		      "Composite test" );

    const char* desoutput = "Hello Dear 1";
    QString qstr, qstr2;

    uiString string = toUiString( "Hello %1 %2").arg( "Dear" ).arg(toString(1));
    string.fillQString( qstr );
    mRunStandardTest( qstr==QString( desoutput ), "Standard argument order");

    string = toUiString( "Hello %2 %1").arg( toString( 1 ) ).arg( "Dear" );
    string.fillQString( qstr );
    mRunStandardTest( qstr==QString(desoutput), "Reversed argument order");

    string = toUiString( "Hello %1 %2");
    string.arg( "Dear" ).arg( toString(1) );
    string.fillQString( qstr );
    mRunStandardTest( qstr==QString(desoutput), "In-place");


    BufferString expargs = toString( string );

    mRunStandardTest( expargs==desoutput, "Argument expansion" );

    uiString cloned;
    cloned = string;
    cloned.makeIndependent();

    string.fillQString( qstr ); cloned.fillQString( qstr2 );
    mRunStandardTest( qstr==qstr2, "copyFrom" );

    const uiString part1 = toUiString( "Part 1" );
    const uiString part2 = toUiString( "Part 2" );
    uiString res = part1; res.constructWordWith( part2 );
    mRunStandardTest( res == toUiString("Part 1Part 2"),
				"constructWordWith(no space)" );
    res = part1; res.constructWordWith( part2, true );
    mRunStandardTest( res == toUiString("Part 1 Part 2"),
				"constructWordWith(with space)" );
    res = part1; res.appendAfterList( part2 );
    mRunStandardTest( res == toUiString("Part 1\nPart 2"), "appendAfterList()");
    res = part1; res.appendPhraseSameLine( part2 );
    mRunStandardTest( res == toUiString("Part 1. Part 2"),
				"appendPhraseSameLine(Unrelated)" );
    res = part1; res.appendPhrase( part2 );
    mRunStandardTest( res == toUiString("Part 1.\nPart 2"), "appendPhrase()" );

    return true;
}


bool testSharedData()
{
    uiString a = toUiString("Hello %1%2").arg( "World" );
    uiString b = a;

    b.arg( "s" );
    mRunStandardTest( b.getString() == "Hello Worlds" &&
		      a.getString() != toString(b), "arg on copy" );

    uiString c = b;
    c = toUiString("Another message");
    mRunStandardTest( c.getString() != toString(b), "assignment of copy" );

    uiString d = b;
    mRunStandardTest( d.getOriginalString() == b.getOriginalString(),
		      "Use of same buffer on normal operations" );

    return true;
}


bool testIsEqual()
{
    const uiString a = toUiString( "A" );
    const uiString a2 = toUiString( "A" );
    const uiString b = toUiString( "B" );

    mRunStandardTest( a == a2, "Is equal is true for equal strings, test 1" );
    mRunStandardTest( a != b,
	    "Is equal is false for different strings, test 1" );

    mRunStandardTest( a2 == a, "Is equal is true for equal strings, test 2" );
    mRunStandardTest( b != a,
	    "Is equal is false for different strings, test 2" );

    return true;
}


bool testQStringAssignment()
{
    const char* message = "Hello World";
    uiString string;
    string.setFrom( QString( message ) );

    BufferString res = toString( string );
    mRunStandardTest( res==message, "QString assignment" );

    return true;
}


bool testNumberStrings()
{
    uiString string = toUiString( 0.9, 3 );
    QString qstr;
    string.fillQString( qstr );
    BufferString bstr( qstr );
    mRunStandardTest( bstr=="0.900", "Number string" );

    ArrPtrMan<wchar_t> wbuf = string.createWCharString();
    qstr = QString::fromWCharArray( wbuf );
    bstr = BufferString( qstr );
    mRunStandardTest( bstr=="0.900", "Number string from wchar" );

    return true;
}


bool testLargeNumberStrings()
{
    uiString string = toUiString( 12500 );
    QString qstr; string.fillQString( qstr );
    BufferString bstr( qstr );
    mRunStandardTest( bstr=="12500", "Large number string" );

    ArrPtrMan<wchar_t> wbuf = string.createWCharString();
    qstr = QString::fromWCharArray( wbuf );
    bstr = BufferString( qstr );
    mRunStandardTest( bstr=="12500", "Large number string from wchar" );

    return true;
}


bool testToLower()
{
    uiString string = uiStrings::phrInput( uiStrings::sHorizon().toLower() );
    BufferString bstr = toString( string );
    mRunStandardTest( bstr=="Input horizon", "To lower" );

    uiString string2 = uiStrings::phrInput( uiStrings::sHorizon() );
    bstr = toString( string2 );
    mRunStandardTest( bstr=="Input Horizon", "To lower does not affect orig" );


    return true;
}


bool testOptionStrings()
{
    uiStringSet options;
    options.add( toUiString( "One" ) )
	   .add( toUiString( "Two" ) )
	   .add( toUiString( "Three" ) )
	   .add( toUiString( "Four" ) )
	   .add( uiString() );

    mRunStandardTest(
	    options.createOptionString(true,-1,false).getString() ==
	      "One, Two, Three and Four", "createOptionString and" );
    mRunStandardTest(
	    options.createOptionString(false,-1,false).getString() ==
	      "One, Two, Three or Four", "createOptionString or" );

    mRunStandardTest(
	    options.createOptionString(false,3,false).getString() ==
	      "One, Two, Three or ...", "createOptionString limited" );

    mRunStandardTest(
	    options.createOptionString(true,-1,true).getString() ==
	      "One\nTwo\nThree and\nFour", "createOptionString nl and" );
    mRunStandardTest(
	    options.createOptionString(false,-1,true).getString() ==
	      "One\nTwo\nThree or\nFour", "createOptionString nl or" );

    mRunStandardTest(
	    options.createOptionString(false,3,true).getString() ==
	      "One\nTwo\nThree\nor ...", "createOptionString nl limited" );


    return true;
}


bool testHexEncoding()
{
    uiString str;
    mRunStandardTest( str.setFromHexEncoded("517420697320677265617421") &&
	              str.getString() == "Qt is great!",
		      "Reading hard-coded string" );


    BufferString msg( "Espana" );
    ((unsigned char*) msg.buf() )[4] = 241;

    uiString original( toUiString(msg) );
    BufferString encoding;
    original.getHexEncoded( encoding );
    str.setFromHexEncoded( encoding );
    QString qstr; str.fillQString( qstr );
    QString orgqstr; original.fillQString( orgqstr );
    mRunStandardTest( str.setFromHexEncoded( encoding ) && orgqstr==qstr,
		      "Reading encoded string" );

    return true;
}

bool fromBufferStringSetToUiStringSet()
{
    BufferStringSet strset;
    strset.add("A");
    strset.add("B");
    strset.add("C");
    uiStringSet uistrset = strset.getUiStringSet();

    BufferString str = strset.cat( " " );
    uiString uistr = uistrset.cat( uiString::Space, uiString::OnSameLine );

    mRunStandardTest( str == toString(uistr), "Comparing BuffStrSet "
				    "UiStrSet" );
    return true;

}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testArg() || !testSharedData() || !testQStringAssignment() ||
	 !testOptionStrings() || !testHexEncoding() || !testIsEqual() ||
	 !testSetEmpty() || !testNumberStrings() || !testLargeNumberStrings()
	 || !testToLower() || !fromBufferStringSetToUiStringSet() )
	return 1;

    return 0;
}
