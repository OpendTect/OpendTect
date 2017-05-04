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
#include "typeset.h"

static FixedString sKeyUnitsArg()	{ return FixedString("+units="); }

static const Coords::ProjectionID cWGS84ID()
{ return Coords::ProjectionID::get( 4326 ); }

static const Coords::Projection* getWGS84Proj()
{
    mDefineStaticLocalObject(const Coords::Projection*,proj,
	    		      = Coords::Projection::getByID(cWGS84ID()) );
    return proj;
}

Coords::Projection::Projection( Coords::ProjectionID pid, const char* usrnm,
				const char* defstr )
    : id_(pid),usernm_(usrnm),defstr_(defstr)
{}

Coords::Projection::~Projection()
{}

bool Coords::Projection::isOK() const
{ return false; }

LatLong Coords::Projection::toGeographicWGS84( const Coord& crd ) const
{
    const Coords::Projection* wgs84proj = getWGS84Proj();
    if ( !isOK() || !wgs84proj )
	return LatLong::udf();

    const Coord llpos = transformTo( *wgs84proj, crd );
    return LatLong( mRad2DegD*llpos.y_, mRad2DegD*llpos.x_ );
}


Coord Coords::Projection::fromGeographicWGS84( const LatLong& ll ) const
{
    const Coords::Projection* wgs84proj = getWGS84Proj();
    if ( !isOK() || !wgs84proj )
	return Coord::udf();

    const Coord llpos( mDeg2RadD*ll.lng_, mDeg2RadD*ll.lat_ );
    return wgs84proj->transformTo( *this, llpos );
}

Coord Coords::Projection::transformTo( const Coords::Projection& target,
					Coord pos ) const
{ return Coord::udf(); }

bool Coords::Projection::isOrthogonal() const
{ return true; }

bool Coords::Projection::isFeet() const
{ return !isMeter(); }

bool Coords::Projection::isMeter() const
{ return true; }

void Coords::Projection::getAll( TypeSet<ProjectionID>& pids,
				 BufferStringSet& usrnms, bool orthogonalonly )
{
    for ( int idx=0; idx<Coords::ProjectionRepos::reposSet().size(); idx++ )
    {
	const Coords::ProjectionRepos* repos =
	    		Coords::ProjectionRepos::reposSet().get( idx );
	for ( int idy=0; idy<repos->size(); idy++ )
	{
	    const Coords::Projection* proj = repos->get( idy );
	    if ( !orthogonalonly || proj->isOrthogonal() )
	    {
		pids.add( proj->id() );
		usrnms.add( proj->userName() );
	    }
	}
    }
}


const Coords::Projection* Coords::Projection::getByID(
						Coords::ProjectionID pid )
{
    for ( int idx=0; idx<Coords::ProjectionRepos::reposSet().size(); idx++ )
    {
	const Coords::ProjectionRepos* repos =
	    		Coords::ProjectionRepos::reposSet().get( idx );
	const Coords::Projection* proj = repos->getByID( pid );
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
	    		Coords::ProjectionRepos::reposSet().get( idx );
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
    			Proj4Projection(Coords::ProjectionID,
					const char* usrnm,
					const char* defstr);
			~Proj4Projection();

    bool		isOK() const;
    bool		isOrthogonal() const;
    bool		isMeter() const;

    Coord		transformTo(const Projection& target,Coord pos) const;

protected:

    void		init();

    projPJ		proj_;
};
} // Namespace

Coords::Proj4Projection::Proj4Projection( Coords::ProjectionID pid,
					  const char* usrnm, const char* defstr)
    : Projection(pid,usrnm,defstr)
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

bool Coords::Proj4Projection::isMeter() const
{
    const char* unitstr = defstr_.find( sKeyUnitsArg().buf() );
    if ( !unitstr )
	return true;

    BufferString unitval( unitstr + sKeyUnitsArg().size() );
    return unitval.firstChar() == 'm';
}


Coord Coords::Proj4Projection::transformTo( const Coords::Projection& target,
					    Coord pos ) const
{
    if ( !isOK() || !target.isOK() )
	return Coord::udf();

    mDynamicCastGet(const Proj4Projection*,targetproj4,&target)
    if ( !targetproj4 )
	return Coord::udf();

    Coord ret = pos;
    if ( pj_transform(proj_,targetproj4->proj_,1,1,&ret.x_,&ret.y_,0) )
	return Coord::udf();

    return ret;
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

	Coords::Projection* proj =
	    		new Coords::Proj4Projection( pid, usrnm, defstr );
	add( proj );
    }

    return true;
}


const Coords::Projection* Coords::ProjectionRepos::getByID(
					Coords::ProjectionID pid ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Coords::Projection* proj = get( idx );
	if ( proj && proj->id() == pid )
	    return proj;
    }

    return 0;
}


const Coords::Projection* Coords::ProjectionRepos::getByName(
							const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Coords::Projection* proj = get( idx );
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
	const Coords::ProjectionRepos* repos = reposset_.get( idx );
	if ( repos && repos->key_ == reposkey )
	    return repos;
    }

    return 0;
}


