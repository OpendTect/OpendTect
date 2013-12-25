/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "nrbytes2string.h"
#include "testprog.h"
#include "bufstringset.h"
#include "iopar.h"


#define mRunTest( desc, test ) \
{ \
    if ( (test) ) \
    { \
	if ( !quiet ) \
	{ \
	    od_cout() << desc << ":"; \
	    od_cout() << " OK\n"; \
	} \
    } \
    else \
    { \
	od_cout() << desc << ":"; \
	od_cout() << " Fail\n"; \
	return false; \
    } \
}


static bool testBytes2String()
{
    NrBytesToStringCreator b2s;
    b2s.setUnitFrom( 100000, true ); //hundred thoughand
    mRunTest( "kB unit (1)",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    b2s.setUnitFrom( 1000000, true ); //One million
    mRunTest( "kB unit (2)",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    b2s.setUnitFrom( 2000000, true ); //Two millions
    mRunTest( "MB unit",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB) )

    b2s.setUnitFrom( 1000000, true ); //One million
    mRunTest( "Maximum flag turned on",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB) )

    b2s.setUnitFrom( 1000000, false ); //One million
    mRunTest( "Maximum flag turned off",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    mRunTest( "Conversion test", b2s.getString( 100000 )=="97.66 kB" );

    return true;
}


template <class T>
static bool doTestStringPrecisionInAscII( T val, const char* strval, bool flt )
{
    BufferString valfmtstr = toString( val );
    BufferString testname( flt ? "Float precision " : "Double precision ", val);
    BufferString valbfstr( Conv::to<const char*>( val ) );

    mRunTest( testname.buf(), valfmtstr==valbfstr && valbfstr==strval );

    return true;
}


#define mTestStringPrecisionF(val,strval) \
    fval = (float)val; \
    if ( !doTestStringPrecisionInAscII(fval,strval,true) ) return false
#define mTestStringPrecisionD(val,strval) \
    dval = (double)val; \
    if ( !doTestStringPrecisionInAscII(dval,strval,false) ) return false


static bool testStringPrecisionInAscII()
{
    float fval; double dval;
    mTestStringPrecisionF( 0, "0" );
    mTestStringPrecisionF( 0.1f, "0.1" );
    mTestStringPrecisionF( 0.05f, "0.05" );
    mTestStringPrecisionF( 0.001f, "0.001" );
    mTestStringPrecisionF( 0.023f, "0.023" );
    mTestStringPrecisionF( 0.0001f, "0.0001" );
    mTestStringPrecisionF( 0.00001f, "1e-05" );
    mTestStringPrecisionF( 0.00000001f, "1e-08" );
    mTestStringPrecisionF( 12.345, "12.345" );
    mTestStringPrecisionF( -123456., "-123456" );
    mTestStringPrecisionF( -1.2345e11, "-1.2345e11" );
    mTestStringPrecisionF( 1.2345e11, "1.2345e11" );
    mTestStringPrecisionD( 0, "0" );
    mTestStringPrecisionD( 0.1, "0.1" );
    mTestStringPrecisionD( 0.05, "0.05" );
    mTestStringPrecisionD( 0.001, "0.001" );
    mTestStringPrecisionF( 0.023, "0.023" );
    mTestStringPrecisionD( 0.0001, "0.0001" );
    mTestStringPrecisionD( 0.00001, "1e-05" );
    mTestStringPrecisionD( 0.00000001, "1e-08" );
    mTestStringPrecisionD( 12.345, "12.345" );
    mTestStringPrecisionD( -123456., "-123456" );
    mTestStringPrecisionD( -1.2345e11, "-1.2345e11" );
    mTestStringPrecisionD( 1.2345e11, "1.2345e11" );
    return true;
}


static bool testBufferStringFns()
{
    BufferString bs( "Tok1 Tok1 Tok2 Tok3 Tok4" );
    bs.replace( '4', '5' );
    bs.replace( "Tok1", 0 );
    bs.replace( "Tok2", " " );
    bs.replace( "Tok3", "X" );
    mRunTest("BufferString replace",bs == "    X Tok5");
    bs.trimBlanks();
    mRunTest("BufferString trimBlanks 1",bs == "X Tok5");
    bs = "\nXX\tYY Z\t";
    bs.trimBlanks();
    mRunTest("BufferString trimBlanks 2",bs == "XX\tYY Z");
    mRunTest("BufferString count",bs.count('Y')==2);
    mRunTest("BufferString contains 1",bs.contains(" Z"));
    mRunTest("BufferString contains 2",!bs.contains("\n"));
    bs = "\nXX\tYY Z\t";
    bs.remove( '\t' );
    mRunTest("BufferString remove 1",bs == "\nXXYY Z");
    bs.remove( "XX" );
    mRunTest("BufferString remove 2",bs == "\nYY Z");
    bs = "XXYYZ";
    bs.embed( '^', '+' );
    mRunTest("BufferString embed",bs == "^XXYYZ+");
    bs.quote( '=' );
    mRunTest("BufferString quote",bs == "=^XXYYZ+=");
    bs.unQuote( '=' );
    mRunTest("BufferString unQuote",bs == "^XXYYZ+");
    bs.unEmbed( '^', '+' );
    mRunTest("BufferString unEmbed",bs == "XXYYZ");

    return true;
}


#define mRunFirstOccTest(tofind,offs) \
    mRunTest( BufferString("firstOcc ",tofind), \
	    firstOcc(str,tofind)-str == offs)

#define mRunLastOccTest(tofind,offs) \
    mRunTest( BufferString("lastOcc ",tofind), \
	    lastOcc(str,tofind)-str == offs)

static bool testOccFns()
{
    const char* str = "[BEGIN] Tok1 Tok2 Tok1 Tok3 [END]";
    const int len = FixedString(str).size();

    mRunFirstOccTest('N',5);
    mRunFirstOccTest("N",5);
    mRunFirstOccTest("Tok1",8);
    mRunFirstOccTest("",0);

    mRunLastOccTest('N',len-3);
    mRunLastOccTest("N",len-3);
    mRunLastOccTest("Tok1",len-15);
    mRunLastOccTest("",len);

    mRunTest( "firstOcc non-exist", firstOcc(str,"Tok4") == 0 )
    mRunTest( "firstOcc non-exist", firstOcc(str,'X') == 0 )
    mRunTest( "lastOcc non-exist", lastOcc(str,"Tok4") == 0 )
    mRunTest( "lastOcc non-exist", lastOcc(str,'X') == 0 )

    return true;
}

#define mDefVarDef(typ,nm,val) \
    const typ##VarDef nm = { val, #val }

#define mPrNumb(var,maxsz) \
    str = toString( var.v_, maxsz ); \
    od_cout() << var.desc_ << ' ' << maxsz << ": \"" << str << '"' << od_endl

static bool testLimFToStringFns()
{
    if ( quiet ) return true;

    struct floatVarDef { float v_; const char* desc_; };
    struct doubleVarDef { double v_; const char* desc_; };
    mDefVarDef( float, f1, 0.1234567f );
    mDefVarDef( float, f2, 0.1234567e10f );
    mDefVarDef( float, f3, 1.234567e-8f );
    mDefVarDef( float, f4, -1.234567e-8f );
    mDefVarDef( double, d1, 0.123456789 );
    mDefVarDef( double, d2, 0.123456789e10 );
    mDefVarDef( double, d3, 1.23456789e-8 );
    mDefVarDef( double, d4, -1.23456789e-8 );

    BufferString str;
    mPrNumb(f1,6); mPrNumb(f1,8); mPrNumb(f1,10); mPrNumb(f1,12);
    mPrNumb(f2,6); mPrNumb(f2,8); mPrNumb(f2,10); mPrNumb(f2,12);
    mPrNumb(f3,6); mPrNumb(f3,8); mPrNumb(f3,10); mPrNumb(f3,12);
    mPrNumb(f4,6); mPrNumb(f4,8); mPrNumb(f4,10); mPrNumb(f4,12);
    mPrNumb(d1,6); mPrNumb(d1,8); mPrNumb(d1,10); mPrNumb(d1,12);
    mPrNumb(d2,6); mPrNumb(d2,8); mPrNumb(d2,10); mPrNumb(d2,12);
    mPrNumb(d3,6); mPrNumb(d3,8); mPrNumb(d3,10); mPrNumb(d3,12);
    mPrNumb(d4,6); mPrNumb(d4,8); mPrNumb(d4,10); mPrNumb(d4,12);

    return true;
}



int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testBytes2String()
      || !testStringPrecisionInAscII()
      || !testBufferStringFns()
      || !testOccFns()
      || !testLimFToStringFns() )
	ExitProgram( 1 );

    BufferStringSet strs;
    strs.add( "Str pos 0" );
    strs.add( "" );
    strs.add( "Str pos 2" );
    IOPar iop; strs.fillPar( iop );
    strs.setEmpty(); strs.usePar( iop );
    mRunTest( "BufferString use/fillPar test", strs.get(2)=="Str pos 2" );

    if ( !quiet )
    {
	FixedString str( 0 );
	od_cout() << "Should be empty: '" << str << "'" << od_endl;
    }

    return ExitProgram( 0 );
}
