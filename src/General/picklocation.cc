/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001
-*/


#include "picklocation.h"
#include "trigonometry.h"
#include "bufstring.h"
#include "survinfo.h"
#include "separstr.h"
#include "posimpexppars.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "trckey.h"
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
    , text_(0) \


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


Pick::Location::Location( const Coord3& c, const Sphere& d )
    : pos_(c)
    , mInitPtrs
{
    if ( !d.isNull() )
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
    rettk = TrcKey( SI().transform(pos_.getXY()) );
    return rettk;
}


bool Pick::Location::is2D() const
{
    return trckey_ && trckey_->is2D();
}


Pos::GeomID Pick::Location::geomID() const
{
    return trckey_ ? trckey_->geomID() : GeomID::get3D();
}


Pick::Location::linenr_type Pick::Location::lineNr() const
{
    return trckey_ ? trckey_->lineNr() : binID().lineNr();
}


Pick::Location::trcnr_type Pick::Location::trcNr() const
{
    return trckey_ ? trckey_->trcNr() : binID().trcNr();
}


BinID Pick::Location::binID() const
{
    if ( trckey_ )
	return trckey_->binID();
    else if ( !hasPos() )
	return BinID::udf();
    return SI().transform( pos_.getXY() );
}


Bin2D Pick::Location::bin2D() const
{
    return Bin2D( Pos::GeomID(lineNr()), trcNr() );
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


Pick::Location& Pick::Location::setLineNr( linenr_type lnr )
{
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setLineNr( lnr );
    return *this;
}


Pick::Location& Pick::Location::setTrcNr( trcnr_type tnr )
{
    if ( !trckey_ )
	trckey_ = new TrcKey;
    trckey_->setTrcNr( tnr );
    return *this;
}


Pick::Location& Pick::Location::setGeomID( GeomID geomid )
{
    if ( trckey_ )
	trckey_->setGeomID( geomid );
    else if ( geomid.is2D() )
	trckey_ = new TrcKey( geomid, 0 );

    return *this;
}


Pick::Location& Pick::Location::setPos( const BinID& bid, bool updcoord )
{
    if ( !trckey_ )
	trckey_ = new TrcKey( bid );
    else
	trckey_->setPos( bid );
    if ( updcoord )
	setPos( trckey_->getCoord() );
    return *this;
}


Pick::Location& Pick::Location::setPos( GeomID gid, trcnr_type tnr,
					bool updcoord )
{
    if ( !trckey_ )
	trckey_ = new TrcKey( gid, tnr );
    else
	trckey_->setPos( gid, tnr );
    if ( updcoord )
	setPos( trckey_->getCoord() );
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


bool Pick::Location::fromString( const char* inp )
{
    if ( !inp || !*inp )
	return false;

    BufferString inpstr( inp );
    char* str = inpstr.getCStr();
    mSkipBlanks(str);

    // The location may start with a Label ID
    if ( *str != '@' )
	grplblid_.setInvalid();
    else
    {
	str++;
	char* idstr = inpstr.getCStr();
	mSkipNonBlanks( str );
	*str++ = '\0';
	grplblid_.setI( idstr );
	mSkipBlanks(str);
    }

    // Then we may have the text_
    if ( *str != '"' )
    {
	if ( text_ )
	    { delete text_; text_ = 0; }
    }
    else
    {
	str++;

	if ( !text_ )
	    text_ = new BufferString( str );
	else
	    text_->set( str );

	char* start = text_->getCStr();
	char* stop = firstOcc( start, '"' );
	if ( !stop )
	    { delete text_; text_ = 0; }
	else
	{
	    *stop = '\0';
	    str += stop - start + 1;
	    text_->replace( newlinechar, pipechar );
	}
    }

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
    const auto storedid = getNextInt( str );
    const SurvGeom* geom = 0;
    if ( mIsUdf(storedid) || storedid < 0 )
	geom = &SurvGeom::get3D();
    else
	geom = &SurvGeom::get2D( GeomID(storedid) );
    if ( !trckey_ )
	trckey_ = new TrcKey;

    trckey_->setGeomID( geom->geomID() ).setFrom( pos_.getXY() );
    return true;
}

void Pick::Location::toString( BufferString& str, bool forexport,
				    const Coords::CoordSystem* expcrs ) const
{
    str.setEmpty();

    if ( !forexport && grplblid_.isValid() )
	str.add( "@" ).add( grplblid_.getI() ).add( od_tab );

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

	if ( expcrs )
	    usepos.setXY( expcrs->convertFrom(usepos.getXY(),
						    *SI().getCoordSystem()) );

	usepos.z_ = usepos.z_ * SI().showZ2UserFactor();
    }

    str.add( usepos.x_ ).add( od_tab ).add( usepos.y_ );
    str.add( od_tab ).add( usepos.z_ );
    if ( hasDir() )
	str.add( od_tab ).add( dir_->radius_ ).add( od_tab )
	   .add( dir_->theta_ ).add( od_tab ).add( dir_->phi_ );

    if ( trckey_ && trckey_->geomID().isValid() )
	str.add( od_tab ).add( trckey_->geomID() );
}


static const char* sKeyGroupLabel = "Group Label";


void Pick::GroupLabel::fillPar( IOPar& iop, int nr ) const
{
    FileMultiString fms;
    fms += id_.getI(); fms += txt_;
    iop.set( IOPar::compKey(sKeyGroupLabel,nr), fms );
}


bool Pick::GroupLabel::usePar( const IOPar& iop, int nr )
{
    const char* res = iop.find( IOPar::compKey(sKeyGroupLabel,nr) );
    if ( !res || !*res )
	return false;

    FileMultiString fms( res );
    id_.setI( fms[0].str() );
    txt_.set( fms[1] );
    return true;
}


void Pick::GroupLabel::removeFromPar( IOPar& iop )
{
    for ( int nr=0; ; nr++ )
    {
	const BufferString ky( IOPar::compKey(sKeyGroupLabel,nr) );
	if ( !iop.hasKey(ky) )
	    break;

	iop.removeWithKey( ky );
    }
}
