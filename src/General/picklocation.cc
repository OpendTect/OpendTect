/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
-*/


#include "picklocation.h"
#include "trigonometry.h"
#include "bufstring.h"
#include "survinfo.h"
#include "separstr.h"
#include "posimpexppars.h"
#include <ctype.h>

static char pipechar = '|';
static char newlinechar = '\n';
namespace Pick
{
    static Location			dummyloc_( Coord3::udf() );
    static const Location		udfloc_( Coord3::udf() );
    Location& Location::dummy()		{ return dummyloc_; }
    const Location& Location::udf()	{ return udfloc_; }
}

#define mInitPtrs \
      dir_(0) \
    , trckey_(0) \
    , text_(0)


Pick::Location::Location()
    : pos_(Coord3::udf())
    , mInitPtrs
{
}

Pick::Location::Location( double x, double y, double zval )
    : pos_(x,y,zval)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord& c, float zval )
    : pos_(c,zval)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord3& c )
    : pos_(c)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord3& c, const Coord3& d )
    : pos_(c)
    , mInitPtrs
{
    dir_ = new Sphere( d );
}


Pick::Location::Location( const Coord3& c, const Sphere& d )
    : pos_(c)
    , mInitPtrs
{
    dir_ = new Sphere( d );
}

Pick::Location::Location( const Location& oth )
    : mInitPtrs
{
    *this = oth;
}


Pick::Location::~Location()
{
    delete trckey_;
    delete dir_;
    delete text_;
}


bool Pick::Location::operator ==( const Location& oth ) const
{
    return pos_ == oth.pos_ && dir() == oth.dir();
}


#define mSetMember(memb,typ,val) \
    if ( val && memb ) \
	*memb = *val; \
    else \
    { \
	delete memb; \
	if ( val ) \
	    memb = new typ( *val ); \
	else \
	    memb = 0; \
    }

Pick::Location& Pick::Location::operator=( const Location& oth )
{
    if ( this != &oth )
    {
#	define mCopyMemb( memb, typ ) mSetMember( memb, typ, oth.memb )
	pos_ = oth.pos_;
	mCopyMemb(dir_,Sphere)
	mCopyMemb(trckey_,TrcKey)
	mCopyMemb(text_,BufferString)
    }
    return *this;
}


bool Pick::Location::hasTrcKey() const
{
    return trckey_ && !trckey_->isUdf();
}


bool Pick::Location::hasDir() const
{
    return dir_ && !dir_->isNull();
}


bool Pick::Location::hasText() const
{
    return text_ && !text_->isEmpty();
}


const TrcKey& Pick::Location::trcKey() const
{
    if ( trckey_ )
	return *trckey_;
    else if ( !hasPos() )
	return TrcKey::udf();

    mDefineStaticLocalObject( TrcKey, rettk, );
    rettk.setSurvID( Survey::GM().default3DSurvID() );
    rettk.setPosition( SI().transform(pos_.getXY()) );
    return rettk;
}


bool Pick::Location::is2D() const
{
    return trcKey().is2D();
}


Pos::SurvID Pick::Location::survID() const
{
    return trcKey().survID();
}


Pos::GeomID Pick::Location::geomID() const
{
    return trcKey().geomID();
}


Pos::TraceID Pick::Location::lineNr() const
{
    return trcKey().lineNr();
}


Pos::TraceID Pick::Location::trcNr() const
{
    return trcKey().trcNr();
}


const BinID& Pick::Location::binID() const
{
    return trcKey().position();
}


const Sphere& Pick::Location::dir() const
{
    return dir_ ? *dir_ : Sphere::nullSphere();
}


const BufferString& Pick::Location::text() const
{
    return text_ ? *text_ : BufferString::empty();
}


void Pick::Location::setTK( const TrcKey* tk )
{ mSetMember( trckey_, TrcKey, tk ) }
void Pick::Location::setD( const Sphere* sph )
{ mSetMember( dir_, Sphere, sph ) }
Pick::Location& Pick::Location::setTrcKey( const TrcKey& tk )
{ setTK( tk.isUdf() ? 0 : &tk ); return *this; }
Pick::Location& Pick::Location::setDir( const Sphere& sph )
{ setD( sph.isNull() ? 0 : &sph ); return *this; }


Pick::Location& Pick::Location::setLineNr( Pos::LineID lnr )
{
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setLineNr( lnr );
    return *this;
}


Pick::Location& Pick::Location::setTrcNr( Pos::TraceID tnr )
{
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setTrcNr( tnr );
    return *this;
}


Pick::Location& Pick::Location::setGeomID( Pos::GeomID geomid )
{
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setGeomID( geomid );
    return *this;
}


Pick::Location& Pick::Location::setBinID( const BinID& bid, bool updcoord )
{
    if ( !trckey_ )
	trckey_ = new TrcKey( bid );
    else
	trckey_->setPosition( bid );
    if ( updcoord )
	setPos( trckey_->getCoord() );
    return *this;
}


Pick::Location& Pick::Location::setSurvID( Pos::SurvID survid, bool updpos )
{
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setSurvID( survid );
    if ( updpos )
	trckey_->setFrom( pos_.getXY() );
    return *this;
}


Pick::Location& Pick::Location::setText( const char* txt )
{
    if ( !text_ )
    {
	if ( txt && *txt )
	    text_ = new BufferString( txt );
    }
    else if ( !txt || !*txt )
	{ delete text_; text_ = 0; }
    else
	*text_ = txt;

    return *this;
}


bool Pick::Location::hasTextKey( const char* key ) const
{ return fndKeyTxt( key, 0 ); }
bool Pick::Location::getKeyedText( const char* key, BufferString& val ) const
{ return fndKeyTxt( key, &val ); }


bool Pick::Location::fndKeyTxt( const char* key, BufferString* val ) const
{
    if ( !text_ || !*text_ )
	{ if ( val ) val->setEmpty(); return false; }

    SeparString sepstr( *text_, '\'' );
    const int strsz = sepstr.size();
    for ( int idx=0; idx<strsz; idx+=2 )
    {
	if ( sepstr[idx] == key )
	    { if ( val ) *val = sepstr[idx+1]; return true; }
    }

    return false;
}


void Pick::Location::setKeyedText( const char* key, const char* txt )
{
    removeTextKey( key );
    if ( !txt || !*txt )
	return;

    if ( !text_ )
	text_ = new BufferString;
    SeparString sepstr( *text_, '\'' );
    sepstr.add( key ).add( txt );
    setText( sepstr );
}


void Pick::Location::removeTextKey( const char* key )
{
    if ( !hasText() )
	return;

    SeparString sepstr( *text_, '\'' );
    for ( int idx=0; idx<sepstr.size(); idx+=2 )
    {
	if ( sepstr[idx] != key )
	    continue;

	SeparString copy( 0, '\'' );
	const int nrkeys = sepstr.size();
	for ( int idy=0; idy<nrkeys; idy++ )
	{
	    if ( idy!=idx && idy!=idx+1 )
		copy.add( sepstr[idy] );
	}

	sepstr = copy;
	idx -= 2;
    }

    setText( sepstr );
}


#define mReadVal(type,readFunc) \
    if ( !*str ) return mUdf(type); \
    char* endptr = str; mSkipNonBlanks( endptr ); \
    if ( *endptr ) *endptr++ = '\0'; \
    type v = readFunc( str ); \
    str = endptr; mSkipBlanks(str); \
    return v

static double getNextVal( char*& str )
{
    mReadVal( double, toDouble );
}
static int getNextInt( char*& str )
{
    mReadVal( int, toInt );
}


bool Pick::Location::fromString( const char* s )
{
    if ( !s || !*s )
	return false;

    // The location may start with the text_
    if ( *s == '"' )
    {
	s++;

	if ( !text_ )
	    text_ = new BufferString( s );
	else
	    *text_ = s;

	char* start = text_->getCStr();
	char* stop = firstOcc( start, '"' );
	if ( !stop )
	    { delete text_; text_ = 0; }
	else
	{
	    *stop = '\0';
	    s += stop - start + 1;
	    text_->replace( newlinechar, pipechar );
	}
    }
    else if ( text_ )
	{ delete text_; text_ = 0; }

    BufferString bufstr( s );
    char* str = bufstr.getCStr();
    mSkipBlanks(str);

    // Then, we always have the actual payload, the coordinate
    Coord3 posread;
    posread.x_ = getNextVal( str );
    posread.y_ = getNextVal( str );
    posread.z_ = getNextVal( str );
    if ( posread.isUdf() )
	return false;

    pos_ = posread;

    // Sometimes, we have a direction
    mSkipBlanks(str);
    const FixedString data( str );
    if ( data.count('\t') > 1 )
    {
	Coord3 dirread;
	dirread.x_ = getNextVal( str );
	dirread.y_ = getNextVal( str );
	dirread.z_ = getNextVal( str );

	if ( !mIsUdf(dirread.y_) )
	{
	    if ( mIsUdf(dirread.z_) )
		dirread.z_ = 0.;
	}

	delete dir_;
	dir_ = new Sphere( dirread );
    }

    // Sometimes, we have a stored GeomID. We always want to set the TrcKey.
    mSkipBlanks(str);
    const Pos::SurvID geomid = getNextInt( str );
    const Survey::Geometry* geom = 0;
    if ( !mIsUdf(geomid) )
	geom = Survey::GM().getGeometry( geomid );
    if ( !geom )
	geom = &Survey::Geometry::default3D();
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setGeomID( geom->getID() );
    trckey_->setFrom( pos_.getXY() );

    return true;
}


void Pick::Location::toString( BufferString& str, bool forexport ) const
{
    str.setEmpty();
    if ( text_ && *text_ )
    {
	BufferString txt( *text_ );
	txt.replace( newlinechar, pipechar );
	str.set( "\"" ).add( txt ).add( "\"\t" );
    }

    Coord3 usepos( pos_ );
    if ( forexport )
    {
	Coord v = usepos.getXY();
	mPIEPAdj(Coord,v,false);
	usepos.x_ = v.x_;
	usepos.y_ = v.y_;
	if ( mPIEP.haveZChg() )
	{
	    float zval = (float)usepos.z_;
	    mPIEPAdj(Z,zval,false);
	    usepos.z_ = zval;
	}

	usepos.z_ = usepos.z_ * SI().showZ2UserFactor();
    }

    str.add( usepos.x_ ).add( od_tab ).add( usepos.y_ );
    str.add( od_tab ).add( usepos.z_ );
    if ( hasDir() )
	str.add( od_tab ).add( dir_->radius_ ).add( od_tab )
	   .add( dir_->theta_ ).add( od_tab ).add( dir_->phi_ );

    if ( trckey_ && !mIsUdf(trckey_->geomID()) )
	str.add( od_tab ).add( trckey_->geomID() );
}
