/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
static const char* sKeyDip = "Dip";


#define mInitPtrs \
      trckey_(TrcKey::udf()) \
    , text_(nullptr)

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
    , dir_(d)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord3& c, const Sphere& d )
    : pos_(c)
    , dir_(d)
    , mInitPtrs
{
}

Pick::Location::Location( const Location& oth )
    : mInitPtrs
{
    *this = oth;
}


Pick::Location::~Location()
{
    delete text_;
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

void Pick::Location::operator=( const Location& oth )
{
    if ( this != &oth )
    {
#	define mCopyMemb( memb, typ ) mSetMember( memb, typ, oth.memb )
	pos_ = oth.pos_;
	dir_ = oth.dir_;
	trckey_ = oth.trckey_;
	mCopyMemb(text_,BufferString)
    }
}


bool Pick::Location::hasTrcKey() const
{
    return !mIsUdf(trckey_.inl()) && !mIsUdf(trckey_.crl())
	&& !trckey_.isSynthetic();
}


bool Pick::Location::hasText() const
{
    return text_ && !text_->isEmpty();
}


const TrcKey& Pick::Location::trcKey() const
{
    if ( hasTrcKey() || !hasPos() )
	return trckey_;

    mDefineStaticLocalObject( TrcKey, rettk, );
    rettk.setPosition( SI().transform(pos_) );
    return rettk;
}


bool Pick::Location::is2D() const
{
    return trcKey().is2D();
}


OD::GeomSystem Pick::Location::survID() const
{
    return geomSystem();
}


OD::GeomSystem Pick::Location::geomSystem() const
{
    return trcKey().geomSystem();
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
    return dir_;
}


const BufferString& Pick::Location::text() const
{
    return text_ ? *text_ : BufferString::empty();
}


Pick::Location& Pick::Location::setTrcKey( const TrcKey& tk )
{ trckey_ = tk; return *this; }
Pick::Location& Pick::Location::setDir( const Sphere& sph )
{ dir_ = sph; return *this; }


Pick::Location& Pick::Location::setLineNr( Pos::LineID lnr )
{
    trckey_.setLineNr( lnr );
    return *this;
}


Pick::Location& Pick::Location::setTrcNr( Pos::TraceID tnr )
{
    trckey_.setTrcNr( tnr );
    return *this;
}


Pick::Location& Pick::Location::setGeomID( Pos::GeomID geomid )
{
    trckey_.setGeomID( geomid );
    return *this;
}


Pick::Location& Pick::Location::setBinID( const BinID& bid, bool updcoord )
{
    trckey_.setPosition( bid );
    if ( updcoord )
	setPos( trckey_.getCoord() );
    return *this;
}


Pick::Location& Pick::Location::setSurvID( OD::GeomSystem gs, bool updpos )
{
    return setGeomSystem( gs, updpos );
}


Pick::Location& Pick::Location::setGeomSystem( OD::GeomSystem gs, bool updpos )
{
    trckey_.setGeomSystem( gs );
    if ( updpos )
	trckey_.setFrom( pos_ );
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

bool Pick::Location::getText( const char* idkey, BufferString& val ) const
{ return getKeyedText(idkey,val); }

bool Pick::Location::fndKeyTxt( const char* key, BufferString* val ) const
{
    if ( !hasText() )
    {
	if ( val )
	    val->setEmpty();

	return false;
    }

    SeparString sepstr( *text_, '\'' );
    const int strsz = sepstr.size();
    for ( int idx=0; idx<strsz; idx+=2 )
    {
	if ( sepstr[idx] == key )
	{
	    if ( val )
		*val = sepstr[idx+1];

	    return true;
	}
    }

    return false;
}


void Pick::Location::setText( const char* key, const char* txt )
{ setKeyedText(key,txt); }
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


void Pick::Location::unSetText( const char* key )
{ removeTextKey(key); }
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
    mSkipBlanks(s);
    if ( !*s )
	return false;

    // The location may start with the label ID (will be introduced in 7.0)
    if ( *s == '@' )
	{ s++; mSkipNonBlanks(s); mSkipBlanks(s); }

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
    posread.x = getNextVal( str );
    posread.y = getNextVal( str );
    posread.z = getNextVal( str );
    if ( posread.isUdf() )
	return false;

    pos_ = posread;

    // Sometimes, we have a direction
    mSkipBlanks(str);
    const StringView data( str );
    if ( data.count( '\t' ) > 1 )
    { // Read the direction too before any trace key information
	Coord3 dirread;
	dirread.x = getNextVal( str );
	dirread.y = getNextVal( str );
	dirread.z = getNextVal( str );

	if ( !mIsUdf(dirread.y) )
	{
	    if ( mIsUdf(dirread.z) )
		dirread.z = 0.;
	}

	dir_ = Sphere( dirread );
    }

    // Sometimes, we have a stored GeomID. We always want to set the TrcKey.
    mSkipBlanks(str);
    const Pos::GeomID geomid( getNextInt(str) );
    const Survey::Geometry* geom = nullptr;
    if ( geomid.isValid() )
	geom = Survey::GM().getGeometry( geomid );
    if ( !geom )
	geom = &Survey::Geometry::default3D();

    trckey_.setGeomID( geom->getID() );
    trckey_.setFrom( pos_ );

    return true;
}


void Pick::Location::toString( BufferString& str, bool forexport,
				   const Coords::CoordSystem* expcrs ) const
{
    str.setEmpty();
    if ( text_ && !text_->isEmpty() )
    {
	BufferString txt( text_ );
	txt.replace( newlinechar, pipechar );
	str.set( "\"" ).add( txt ).add( "\"\t" );
    }

    Coord3 usepos( pos_ );
    if ( forexport )
    {
	mPIEPAdj(Coord,usepos,false);
	if ( mPIEP.haveZChg() )
	{
	    float zval = (float)usepos.z;
	    mPIEPAdj(Z,zval,false);
	    usepos.z = zval;
	}

	if ( expcrs )
	{
	  Coord crd = expcrs->convertFrom(usepos.coord(),
						  *SI().getCoordSystem());
	  usepos.setXY( crd.x, crd.y );
	}

	usepos.z = usepos.z * SI().showZ2UserFactor();
    }

    str.add( usepos.x ).add( od_tab ).add( usepos.y );
    str.add( od_tab ).add( usepos.z );
    if ( hasDir() )
	str.add( od_tab ).add( dir_.radius ).add( od_tab )
	   .add( dir_.theta ).add( od_tab ).add( dir_.phi );

    if ( trckey_.isUdf() || trckey_.position().isUdf() )
	return;

    //actually both calls return the same, but for the clarity
    if ( trckey_.is2D() )
	str.add( od_tab ).add( trckey_.geomID().asInt() );
    else
	str.add( od_tab ).add( trckey_.lineNr() );

    str.add( od_tab ).add( trckey_.trcNr() );
}


void Pick::Location::setDip( float inldip, float crldip )
{
    SeparString dipvaluetext;
    dipvaluetext += ::toString( inldip );
    dipvaluetext += ::toString( crldip );
    setKeyedText( sKeyDip, dipvaluetext.buf() );
}


float Pick::Location::inlDip() const
{
    BufferString dipvaluetext;
    getKeyedText( sKeyDip, dipvaluetext );
    const SeparString dipstr( dipvaluetext );
    return dipstr.getFValue( 0 );
}


float Pick::Location::crlDip() const
{
    BufferString dipvaluetext;
    getKeyedText( sKeyDip, dipvaluetext );
    const SeparString dipstr( dipvaluetext );
    return dipstr.getFValue( 1 );
}
