/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "projects.h"
#include "proj_api.h"

#include "crsproj.h"
#include "od_iostream.h"
#include "bufstringset.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "typeset.h"

static FixedString sKeyUnitsArg()	{ return FixedString("+units="); }
static FixedString sKeyToMeter()	{ return FixedString("+to_meter="); }
static FixedString sKeyEPSG()		{ return FixedString("EPSG"); }

static Coords::AuthorityCode cWGS84AuthCode()
{ return Coords::AuthorityCode(sKeyEPSG(),4326); }

bool Coords::AuthorityCode::operator==( const Coords::AuthorityCode& oth ) const
{ return authority_ == oth.authority_ && id_ == oth.id_; }

Coords::AuthorityCode Coords::AuthorityCode::fromString( const char* str )
{
    FileMultiString fms( str );
    const bool hasauth = fms.size() == 2;
    const BufferString authstr = hasauth ? fms[0] : sKeyEPSG();
    const int idnum = fms.getIValue( hasauth ? 1 : 0 );
    return Coords::AuthorityCode( authstr, idnum );
}


BufferString Coords::AuthorityCode::toString() const
{
    FileMultiString ret( authority_ );
    ret.add( id_ );
    return BufferString( ret.buf() );
}


BufferString Coords::AuthorityCode::toURNString()
{
    BufferString urnstr = "urn:ogc:def:crs:";
    urnstr.add( authority_ ); urnstr.add( "::" );
    urnstr.add( id_ );
    return urnstr;
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
    if ( !proj || !proj->getReady() )
	return LatLong::udf();

    return transformTo( *proj, crd );
}


Coord Coords::Projection::fromGeographic( const LatLong& ll, bool wgs84 ) const
{
    const Coords::Projection* proj = wgs84 ? getWGS84Proj() : this;
    if ( !proj || !proj->getReady() )
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
				 BufferStringSet& usrnms,
				 BufferStringSet& defstrs,
				 bool orthogonalonly )
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
			&& codes[index].id() > proj->authCode().id() )
		    index--;

		index++;
		if ( index >= codes.size()-1 )
		{
		    codes.add( proj->authCode() );
		    usrnms.add( proj->userName() );
		    defstrs.add( proj->defStr() );
		}
		else
		{
		    codes.insert( index, proj->authCode() );
		    usrnms.insertAt( new BufferString(proj->userName()), index);
		    defstrs.insertAt( new BufferString(proj->defStr()), index);
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


static BufferString getArgVal( const char* defstr, const char* argkey )
{
    FixedString argstr( argkey );
    BufferString str( defstr );
    char* ptr = str.find( argkey );
    if ( !ptr )
	return BufferString::empty();

    str = ptr + argstr.size();
    ptr = str.find( ' ' );
    if ( ptr ) *ptr = '\0';

    return str;	
}


BufferString Coords::Projection::getInfoText( const char* defstr )
{
    BufferString info;
    const BufferString projstr = getArgVal( defstr, "+proj=" );
    if ( !projstr.isEmpty() )
    {
	PJ_LIST* pjlist = pj_get_list_ref();
	for ( int idx=0; pjlist[idx].id != 0; idx++ )
	{
	    if ( projstr == pjlist[idx].id )
	    {
		BufferString projnm = pjlist[idx].descr[0];
		char* nl = projnm.find( '\n' );
		if ( nl ) *nl = '\0';
		info.add( "Projection: " ).add( projnm ).addNewLine();
		break;
	    }
	}
    }

    BufferString datumellipsid;
    const BufferString datumstr = getArgVal( defstr, "+datum=" );
    if ( !datumstr.isEmpty() )
    {
	PJ_DATUMS* pjlist = pj_get_datums_ref();
	for ( int idx=0; pjlist[idx].id != 0; idx++ )
	{
	    if ( datumstr == pjlist[idx].id )
	    {
		datumellipsid = pjlist[idx].ellipse_id;
		BufferString datumnm = pjlist[idx].comments;
		if ( datumnm.isEmpty() )
		    datumnm = datumstr;
		info.add( "Datum: " ).add( datumnm ).addNewLine();
		break;
	    }
	}
    }

    BufferString ellipsestr = getArgVal( defstr, "+ellps=" );
    if ( ellipsestr.isEmpty() )
	ellipsestr = datumellipsid;
    if ( !ellipsestr.isEmpty() )
    {
	PJ_ELLPS* pjlist = pj_get_ellps_ref();
	for ( int idx=0; pjlist[idx].id != 0; idx++ )
	{
	    if ( ellipsestr == pjlist[idx].id )
	    {
		info.add( "Ellipse: " ).add( pjlist[idx].name ).addNewLine();
		break;
	    }
	}
    }

    BufferString zonestr = getArgVal( defstr, "+zone=" );
    if ( !zonestr.isEmpty() )
    {
	zonestr += FixedString(defstr).find("+south") ? "S" : "N";
	info.add( "Zone: " ).add( zonestr ).addNewLine();
    }

    const BufferString unitstr = getArgVal( defstr, "+units=" );
    if ( !unitstr.isEmpty() )
    {
	PJ_UNITS* pjlist = pj_get_units_ref();
	for ( int idx=0; pjlist[idx].id != 0; idx++ )
	{
	    if ( unitstr == pjlist[idx].id )
	    {
		info.add( "Unit: " ).add( pjlist[idx].name ).addNewLine();
		break;
	    }
	}
    }

    return info;
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

    bool		isOK() const override;
    bool		isOrthogonal() const override;
    bool		isLatLong() const override;
    bool		isMeter() const override;

    Coord		transformTo(const Projection& target,
				    LatLong) const override;
    LatLong		transformTo(const Projection& target,
				    Coord) const override;

protected:

    bool		getReady() const override;

    bool		init();
    inline projPJ	getLLProj() const
			{ return llproj_ ? llproj_ : proj_; }

    projPJ		proj_;
    projPJ		llproj_;
};
} // Namespace

Coords::Proj4Projection::Proj4Projection( Coords::AuthorityCode code,
					  const char* usrnm, const char* defstr)
    : Projection(code,usrnm,defstr)
    , proj_(0),llproj_(0)
{
}


Coords::Proj4Projection::~Proj4Projection()
{
    pj_free( proj_ );
    pj_free( llproj_ );
}


bool Coords::Proj4Projection::isOK() const
{
    return proj_;
}


bool Coords::Proj4Projection::getReady() const
{
    return isOK() || const_cast<Proj4Projection*>(this)->init();
}


bool Coords::Proj4Projection::init()
{
    proj_ = pj_init_plus( defstr_.buf() );
    if ( proj_ && !isLatLong() )
	llproj_ = pj_latlong_from_proj( proj_ );

    return proj_;
}


bool Coords::Proj4Projection::isLatLong() const
{
    return defstr_.contains( "longlat" );
}


bool Coords::Proj4Projection::isMeter() const
{
    const char* unitstr = defstr_.find( sKeyUnitsArg().buf() );
    if ( unitstr )
    {
	const BufferString unitval( unitstr + sKeyUnitsArg().size() );
	return unitval.firstChar() == 'm';
    }

    const char* tometerstr = defstr_.find( sKeyToMeter().buf() );
    if ( tometerstr )
    {
	const BufferString convval( tometerstr + sKeyToMeter().size() );
	const bool isft = convval.startsWith( "0.304" );
	return !isft;
    }

    return true;
}


Coord Coords::Proj4Projection::transformTo( const Coords::Projection& target,
					    LatLong ll ) const
{
    if ( !getReady() || !target.getReady() )
	return Coord::udf();

    projPJ srcproj4 = getLLProj();
    mDynamicCastGet(const Proj4Projection*,targetproj4,&target)
    if ( !srcproj4 || !targetproj4 || targetproj4->isLatLong() )
	return Coord::udf();

    Coord ret( ll.lng_ * mDeg2RadD, ll.lat_ * mDeg2RadD );
    double dummyz = 0;
    if ( pj_transform(srcproj4,targetproj4->proj_,1,1,&ret.x,&ret.y,&dummyz) ||
      ret.x == HUGE_VAL || ret.y == HUGE_VAL )
	return Coord::udf();

    return ret;
}


LatLong Coords::Proj4Projection::transformTo( const Coords::Projection& target,
					      Coord pos ) const
{
    if ( !getReady() || !target.getReady() )
	return LatLong::udf();

    mDynamicCastGet(const Proj4Projection*,targetproj4,&target)
    projPJ targetproj4ll = targetproj4 ? targetproj4->getLLProj() : 0;
    if ( !targetproj4ll )
	return LatLong::udf();

    double dummyz = 0;
    if ( pj_transform(proj_,targetproj4ll,1,1,&pos.x,&pos.y,&dummyz) ||
			      pos.x == HUGE_VAL || pos.y == HUGE_VAL )
	return LatLong::udf();

    return LatLong( pos.y * mRad2DegD, pos.x * mRad2DegD );
}


bool Coords::Proj4Projection::isOrthogonal() const
{ return !isLatLong();	}



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
	const Coords::ProjectionID pid = idnum;

	// Trim defstr
	char* firstplusptr = linebuf.find( '+' );
	if ( !firstplusptr ) mContinue
	BufferString defstr( firstplusptr );
	char* bracesptr = defstr.findLast( '<' );
	if ( bracesptr )
	    *bracesptr = '\0';

	Coords::Proj4Projection* proj = new Coords::Proj4Projection(
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
    FilePath fp( mGetSetupFileName("CRS") );
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
