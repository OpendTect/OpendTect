/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "crsproj.h"
#include "od_istream.h"

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
    , proj_(0)
{
    init();
}


Coords::Projection::~Projection()
{
    pj_free( proj_ );
}


bool Coords::Projection::isOK() const
{
    return proj_;
}


void Coords::Projection::init()
{
    proj_ = pj_init_plus( defstr_.buf() );
}


LatLong Coords::Projection::toGeographicWGS84( const Coord& crd ) const
{
    const Coords::Projection* wgs84proj = getWGS84Proj();
    if ( !isOK() || !wgs84proj )
	return LatLong::udf();

    const Coord llpos = transform( *this, *wgs84proj, crd );
    return LatLong( llpos.x_, llpos.y_ );
}


Coord Coords::Projection::fromGeographicWGS84( const LatLong& ll ) const
{
    const Coords::Projection* wgs84proj = getWGS84Proj();
    if ( !isOK() || !wgs84proj )
	return Coord::udf();

    const Coord llpos( ll.lat_, ll.lng_ );
    return transform( *wgs84proj, *this, llpos );
}


Coord Coords::Projection::transform( const Coords::Projection& from,
				     const Coords::Projection& to, Coord pos )
{
    if ( !from.isOK() || !to.isOK() )
	return Coord::udf();

    Coord ret = pos;
    if ( pj_transform(from.proj_,to.proj_,1,1,&ret.x_,&ret.y_,0) )
	return Coord::udf();

    return ret;
}


bool Coords::Projection::isFeet() const
{ return !isMeter(); }

bool Coords::Projection::isMeter() const
{
    const char* unitstr = defstr_.find( sKeyUnitsArg().buf() );
    if ( !unitstr )
	return true;

    BufferString unitval( unitstr + sKeyUnitsArg().size() );
    return unitval.firstChar() == 'm';
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
	const od_uint16 idnum = mCast( od_uint16, idstr.toInt() );
	const Coords::ProjectionID pid = Coords::ProjectionID::get( idnum );

	// Trim defstr
	char* firstplusptr = linebuf.find( '+' );
	if ( !firstplusptr ) mContinue
	BufferString defstr( firstplusptr );
	char* bracesptr = defstr.findLast( '<' );
	if ( bracesptr )
	    *bracesptr = '\0';

	Coords::Projection* proj = new Coords::Projection( pid, usrnm, defstr );
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
