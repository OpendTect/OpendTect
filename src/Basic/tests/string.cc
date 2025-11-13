/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "dbkey.h"
#include "iopar.h"
#include "nrbytes2string.h"
#include "perthreadrepos.h"
#include "stringbuilder.h"
#include "testprog.h"

#ifdef __win__
# include "windows.h"
#endif


static const char* getStringUsingBuf()
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
	ret.add( "Some content" );

    return ret.str();
}


static const OD::String& getStringFromRef()
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
	ret.add( "Some content" );

    return ret;
}


static bool testWords()
{
    const char* inp = "The sentence 'this one' is not \"that one\"";
    BufferStringSet words;
    words.addWordsFrom( inp );
    mRunStandardTest( words.size()==6, "addWordsFrom size" );
    mRunStandardTest( words.get(2)=="this one", "addWordsFrom part 1" );
    mRunStandardTest( words.get(5)=="that one", "addWordsFrom part 2" );
    words.setEmpty();
    words.addWordsFrom( nullptr );
    mRunStandardTest( words.isEmpty(), "addWordsFrom null string" );
    words.addWordsFrom( "" );
    mRunStandardTest( words.isEmpty(), "addWordsFrom empty string" );
    words.addWordsFrom( "    " );
    mRunStandardTest( words.isEmpty(), "addWordsFrom blanks string" );
    words.addWordsFrom( "''" );
    mRunStandardTest( words.size()==1, "addWordsFrom empty word size" );
    mRunStandardTest( words.get(0).isEmpty(),
		      "addWordsFrom empty word content" );

    return true;
}

static bool testBuilder()
{
    StringBuilder sb;
    sb.set( "Apenoot" ).addSpace( 3 ).add( "Yo" );
    BufferString res( sb.result() );
    mRunStandardTest( res=="Apenoot   Yo", "StringBuilder result (1)" );

    sb.addNewLine( 300 ).add( "X" );
    res = sb.result();
    mRunStandardTest( res[11]=='o' && res[12]=='\n'
		      && res[300+11]=='\n' && res[301+11]=='X',
			"StringBuilder result (2)" );

    return true;
}


static bool testTruncate()
{
    BufferString longstring( "Hello world! Lets solve all poverty today!");
    truncateString( longstring.getCStr(), 16 );

    mRunStandardTest( longstring=="Hello world! ...", "Truncate string" );
    mRunStandardTest( longstring.size()==16, "Truncate string size" );

    return true;
}


static bool testBytes2String()
{
    NrBytesToStringCreator b2s;
    b2s.setUnitFrom( 100000, true ); //hundred thoughand
    mRunStandardTest(
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB),
	     "kB unit (1)" )

    b2s.setUnitFrom( 1000000, true ); //One million
    mRunStandardTest(
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB),
	     "kB unit (2)" )

    b2s.setUnitFrom( 2000000, true ); //Two millions
    mRunStandardTest(
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB),
	     "MB unit" )

    b2s.setUnitFrom( 1000000, true ); //One million
    mRunStandardTest(
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB),
	     "Maximum flag turned on" )

    b2s.setUnitFrom( 1000000, false ); //One million
    mRunStandardTest(
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB),
	     "Maximum flag turned off" )

    mRunStandardTest( b2s.getString( 100000 )=="97.66 kB", "Conversion test" );

    return true;
}


template <class T>
static bool doTestStringPrecisionIntegers( T val, od_uint16 width,
					   const char* strval, const char* desc)
{
    const BufferString bstostring = toString( val, width );
    BufferString testname( "Number string for ", desc );
    testname.addSpace().add( val );

    T retval;
    Conv::set<T>( retval, bstostring.buf() );
    mRunStandardTest( bstostring == strval && retval==val, testname.str() );

    return true;
}

#define mTestStringPrecisionI(val,strval,desc) \
    if ( !doTestStringPrecisionIntegers(val,width,strval,desc) ) return false

static bool testStringPrecisionIntegers()
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
    mTestStringPrecisionI( psval, "31245", "positive short" );
    mTestStringPrecisionI( msval, "-31245", "negative short" );
    mTestStringPrecisionI( usval, "51235", "unsigned short" );
    mTestStringPrecisionI( pi32val, "2147483640", "positive signed int" );
    mTestStringPrecisionI( mi32val, "-2147483640", "negative signed int" );
    mTestStringPrecisionI( ui32val, "4294967290", "unsigned int" );
    mTestStringPrecisionI( pi64val, "9223372036854775703",
			   "positive signed long long int" );
    mTestStringPrecisionI( mi64val, "-9223372036854775703",
			   "negative signed long long int" );
    mTestStringPrecisionI( ui64val, "18446744073709551610",
			   "unsigned long long int" );

    width = 8;
    mTestStringPrecisionI( msval, "  -31245", "negative short with padding" );

    return true;
}


template <class T>
static bool doTestStringPrecisionInAscII( T val, const char* strval, bool flt )
{
    const BufferString bstostring = toStringPrecise( val );
    const BufferString testname( flt ? "Float precision " : "Double precision ",
				 strval );
    T retval;
    Conv::set<T>( retval, bstostring );
    mRunStandardTest( bstostring == strval && retval==val, testname.buf() )

    return true;
}


#define mTestStringPrecisionF(val,strval) \
    fval = (float)val; \
    if ( !doTestStringPrecisionInAscII(fval,strval,true) ) return false
#define mTestStringPrecisionD(val,strval) \
    dval = val; \
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
    mTestStringPrecisionF( -1.2345e11, "-1.2345e+11" );
    mTestStringPrecisionF( 1.2345e11, "1.2345e+11" );
    mTestStringPrecisionD( 0, "0" );
    mTestStringPrecisionD( 0.1, "0.1" );
    mTestStringPrecisionD( 0.05, "0.05" );
    mTestStringPrecisionD( 0.001, "0.001" );
    mTestStringPrecisionD( 0.023, "0.023" );
    mTestStringPrecisionD( 0.0001, "0.0001" );
    mTestStringPrecisionD( 0.00001, "1e-05" );
    mTestStringPrecisionD( 0.00000001, "1e-08" );
    mTestStringPrecisionD( 12.345, "12.345" );
    mTestStringPrecisionD( -123456., "-123456" );
    mTestStringPrecisionD( -1.2345e11, "-123450000000" );
    mTestStringPrecisionD( 1.2345e11, "123450000000" );
    mTestStringPrecisionD( 1.2345e16, "1.2345e+16" );
    mTestStringPrecisionD( 1.6e-5, "1.6e-05" );
    mTestStringPrecisionD( 1.5e-5, "1.5e-05" );
    mTestStringPrecisionD( 55.0554844553, "55.0554844553" );
    mTestStringPrecisionD( 55.05548445533, "55.05548445533" );
    mTestStringPrecisionD( 55.05548445535, "55.05548445535" );
    mTestStringPrecisionD( 55.055484455333, "55.055484455333" );
    mTestStringPrecisionD( 55.055484455335, "55.055484455335" );
    mTestStringPrecisionD( 55.0554844553334, "55.0554844553334" );
    mTestStringPrecisionD( 5.50554844553e50, "5.50554844553e+50" );
    return true;
}


template <class T>
static bool doTestStringCPrecisionInAscII( T val, const char* fmt,
					   const char* strval, bool flt )
{
    const BufferString bstostring = toStringCFmt( val, fmt );
    const BufferString testname( flt ? "Float c-format precision "
				     : "Double c-format precision ",
				 strval );
    BufferString errmsg( "Expected: ", strval, "; Received: " );
    errmsg.add( bstostring );
    mRunStandardTestWithError( bstostring == strval, testname, errmsg );

    return true;
}

#undef mTestStringPrecisionF
#define mTestStringPrecisionF(val,cformat,strval) \
    fval = (float)val; \
    if ( !doTestStringCPrecisionInAscII(fval,cformat,strval,true) ) return false
#undef mTestStringPrecisionD
#define mTestStringPrecisionD(val,cformat,strval) \
    dval = val; \
    if ( !doTestStringCPrecisionInAscII(dval,cformat,strval,false)) return false

static bool testStringCPrecisionInAscII()
{
    float fval; double dval;

    mTestStringPrecisionF( 0.023f, "%f", "0.023000" );
    mTestStringPrecisionF( 0.023f, "%6.2f", "  0.02" );
    mTestStringPrecisionF( 1245.23f, "%f", "1245.229980" );
    mTestStringPrecisionF( 1245.23f, "%7.2f", "1245.23" );
    mTestStringPrecisionF( 1245.23f, "%10.2f", "   1245.23" );
    mTestStringPrecisionD( 0.023, "%f", "0.023000" );
    mTestStringPrecisionD( 0.023, "%6.2f", "  0.02" );
    mTestStringPrecisionD( 1245.23, "%f", "1245.230000" );
    mTestStringPrecisionD( 1245.23, "%7.2f", "1245.23" );
    mTestStringPrecisionD( 1245.23, "%10.2f", "   1245.23" );

    return true;
}

template <class T>
static bool doTestStringCPrecisionNrDec( T val, int nrdec,
					 const char* strval, bool flt )
{
    const BufferString bstostring = toStringDec( val, nrdec );
    BufferString testname( flt ? "Float nrdec: "
			       : "Double nrdec: ", nrdec );
    testname.add( "; For: " ).add( strval );
    BufferString errmsg( "Expected: ", strval, "; Received: " );
    errmsg.add( bstostring );
    mRunStandardTestWithError( bstostring == strval, testname, errmsg );

    return true;
}

#undef mTestStringPrecisionF
#define mTestStringPrecisionF(val,nrdec,strval) \
    fval = (float)val; \
    if ( !doTestStringCPrecisionNrDec(fval,nrdec,strval,true) ) return false
#undef mTestStringPrecisionD
#define mTestStringPrecisionD(val,nrdec,strval) \
    dval = val; \
    if ( !doTestStringCPrecisionNrDec(dval,nrdec,strval,false)) return false

static bool testStringCPrecisionNrDec()
{
    float fval; double dval;

    mTestStringPrecisionF( 0.023f, 0, "0" );
    mTestStringPrecisionF( 0.023f, 1, "0.0" );
    mTestStringPrecisionF( 0.023f, 2, "0.02" );
    mTestStringPrecisionF( 0.023f, 3, "0.023" );
    mTestStringPrecisionF( 0.023f, 4, "0.0230" );
    mTestStringPrecisionF( 1245.23f, 0, "1245" );
    mTestStringPrecisionF( 1245.23f, 1, "1245.2" );
    mTestStringPrecisionF( 1245.23f, 2, "1245.23" );
    mTestStringPrecisionF( 1245.23f, 3, "1245.230" );
    mTestStringPrecisionF( -1.23456e-5f, 0, "-1e-05" );
    mTestStringPrecisionF( -1.23456e-5f, 1, "-1.2e-05" );
    mTestStringPrecisionF( -1.23456e-5f, 2, "-1.23e-05" );
    mTestStringPrecisionF( -1.23456e-5f, 3, "-1.235e-05" );
    mTestStringPrecisionF( -1.23456e-5f, 4, "-1.2346e-05" );
    mTestStringPrecisionF( 1234523.789f, 0, "1e+06" );
    mTestStringPrecisionF( 1234523.789f, 1, "1.2e+06" );
    mTestStringPrecisionF( 1234523.789f, 2, "1.23e+06" );
    mTestStringPrecisionF( 1234523.789f, 3, "1.235e+06" );
    mTestStringPrecisionF( 1234523.789f, 4, "1.2345e+06" );
    mTestStringPrecisionD( 0.023, 0, "0" );
    mTestStringPrecisionD( 0.023, 1, "0.0" );
    mTestStringPrecisionD( 0.023, 2, "0.02" );
    mTestStringPrecisionD( 0.023, 3, "0.023" );
    mTestStringPrecisionD( 0.023, 4, "0.0230" );
    mTestStringPrecisionD( 1245.23, 0, "1245" );
    mTestStringPrecisionD( 1245.23, 1, "1245.2" );
    mTestStringPrecisionD( 1245.23, 2, "1245.23" );
    mTestStringPrecisionD( 1245.23, 3, "1245.230" );
    mTestStringPrecisionD( -1.23456e-5, 0, "-1e-05" );
    mTestStringPrecisionD( -1.23456e-5, 1, "-1.2e-05" );
    mTestStringPrecisionD( -1.23456e-5, 2, "-1.23e-05" );
    mTestStringPrecisionD( -1.23456e-5, 3, "-1.235e-05" );
    mTestStringPrecisionD( -1.23456e-5, 4, "-1.2346e-05" );
    mTestStringPrecisionD( 1234523.789, 0, "1e+06" );
    mTestStringPrecisionD( 1234523.789, 1, "1.2e+06" );
    mTestStringPrecisionD( 1234523.789, 2, "1.23e+06" );
    mTestStringPrecisionD( 1234523.789, 3, "1.235e+06" );
    mTestStringPrecisionD( 1234523.789, 4, "1.2345e+06" );

    return true;
}

static bool testStringPtr()
{
    const int expsz = __iswin__ ? 16 : (__ismac__ ? 11 : 14);
    const double a = 3.14;
    BufferString ret;
    ret.setPtr( &a );
    mRunStandardTest( ret.size() == expsz, "BufferString::toStringPtr" );

    const BufferString teststr( "some text" );
    ret.setEmpty().set( toStringPtr(&teststr) );
    mRunStandardTest( ret.size() == expsz, "Global toStringPtr" );

    return true;
}


static bool testBufferStringFns()
{
    BufferString bs( "Tok1 Tok1 Tok2 Tok3 Tok4" );
    bs.replace( '4', '5' );
    bs.replace( "Tok1", 0 );
    bs.replace( "Tok2", " " );
    bs.replace( "Tok3", "X" );
    mRunStandardTest( bs == "    X Tok5", "BufferString replace" );
    bs.trimBlanks();
    mRunStandardTest( bs == "X Tok5", "BufferString trimBlanks 1" );
    bs = "\nXX\tYY Z\t";
    bs.trimBlanks();
    mRunStandardTest( bs == "XX\tYY Z", "BufferString trimBlanks 2" );
    mRunStandardTest( bs.count('Y')==2, "BufferString count" );
    mRunStandardTest( bs.contains(" Z"), "BufferString contains 1" );
    mRunStandardTest( !bs.contains("\n"), "BufferString contains 2" );
    bs = "\nXX\tYY Z\t";
    bs.remove( '\t' );
    mRunStandardTest( bs == "\nXXYY Z", "BufferString remove 1" );
    bs.remove( "XX" );
    mRunStandardTest( bs == "\nYY Z", "BufferString remove 2" );
    bs = "XXYYZ";
    bs.embed( '^', '+' );
    mRunStandardTest( bs == "^XXYYZ+", "BufferString embed" );
    bs.quote( '=' );
    mRunStandardTest( bs == "=^XXYYZ+=", "BufferString quote" );
    bs.unQuote( '=' );
    mRunStandardTest( bs == "^XXYYZ+", "BufferString unQuote" );
    bs.unEmbed( '^', '+' );
    mRunStandardTest( bs == "XXYYZ", "BufferString unEmbed" );

#define mSetBSToInit() bs = "\nXX$.YY/:\\ Z\t"
    mSetBSToInit(); bs.clean( BufferString::OnlyAlphaNum );
    mRunStandardTest( bs == "_XX__YY____Z_", "BufferString clean OnlyAlphaNum");
    mSetBSToInit(); bs.clean( BufferString::AllowDots );
    mRunStandardTest( bs == "_XX_.YY____Z_", "BufferString clean AllowDots" );
    mSetBSToInit(); bs.clean( BufferString::NoSpaces );
    mRunStandardTest( bs == ( __iswin__ ? "_XX_.YY_:\\_Z_" : "_XX_.YY/___Z_" ),
		      "BufferString clean NoSpaces" );
    mSetBSToInit(); bs.clean( BufferString::NoFileSeps );
    mRunStandardTest(bs == "\nXX_.YY___ Z\t","BufferString clean NoFileSeps");
    mSetBSToInit(); bs.clean( BufferString::NoSpecialChars );
#ifdef __win__
    mRunStandardTest( bs == "\nXX_.YY_:\\ Z\t",
		      "BufferString clean NoSpecialChars");
    const TCHAR path[] = __TEXT( "C:\\temp\\this is a path" );
    BufferString retpath( path );
    mRunStandardTest( retpath == "C:\\temp\\this is a path",
	"TCHAR to BufferString using constructor" );
    retpath.set( "New content" ).set( path );
    mRunStandardTest( retpath == "C:\\temp\\this is a path",
	"TCHAR to BufferString using set (from non-empty)" );
    const TCHAR addedpath[] = __TEXT( "\\my folder\\dir" );
    retpath.add( addedpath );
    mRunStandardTest( retpath == "C:\\temp\\this is a path\\my folder\\dir",
	"TCHAR to BufferString using add function (from non-empty)" );
    retpath.setEmpty();
    retpath = toString( path );
    mRunStandardTest( retpath == "C:\\temp\\this is a path",
	"TCHAR to BufferString using toString global function" );
#else
    mRunStandardTest( bs == "\nXX_.YY/__ Z\t",
		      "BufferString clean NoSpecialChars" );
#endif
    return true;
}


#define mRunBSFindTest(tofind,offs) \
    mRunStandardTest( offs == str.find(tofind) - str.buf(), \
		BufferString("BufferString::find ",tofind) )

#define mRunBSFindLastTest(tofind,offs) \
    mRunStandardTest( offs == str.findLast(tofind) - str.buf(), \
		BufferString("BufferString::findLast ",tofind) )

static bool testOccFns()
{
    const BufferString str = "[BEGIN] Tok1 Tok2 Tok1 Tok3 [END]";
    const unsigned int len = str.size();

    mRunBSFindTest('N',5);
    mRunBSFindTest("N",5);
    mRunBSFindTest("Tok1",8);
    mRunBSFindTest("",0);

    mRunBSFindLastTest('N',len-3);
    mRunBSFindLastTest("N",len-3);
    mRunBSFindLastTest("Tok1",len-15);
    mRunBSFindLastTest("",len);

    mRunStandardTest( str.contains('k'), "contains exist (char)" )
    mRunStandardTest( str.contains("N]"), "contains exist (str)" )
    mRunStandardTest( !str.contains("Tok4"), "contains non-exist (char)" )
    mRunStandardTest( !str.contains('X'), "contains non-exist (str)" )

    return true;
}

#define mDefVarDef(typ,nm,val) \
    const typ##VarDef nm = { val, #val }

#define mPrNumb(var,maxsz) \
    str = toStringLim( var.v_, maxsz ); \
    logStream() << var.desc_ << ' ' << maxsz << ": \"" << str << '"' << od_endl

static bool testLimFToStringFns()
{
    struct floatVarDef { float v_; const char* desc_; };
    struct doubleVarDef { double v_; const char* desc_; };
    mDefVarDef( float, f1, 0.1234567f );
    mDefVarDef( float, f2, 0.1234567e10f );
    mDefVarDef( float, f3, 1.234567e-8f );
    mDefVarDef( float, f4, -1.234567e-8f );
    mDefVarDef( float, f5, -123456.78f );
    mDefVarDef( float, f6, 820.00006104f );
    mDefVarDef( float, f7, .0006104f );
    mDefVarDef( float, f8, +000.0006104f );
    mDefVarDef( float, f9, 0.00061040035f );
    mDefVarDef( float, f10, 0.000610400035f );
    mDefVarDef( double, d1, 0.123456789 );
    mDefVarDef( double, d2, 0.123456789e10 );
    mDefVarDef( double, d3, 1.23456789e-8 );
    mDefVarDef( double, d4, -1.23456789e-8 );
    mDefVarDef( double, d5, -123456.78 );

    BufferString str;
    mPrNumb(f1,6); mPrNumb(f1,8); mPrNumb(f1,10); mPrNumb(f1,12);
    mPrNumb(f2,6); mPrNumb(f2,8); mPrNumb(f2,10); mPrNumb(f2,12);
    mPrNumb(f3,6); mPrNumb(f3,8); mPrNumb(f3,10); mPrNumb(f3,12);
    mPrNumb(f4,6); mPrNumb(f4,8); mPrNumb(f4,10); mPrNumb(f4,12);
    mPrNumb(f5,6); mPrNumb(f5,8); mPrNumb(f5,10); mPrNumb(f5,12);
    mPrNumb(f6,6); mPrNumb(f6,8); mPrNumb(f6,10); mPrNumb(f6,12);
    mPrNumb(f7,6); mPrNumb(f7,8); mPrNumb(f7,10); mPrNumb(f7,12);
    mPrNumb(f8,6); mPrNumb(f8,8); mPrNumb(f8,10); mPrNumb(f8,12);
    mPrNumb(f9,6); mPrNumb(f9,8); mPrNumb(f9,10); mPrNumb(f9,12);
    mPrNumb(f9,13); mPrNumb(f9,14);
    mPrNumb(f10,6); mPrNumb(f10,8); mPrNumb(f10,10); mPrNumb(f10,12);
    mPrNumb(f10,13); mPrNumb(f10,14);
    mPrNumb(d1,6); mPrNumb(d1,8); mPrNumb(d1,10); mPrNumb(d1,12);
    mPrNumb(d2,6); mPrNumb(d2,8); mPrNumb(d2,10); mPrNumb(d2,12);
    mPrNumb(d3,6); mPrNumb(d3,8); mPrNumb(d3,10); mPrNumb(d3,12);
    mPrNumb(d4,6); mPrNumb(d4,8); mPrNumb(d4,10); mPrNumb(d4,12);
    mPrNumb(d5,6); mPrNumb(d5,8); mPrNumb(d5,10); mPrNumb(d5,12);

    return true;
}

static bool testEmptyStringComparison()
{
    BufferString bfstr;
    mRunStandardTest( bfstr=="", "Empty string comparison - BufferString");
    StringView fxdstr;
    mRunStandardTest( fxdstr=="", "Empty string comparison - StringView");

    return true;
}


bool testStringContentType()
{
    mRunStandardTest( isNumberString("693.56") == true, "Test number string" )
    mRunStandardTest( isNumberString("1e4") == true, "Test enumber string" )
    mRunStandardTest( isNumberString("7.53rs") == false, "Test non-num string" )
    mRunStandardTest( isNumberString("534976",true) == true, "Test int string" )
    mRunStandardTest( isNumberString("100010.4",true) == false,
		      "Test non-int string" )
    mRunStandardTest( isAlphaNumString("hagh4T90PKfr") == true,
		      "Test alpha-num string" )
    mRunStandardTest( isAlphaNumString("ghgs767#jjk") == false,
		      "Test non-alpha-num string" )
    mRunStandardTest( isBoolString("No") == true, "Test bool string" )
    mRunStandardTest( isBoolString("Nay") == false, "Test non-bool string" )

    return true;
}


bool testGetFromString()
{
    const char* str = "080";
    int val = 0;
    mRunStandardTest( getFromString(val,str,0), "Parse integer string" );
    mRunStandardTest( val==80, "Parse integer string correctly" );

    str = "0009866543578873800";
    od_int64 vall = 0;
    mRunStandardTest( getFromString(vall,str,0), "Parse int_64  string" );
    mRunStandardTest( vall==9866543578873800, "Parse int_64 string correctly" );

    str = "008738.04";
    float valf = 0;
    mRunStandardTest( getFromString(valf,str,0.0f), "Parse float string" );
    mRunStandardTest( mIsEqual(valf,8738.04,1e-3),
		      "Parse float string correctly" );

    str = "000986654.4380";
    double vald = 0;
    mRunStandardTest( getFromString(vald,str,0.0), "Parse double string" );
    mRunStandardTest( mIsEqual(vald,986654.4380,1e-5),
		      "Parse double string correctly" );

    return true;
}


bool testStaticString()
{
    FilePath fp1;
    fp1.set( getStringUsingBuf() );
    mRunStandardTest( !fp1.isEmpty(),
		      "Static const char* to buf" );

    FilePath fp2;
    fp2.set( getStringFromRef().buf() );
    mRunStandardTest( !fp2.isEmpty(), "Static buf of const OD::String& to buf");

    //new syntax
    FilePath fp3;
    fp3.set( getStringFromRef() );
    mRunStandardTest( !fp3.isEmpty(), "Static const OD::String& to buf" );

    return true;
}


bool testStringSetIOPar()
{
    BufferStringSet strs;
    strs.add( "Str pos 0" );
    strs.add( "" );
    strs.add( "Str pos 2" );
    IOPar iop; strs.fillPar( iop );
    strs.setEmpty(); strs.usePar( iop );
    mRunStandardTest( strs.get(2)=="Str pos 2",
		      "BufferString use/fillPar test" );

    return true;
}


bool testStringSetReplace()
{
    BufferStringSet strs;
    strs.add( "Str pos 0" );
    strs.add( "" );
    strs.add( "Str pos 2" );

    strs.replace( 1, "Str pos 1" );
    mRunStandardTest( strs.get(1)=="Str pos 1",
		      "BufferStringSet replace test (valid const char*)" );
    strs.replace( 1, "" );
    mRunStandardTest( strs.get(1).isEmpty(),
		      "BufferStringSet replace test (empty const char*)" );

    const BufferString replstr( "Str pos 1");
    strs.replace( 1, replstr );
    mRunStandardTest( strs.get(1)=="Str pos 1",
		      "BufferStringSet replace test (OD::String&)" );
    strs.replace( 1, BufferString::empty() );
    mRunStandardTest( strs.get(1).isEmpty(),
		      "BufferStringSet replace test (empty OD::String&)" );

    auto* newreplstr = new BufferString( "Str pos 1");
    strs.replace( 1, newreplstr );
    mRunStandardTest( strs.get(1)=="Str pos 1",
		      "BufferStringSet replace test (BufferString*)" );
    auto* newemptyreplstr = new BufferString;
    strs.replace( 1, newemptyreplstr );
    mRunStandardTest( strs.get(1).isEmpty(),
		      "BufferStringSet replace test (empty BufferString*)" );

    return true;
}


static BufferString getString( BufferString ret )
{
    return ret;
}


bool testStringConv()
{
    const StringView str( 0 );
    mRunStandardTest( str.isEmpty(),
	BufferString( "StringView(0) should be empty: '", str, "'" ) )

    const BufferString bufstr = getString( 0 );
    mRunStandardTest( bufstr.isEmpty(),
	BufferString("Implicit BufferString(0) should be empty: '",bufstr,"'") )

    const BufferString bufstr2( 0 );
    mRunStandardTest( bufstr2 == "0",
	BufferString( "Explicit BufferString(0) should not be empty: '",
		      bufstr2, "'" ) )

    const BufferString str4point9( 4.9f );
    mRunStandardTest( str4point9=="4.9", "4.9 string" )

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testBuilder()
      || !testWords()
      || !testBytes2String()
      || !testStringPrecisionIntegers()
      || !testStringPrecisionInAscII()
      || !testStringCPrecisionInAscII()
      || !testStringCPrecisionNrDec()
      || !testStringPtr()
      || !testTruncate()
      || !testBufferStringFns()
      || !testOccFns()
      || !testLimFToStringFns()
      || !testEmptyStringComparison()
      || !testStringContentType()
      || !testGetFromString()
      || !testStaticString()
      || !testStringSetIOPar()
      || !testStringSetReplace()
      || !testStringConv() )
	return 1;

    return 0;
}
