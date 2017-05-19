/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "projects.h"
#include "proj_api.h"

#include "crsproj.h"
#include "od_istream.h"
#include "bufstringset.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "typeset.h"

static FixedString sKeyUnitsArg()	{ return FixedString("+units="); }
static FixedString sKeyEPSG()		{ return FixedString("EPSG"); }

static Coords::AuthorityCode cWGS84AuthCode()
{ return Coords::AuthorityCode( sKeyEPSG(), Coords::ProjectionID::get(4326) ); }

bool Coords::AuthorityCode::operator==( const Coords::AuthorityCode& oth ) const
{ return authority_ == oth.authority_ && id_ == oth.id_; }

Coords::AuthorityCode Coords::AuthorityCode::fromString( const char* str )
{
    FileMultiString fms( str );
    const bool hasauth = fms.size() == 2;
    const BufferString authstr = hasauth ? fms[0] : sKeyEPSG();
    const int idnum = fms.getIValue( hasauth ? 1 : 0 );
    return Coords::AuthorityCode( authstr, Coords::ProjectionID::get(idnum) );
}


BufferString Coords::AuthorityCode::toString() const
{
    FileMultiString ret( authority_ );
    ret.add( id_.getI() );
    return BufferString( ret.buf() );
}


static const Coords::Projection* getWGS84Proj()
{
    mDefineStaticLocalObject(const Coords::Projection*,proj,
			= Coords::Projection::getByAuthCode(cWGS84AuthCode()) );
    return proj;
}

Coords::Projection::Projection( Coords::AuthorityCode code, const char* usrnm,
				const char* defstr )
    : authcode_(code),usernm_(usrnm),defstr_(defstr)
{}

Coords::Projection::~Projection()
{}

bool Coords::Projection::isOK() const
{ return false; }

LatLong Coords::Projection::toGeographic( const Coord& crd, bool wgs84 ) const
{
    const Coords::Projection* proj = wgs84 ? getWGS84Proj() : this;
    if ( !proj || !proj->isOK() )
	return LatLong::udf();

    return transformTo( *proj, crd );
}


Coord Coords::Projection::fromGeographic( const LatLong& ll, bool wgs84 ) const
{
    const Coords::Projection* proj = wgs84 ? getWGS84Proj() : this;
    if ( !proj || !proj->isOK() )
	return Coord::udf();

    return proj->transformTo( *this, ll );
}

Coord Coords::Projection::transformTo( const Coords::Projection& target,
				       LatLong ll ) const
{ return Coord::udf(); }

LatLong Coords::Projection::transformTo( const Coords::Projection& target,
					 Coord pos ) const
{ return LatLong::udf(); }

bool Coords::Projection::isOrthogonal() const
{ return true; }

bool Coords::Projection::isFeet() const
{ return !isMeter(); }

bool Coords::Projection::isMeter() const
{ return true; }

void Coords::Projection::getAll( TypeSet<Coords::AuthorityCode>& codes,
				 BufferStringSet& usrnms, bool orthogonalonly )
{
    for ( int idx=0; idx<Coords::ProjectionRepos::reposSet().size(); idx++ )
    {
	const Coords::ProjectionRepos* repos =
			Coords::ProjectionRepos::reposSet()[idx];
	for ( int idy=0; idy<repos->size(); idy++ )
	{
	    const Coords::Projection* proj = (*repos)[idy];
	    if ( !orthogonalonly || proj->isOrthogonal() )
	    {
		// sort by AuthorityCode
		int index = codes.size()-1;
		while ( index >= 0 &&
			codes[index].authority() == proj->authCode().authority()
		     && codes[index].id().getI()>proj->authCode().id().getI() )
		    index--;

		index++;
		if ( index >= codes.size()-1 )
		{
		    codes.add( proj->authCode() );
		    usrnms.add( proj->userName() );
		}
		else
		{
		    codes.insert( index, proj->authCode() );
		    usrnms.insertAt( new BufferString(proj->userName()), index);
		}
	    }
	}
    }
}


const Coords::Projection* Coords::Projection::getByAuthCode(
						Coords::AuthorityCode code )
{
    for ( int idx=0; idx<Coords::ProjectionRepos::reposSet().size(); idx++ )
    {
	const Coords::ProjectionRepos* repos =
			Coords::ProjectionRepos::reposSet()[idx];
	const Coords::Projection* proj = repos->getByAuthCode( code );
	if ( proj )
	    return proj;
    }

    return 0;
}


const Coords::Projection* Coords::Projection::getByName( const char* usrnm )
{
    for ( int idx=0; idx<Coords::ProjectionRepos::reposSet().size(); idx++ )
    {
	const Coords::ProjectionRepos* repos =
			Coords::ProjectionRepos::reposSet()[idx];
	const Coords::Projection* proj = repos->getByName( usrnm );
	if ( proj )
	    return proj;
    }

    return 0;
}

namespace Coords
{
class Proj4Projection : public Projection
{
public:
			Proj4Projection(Coords::AuthorityCode,
					const char* usrnm,
					const char* defstr);
			~Proj4Projection();

    bool		isOK() const;
    bool		isOrthogonal() const;
    bool		isLatLong() const;
    bool		isMeter() const;

    Coord		transformTo(const Projection& target,LatLong) const;
    LatLong		transformTo(const Projection& target,Coord) const;

protected:

    void		init();

    projPJ		proj_;
};
} // Namespace

Coords::Proj4Projection::Proj4Projection( Coords::AuthorityCode code,
					  const char* usrnm, const char* defstr)
    : Projection(code,usrnm,defstr)
    , proj_(0)
{
    init();
}


Coords::Proj4Projection::~Proj4Projection()
{
    pj_free( proj_ );
}


bool Coords::Proj4Projection::isOK() const
{
    return proj_;
}


void Coords::Proj4Projection::init()
{
    proj_ = pj_init_plus( defstr_.buf() );
}


bool Coords::Proj4Projection::isLatLong() const
{
    return pj_is_latlong( proj_ );
}

bool Coords::Proj4Projection::isMeter() const
{
    const char* unitstr = defstr_.find( sKeyUnitsArg().buf() );
    if ( !unitstr )
	return true;

    BufferString unitval( unitstr + sKeyUnitsArg().size() );
    return unitval.firstChar() == 'm';
}


Coord Coords::Proj4Projection::transformTo( const Coords::Projection& target,
					    LatLong ll ) const
{
    if ( !isOK() || !target.isOK() )
	return Coord::udf();

    projPJ srcproj4 = isLatLong() ? proj_ : pj_latlong_from_proj( proj_ );
    mDynamicCastGet(const Proj4Projection*,targetproj4,&target)
    if ( !srcproj4 || !targetproj4 || targetproj4->isLatLong() )
	return Coord::udf();

    Coord ret( ll.lng_ * mDeg2RadD, ll.lat_ * mDeg2RadD );
    if ( pj_transform(srcproj4,targetproj4->proj_,1,1,&ret.x_,&ret.y_,NULL) )
	return Coord::udf();

    return ret;
}


LatLong Coords::Proj4Projection::transformTo( const Coords::Projection& target,
					      Coord pos ) const
{
    if ( !isOK() || !target.isOK() )
	return LatLong::udf();

    mDynamicCastGet(const Proj4Projection*,targetproj4,&target)
    projPJ targetproj4ll = targetproj4 && !targetproj4->isLatLong()
			 ? pj_latlong_from_proj( targetproj4->proj_ )
			 : targetproj4->isLatLong() ? targetproj4->proj_ : 0;
    if ( !targetproj4ll )
	return LatLong::udf();

    if ( pj_transform(proj_,targetproj4ll,1,1,&pos.x_,&pos.y_,NULL) )
	return LatLong::udf();

    return LatLong( pos.y_ * mRad2DegD, pos.x_ * mRad2DegD );
}


bool Coords::Proj4Projection::isOrthogonal() const
{ return isOK() && !pj_is_latlong( proj_ ) && !pj_is_geocent( proj_ ); }



ObjectSet<Coords::ProjectionRepos> Coords::ProjectionRepos::reposset_;

Coords::ProjectionRepos::ProjectionRepos( const char* reposkey, uiString desc )
    : key_(reposkey), desc_(desc)
{}

bool Coords::ProjectionRepos::readFromFile( const char* fnm )
{
    od_istream infile( fnm );
    BufferString linebuf, prevlinebuf;

#define mContinue { prevlinebuf = linebuf; continue; }
    while ( infile.isOK() && infile.getLine(linebuf) )
    {
	if ( linebuf.size() < 3 || linebuf.firstChar() == '#' ) // Not a defstr
	    mContinue

	// linebuf is a defstr. So, prevlinebuf should be the usernm.
	if ( prevlinebuf.size() < 3 || prevlinebuf.firstChar() != '#' )
	    break;

	const BufferString usrnm( prevlinebuf.getCStr() + 2 );

	// Get ID
	BufferString idstr = linebuf;
	char* firstspace = idstr.find( ' ' );
	if ( !firstspace ) mContinue
	*firstspace = '\0';
	idstr.unEmbed( '<', '>' );
	const int idnum = idstr.toInt();
	const Coords::ProjectionID pid = Coords::ProjectionID::get( idnum );

	// Trim defstr
	char* firstplusptr = linebuf.find( '+' );
	if ( !firstplusptr ) mContinue
	BufferString defstr( firstplusptr );
	char* bracesptr = defstr.findLast( '<' );
	if ( bracesptr )
	    *bracesptr = '\0';

	Coords::Projection* proj = new Coords::Proj4Projection(
		Coords::AuthorityCode(key_,pid), usrnm, defstr );
	add( proj );
    }

    return true;
}


const Coords::Projection* Coords::ProjectionRepos::getByAuthCode(
					Coords::AuthorityCode code ) const
{
    if ( code.authority() != key_ )
	return 0;

    for ( int idx=0; idx<size(); idx++ )
    {
	const Coords::Projection* proj = (*this)[idx];
	if ( proj && proj->authCode() == code )
	    return proj;
    }

    return 0;
}


const Coords::Projection* Coords::ProjectionRepos::getByName(
							const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Coords::Projection* proj = (*this)[idx];
	if ( proj && proj->userName() == nm )
	    return proj;
    }

    return 0;
}


void Coords::ProjectionRepos::addRepos( Coords::ProjectionRepos* newrepos )
{
    reposset_ += newrepos;
}


const Coords::ProjectionRepos* Coords::ProjectionRepos::getRepos(
							const char* reposkey )
{
    for ( int idx=0; idx<reposset_.size(); idx++ )
    {
	const Coords::ProjectionRepos* repos = reposset_[idx];
	if ( repos && repos->key_ == reposkey )
	    return repos;
    }

    return 0;
}


void Coords::ProjectionRepos::getAuthKeys( BufferStringSet& keys )
{
    for ( int idx=0; idx<reposset_.size(); idx++ )
    {
	const Coords::ProjectionRepos* repos = reposset_[idx];
	keys.add( repos->key_ );
    }
}


void Coords::ProjectionRepos::initStdRepos()
{
    File::Path fp( mGetSetupFileName("CRS") );
    Coords::ProjectionRepos* repos = new Coords::ProjectionRepos( "EPSG",
	    			toUiString("Standard EPSG Projectons") );
    fp.add( "epsg" );
    repos->readFromFile( fp.fullPath() );
    Coords::ProjectionRepos::addRepos( repos );

    repos = new Coords::ProjectionRepos( "ESRI", toUiString("ESRI Projectons"));
    fp.setFileName( "esri" );
    repos->readFromFile( fp.fullPath() );
    Coords::ProjectionRepos::addRepos( repos );
}
