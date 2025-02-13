/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "envvars.h"
#include "oddirs.h"
#include "testprog.h"
#include "uistrings.h"

#include <QByteArray>
#include <QString>
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


template <class T>
static bool doTestStringPrecisionIntegers( T val, od_uint16 width,
                                           const char* strval, const char* desc)
{
    const uiString string = toUiString( val, width );
    const BufferString bstostring = string.getString();
    BufferString testname( "Number string for ", desc );
    testname.addSpace().add( strval );

    mRunStandardTest( bstostring == strval, testname.str() );

    return true;
}

#define mTestStringPrecisionI(val,strval,desc) \
    if ( !doTestStringPrecisionIntegers(val,width,strval,desc) ) return false

bool testIntegersNumberStrings()
{
    const short psval =  31245;
    const short msval = -31245;
    const unsigned short usval = 51235;
    const od_int32 pi32val =  2147483640;
    const od_int32 mi32val = -2147483640;
    const od_uint32 ui32val = 4294967290;
    const od_int64 pi64val =  9223372036854775703LL;
    const od_int64 mi64val = -9223372036854775703LL;
    const od_uint64 ui64val = 18446744073709551610ULL;

    od_uint16 width = 0;
    mTestStringPrecisionI( psval, "31,245", "positive short" );
    mTestStringPrecisionI( msval, "-31,245", "negative short" );
    mTestStringPrecisionI( usval, "51,235", "unsigned short" );
    mTestStringPrecisionI( pi32val, "2,147,483,640", "positive signed int" );
    mTestStringPrecisionI( mi32val, "-2,147,483,640", "negative signed int" );
    mTestStringPrecisionI( ui32val, "4,294,967,290", "unsigned int" );
    mTestStringPrecisionI( pi64val, "9,223,372,036,854,775,703",
                           "positive signed long long int" );
    mTestStringPrecisionI( mi64val, "-9,223,372,036,854,775,703",
                           "negative signed long long int" );
    mTestStringPrecisionI( ui64val, "18,446,744,073,709,551,610",
                           "unsigned long long int" );

    width = 8;
    mTestStringPrecisionI( msval, " -31,245", "negative short with padding" );

    return true;
}


bool testFPNumberStringsPrecision()
{
    const float val = 0.9f;
    const int prec = 3;
    uiString string = toUiString( val, 0, 'f', prec );
    QString qstr;
    string.fillQString( qstr );
    BufferString bstr( qstr );
    mRunStandardTest( bstr=="0.900", "Number string from QString" );

    ArrPtrMan<wchar_t> wbuf = string.createWCharString();
    qstr = QString::fromWCharArray( wbuf.ptr() );
    bstr = BufferString( qstr );
    mRunStandardTest( bstr=="0.900", "Number string from wchar" );

    bstr = string.getString();
    mRunStandardTest( bstr=="0.900", "Number string from uiString::getString");
    bstr = toString( string );
    mRunStandardTest( bstr=="0.900", "Number string from toString(uiString&)");

    string = toUiString( val, 0, 'g', prec ); bstr = string.getString();
    mRunStandardTest( bstr=="0.9","Number string with float precision 3 - 'g'");

    const double dval = 0.9;
    string = toUiString( dval, 0, 'f', prec ); bstr = string.getString();
    mRunStandardTest( bstr=="0.900", "Number string with double precision 3" );

    string = toUiString( dval, 0, 'g', prec ); bstr = string.getString();
    mRunStandardTest( bstr=="0.9","Number string with double precision 3 - 'g'");

    return true;
}


template <class T>
static bool doTestStringPrecisionFP( T val, int precision, const char* strval,
				     const char* desc, bool flt )
{
    const uiString string = toUiString( val, precision );
    const BufferString bstostring = string.getString();
    BufferString testname( flt ? "Float" : "Double", " precision string for " );
    testname.add( desc ).addSpace().add( strval );

    mRunStandardTest( bstostring == strval, testname.str() );

    return true;
}

#define mTestStringPrecisionF(val,strval,desc) \
    fval = (float)val; \
    if ( !doTestStringPrecisionFP(fval,width,strval,desc,true) ) return false
#define mTestStringPrecisionD(val,strval,desc) \
    dfval = val; \
    if ( !doTestStringPrecisionFP(fval,width,strval,desc,false) ) return false

static bool testFPNumberStringsFormat()
{
    BufferString bstr;

    float fval = 0.023f;
    uiString string = toUiString( fval ); bstr = string.getString();
    mRunStandardTest( bstr=="0.023", "Float number string" );
    double dval = 0.023;
    string = toUiString( dval ); bstr = string.getString();
    mRunStandardTest( bstr=="0.023", "Double number string" );

    fval = 1245.23f;
    string = toUiString( fval ); bstr = string.getString();
    mRunStandardTest( bstr=="1,245.23", "Float number string" );
    string = toUiString( fval, 0, 'f', 2 ); bstr = string.getString();
    mRunStandardTest( bstr=="1,245.23", "Number string with precision 2" );
    string = toUiString( fval, 10, 'f', 2 ); bstr = string.getString();
    mRunStandardTest( bstr=="  1,245.23",
		      "Float number string with width 10 and precision 2" );

    dval = 1245.23;
    string = toUiString( dval ); bstr = string.getString();
    mRunStandardTest( bstr=="1,245.23", "Double number string" );
    string = toUiString( dval, 0, 'f', 2 ); bstr = string.getString();
    mRunStandardTest( bstr=="1,245.23", "Number string with precision 2" );
    string = toUiString( dval, 10, 'f', 2 ); bstr = string.getString();
    mRunStandardTest( bstr=="  1,245.23",
		      "Double Number string with width 10 and precision 2" );

    return true;
}


bool testLargeNumberStrings()
{
    uiString string = toUiString( 12500 );
    QString qstr; string.fillQString( qstr );
    BufferString bstr( qstr );
    mRunStandardTest( bstr=="12,500", "Large number string" );

    ArrPtrMan<wchar_t> wbuf = string.createWCharString();
    qstr = QString::fromWCharArray( wbuf.ptr() );
    bstr = BufferString( qstr );
    mRunStandardTest( bstr=="12,500", "Large number string from wchar" );

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

    SetEnvVar( "OD_USE_LOCALE_NUMBERS", "Yes" );

    if ( !testArg() ||
	 !testSharedData() ||
	 !testQStringAssignment() ||
	 !testOptionStrings() ||
	 !testHexEncoding() ||
	 !testIsEqual() ||
	 !testSetEmpty() ||
	 !testIntegersNumberStrings() ||
	 !testFPNumberStringsPrecision() ||
	 !testFPNumberStringsFormat() ||
	 !testLargeNumberStrings() ||
	 !testToLower() ||
	 !fromBufferStringSetToUiStringSet() )
	return 1;

    return 0;
}
