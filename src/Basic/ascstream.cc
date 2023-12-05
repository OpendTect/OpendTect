/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "string2.h"
#include "timefun.h"
#include "convert.h"
#include "odver.h"
#include "separstr.h"
#include "ptrman.h"
#include "od_istream.h"
#include "od_ostream.h"

static const char* valsep_replacement = "\\:";
static const char* newline_replacement = "#-NL-#";

#define mChckStrm(act) if ( !strm_.isOK() ) { act; }


static BufferString& getPVN()
{
    mDefineStaticLocalObject( PtrMan<BufferString>, projvernm,
			      = new BufferString );

    return *projvernm;
}


extern "C" const char* GetProjectVersionName()
{
    BufferString& pvn = getPVN();
    if ( pvn.isEmpty() )
	pvn.set( "dTect V" ).add( mODFullVersion );

    return pvn.buf();
}


mExternC( Basic)  void SetProjectVersionName(const char*);
extern "C" void SetProjectVersionName( const char* s )
{
    getPVN() = s;
}


//--- ascostream

#define mImplConstr(var,ismine) : strm_(var), strmmine_(ismine)	{}

ascostream::ascostream( od_ostream& strm )
    mImplConstr( strm, false )
ascostream::ascostream( od_ostream* strm )
    mImplConstr( strm ? *strm : od_ostream::nullStream(), strm )
ascostream::~ascostream()
    { if ( strmmine_ ) delete &strm_; }
bool ascostream::isOK() const
    { return strm_.isOK(); }


bool ascostream::putHeader( const char* fltyp )
{
    mChckStrm(return false)
    strm_ << GetProjectVersionName() << od_newline << fltyp << od_newline
	     << Time::getISODateTimeString() << od_newline;
    newParagraph();
    return strm_.isOK();
}


void ascostream::newParagraph()
{
    mChckStrm(return)
    strm_ << mAscStrmParagraphMarker << od_endl;
}


void ascostream::putKeyword( const char* keyword, bool withsep )
{
    if ( !isOK() || !keyword || !*keyword ) return;

    BufferString towrite = keyword;
    char* ptr = firstOcc( towrite.getCStr(), mAscStrmKeyValSep );
    while ( ptr )
    {
	BufferString tmp( ptr + 1 );
	*ptr = 0;
	towrite += valsep_replacement;
	const od_int64 prevlen = towrite.size();
	towrite += tmp;
	ptr = firstOcc( towrite.getCStr() + prevlen, mAscStrmKeyValSep );
    }

    strm_ << towrite;
    if ( withsep )
	strm_ << mAscStrmKeyValSep << ' ';
}


bool ascostream::put( const char* keyword, const char* value )
{
    putKeyword( keyword, true );
    mChckStrm(return false)

    if ( value )
    {
	const char* nlptr = firstOcc( value, '\n' );
	if ( nlptr )
	{
	    BufferString str( value );
	    char* startptr = str.getCStr();
	    char* ptr = startptr + (nlptr - value);
	    while ( ptr )
	    {
		*ptr++ = '\0';
		strm_ << startptr << newline_replacement;
		startptr = ptr;
		ptr = firstOcc( ptr, '\n' );
	    }
	    value += startptr - str.buf();
	}
	strm_ << value;
    }

    strm_ << od_endl;
    return strm_.isOK();
}


bool ascostream::put( const char* keyword, const OD::String& value )
{
    return put( keyword, value.buf() );
}


bool ascostream::put( const char* kw, const MultiID& key )
{
    return put( kw, key.toString() );
}


#define mDeclPut1IFn(typ) \
bool ascostream::put( const char* keyword, typ value ) \
{ \
    mChckStrm(return false) \
    putKeyword( keyword ); strm_ << value << od_newline; \
    return strm_.isOK(); \
}
mDeclPut1IFn(int)
mDeclPut1IFn(od_uint32)
mDeclPut1IFn(od_int64)
mDeclPut1IFn(od_uint64)


bool ascostream::put( const char* keyword, float value )
{
    return mIsUdf(value) ? strm_.isOK() : put( keyword, toString(value) );
}


bool ascostream::put( const char* keyword, double value )
{
    return mIsUdf(value) ? strm_.isOK()
	 : put( keyword, toString(value) );
}


bool ascostream::putYN( const char* keyword, bool yn )
{
    putKeyword( keyword );
    mChckStrm(return false)
    strm_ << getYesNoString(yn) << od_newline;
    return strm_.isOK();
}


#define mDeclPut2Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; \
    putKeyword( keyword ); strm_ << fms << od_newline; \
    return strm_.isOK(); \
}
mDeclPut2Fn(put,int)
mDeclPut2Fn(put,od_uint32)
mDeclPut2Fn(put,od_int64)
mDeclPut2Fn(put,od_uint64)
mDeclPut2Fn(put,float)
mDeclPut2Fn(put,double)
mDeclPut2Fn(putYN,bool)


#define mDeclPut3Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2, typ v3 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; fms += v3; \
    putKeyword( keyword ); strm_ << fms << od_newline; \
    return strm_.isOK(); \
}
mDeclPut3Fn(put,int)
mDeclPut3Fn(put,od_uint32)
mDeclPut3Fn(put,od_int64)
mDeclPut3Fn(put,od_uint64)
mDeclPut3Fn(put,float)
mDeclPut3Fn(put,double)
mDeclPut3Fn(putYN,bool)


#define mDeclPut4Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2, typ v3, typ v4 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; fms += v3; fms += v4; \
    putKeyword( keyword ); strm_ << fms << od_newline; \
    return strm_.isOK(); \
}
mDeclPut4Fn(put,int)
mDeclPut4Fn(put,od_uint32)
mDeclPut4Fn(put,od_int64)
mDeclPut4Fn(put,od_uint64)
mDeclPut4Fn(put,float)
mDeclPut4Fn(put,double)
mDeclPut4Fn(putYN,bool)


bool ascostream::copyFile( const char* src, const char* dest )
{
    if ( !File::isReadable( src ) || File::exists( dest ) )
	return false;

    FilePath srcfp( src ), destfp( dest );
    od_istream srcstrm( srcfp );
    od_ostream deststrm( dest );
    if ( !srcstrm.isOK() || !deststrm.isOK() )
	return false;
    ascistream in( srcstrm );
    ascostream out( deststrm );
    out.putHeader( in.fileType() );
    in.next();

    while ( in.type()!=ascistream::EndOfFile )
    {
	switch( in.type() )
	{
	    case ascistream::ParagraphMark: out.newParagraph();
		break;
	    case ascistream::KeyVal: out.put( in.keyWord(), in.value() );
		break;
	    case ascistream::Keyword: out.put( in.keyWord() );
		break;
	    default:
		break;
	}
	in.next();
    }
    return true;
}

//--- ascistream

#undef mImplConstr
#define mImplConstr(var,ismine) \
	: strm_(var), strmmine_(ismine)	{ init(rdhead); }

ascistream::ascistream( od_istream& strm, bool rdhead )
    mImplConstr( strm, false )
ascistream::ascistream( od_istream* strm, bool rdhead )
    mImplConstr( strm ? *strm : od_istream::nullStream(), strm )
ascistream::~ascistream()
    { if ( strmmine_ ) delete &strm_; }
bool ascistream::isOK() const
    { return strm_.isOK(); }


void ascistream::init( bool rdhead )
{
    filetype_ = header_ = timestamp_ = "";
    if ( !rdhead )
	return;

    od_stream_Pos strmpos = strm_.position();
    if ( strmpos > 0 )
	strm_.setReadPosition( 0 ); // Jump to the beginning to read header

    const bool res = strm_.getLine(header_) &&
		     strm_.getLine(filetype_) &&
		     strm_.getLine(timestamp_);
    if ( strmpos > 0 )
	strm_.setReadPosition( strmpos ); // Jump back to the current position
    else if ( res )
	next();

    if ( !res )
	return;

    filetype_.trimBlanks();
    if ( filetype_.size() >= 4 )
    {
	char* ptr = filetype_.getCStr() + filetype_.size() - 4;
	if ( caseInsensitiveEqual(ptr,"file",0) )
	    *ptr = '\0';
	filetype_.trimBlanks();
    }
}


bool ascistream::hasStandardHeader() const
{
    return header_.startsWith( "dTect" );
}


ascistream& ascistream::next()
{
    keybuf_.setEmpty(); valbuf_.setEmpty();
    mChckStrm(return *this)

    BufferString lineread;
    if ( !strm_.getLine(lineread) )
	return *this;

    char* linebuf = lineread.getCStr();
    if ( linebuf[0] == '\0' || ( linebuf[0]=='-' && linebuf[1]=='-' ) )
	return next();
    if ( linebuf[0] == mAscStrmParagraphMarker[0] )
	{ keybuf_ = mAscStrmParagraphMarker; return *this; }

    const int sz = lineread.size();
    char* keywptr = linebuf;
    char* valptr = nullptr;
    for ( int ich=1; ich<sz; ich++ )
    {
	if ( linebuf[ich] == mAscStrmKeyValSep && linebuf[ich-1]!='\\' )
	{
	    valptr = linebuf + ich;
	    *valptr++ = '\0';
	    mTrimBlanks(valptr);
	    break;
	}
    }

    const char keyvalsepstr[] = { mAscStrmKeyValSep, '\0' };
    mTrimBlanks( keywptr );
    keybuf_ = keywptr;
    keybuf_.replace( valsep_replacement, keyvalsepstr );
    if ( valptr )
    {
	valbuf_ = valptr;
	valbuf_.replace( newline_replacement, "\n" );
    }

    return *this;
}


bool ascistream::isOfFileType( const char* ftyp ) const
{
    return filetype_.startsWith( ftyp, OD::CaseInsensitive );
}


const char* ascistream::version() const
{
    const char* vptr = lastOcc( header_.buf(), 'V' );
    if ( vptr )
	return vptr + 1;

    return "0.0.0";
}


int ascistream::majorVersion() const
{
    const SeparString ss( version(), '.' );
    return ss.size() > 0 ? ss.getIValue(0) : 0;
}


int ascistream::minorVersion() const
{
    const SeparString ss( version(), '.' );
    return ss.size() > 1 ? ss.getIValue(1) : 0;
}


int ascistream::patchVersion() const
{
    const SeparString ss( version(), '.' );
    return ss.size() > 2 ? ss.getIValue(2) : 0;
}


ascistream::EntryType ascistream::type() const
{
    if ( !strm_.isOK() )
	return EndOfFile;
    if ( keybuf_.isEmpty() )
	return Empty;
    if ( *keybuf_.buf() == *mAscStrmParagraphMarker )
	return ParagraphMark;
    if ( valbuf_.isEmpty() )
	return Keyword;

    return KeyVal;
}


bool ascistream::hasKeyword( const char* keyw ) const
{
    if ( !keyw ) return keybuf_.isEmpty();
    mSkipBlanks(keyw);
    return keybuf_ == keyw;
}


bool ascistream::hasValue( const char* val ) const
{
    if ( !val ) return valbuf_.isEmpty();
    mSkipBlanks(val);
    return valbuf_ == val;
}


#define mDeclGetFn(typ,fn) \
typ ascistream::get##fn( int idx ) const \
{ \
    if ( idx < 1 ) return Conv::to<typ>( valbuf_.buf() ); \
    FileMultiString fms( valbuf_.buf() ); \
    return Conv::to<typ>( fms[idx] ); \
}

mDeclGetFn(int,IValue)
mDeclGetFn(od_uint32,UIValue)
mDeclGetFn(od_int64,I64Value)
mDeclGetFn(od_uint64,UI64Value)
mDeclGetFn(float,FValue)
mDeclGetFn(double,DValue)
mDeclGetFn(bool,YN)
