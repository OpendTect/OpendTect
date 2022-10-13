/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "crsproj.h"
#include "od_iostream.h"
#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "odjson.h"
#include "separstr.h"
#include "typeset.h"
#include "unitofmeasure.h"

static StringView sKeyEPSG()		{ return StringView("EPSG"); }

Coords::AuthorityCode Coords::AuthorityCode::sWGS84AuthCode()
{ return Coords::AuthorityCode(sKeyEPSG(),4326); }

bool Coords::AuthorityCode::operator==( const Coords::AuthorityCode& oth ) const
{ return authority_ == oth.authority_ && code_ == oth.code_; }

Coords::AuthorityCode Coords::AuthorityCode::fromString( const char* str )
{
    FileMultiString fms( str );
    const bool hasauth = fms.size() == 2;
    const BufferString authstr = hasauth ? fms[0] : sKeyEPSG();
    return Coords::AuthorityCode( authstr, fms[hasauth ? 1 : 0].buf() );
}


BufferString Coords::AuthorityCode::toString() const
{
    FileMultiString ret( authority_ );
    ret.add( code_ );
    return BufferString( ret.buf() );
}


BufferString Coords::AuthorityCode::toURNString()
{
    BufferString urnstr = "urn:ogc:def:crs:";
    urnstr.add( authority_ ); urnstr.add( "::" );
    urnstr.add( code_ );
    return urnstr;
}


static const Coords::Projection* getWGS84Proj()
{
    mDefineStaticLocalObject(const Coords::Projection*,proj,
				= Coords::Projection::getByAuthCode(
				    Coords::AuthorityCode::sWGS84AuthCode()) );
    return proj;
}

Coords::Projection::Projection( Coords::AuthorityCode code )
    : authcode_(code)
{}

Coords::Projection::~Projection()
{}

bool Coords::Projection::isOK() const
{ return false; }

LatLong Coords::Projection::toGeographic( const Coord& crd, bool wgs84 ) const
{
    const Coords::Projection* proj = wgs84 ? getWGS84Proj() : this;
    if ( !proj || !proj->isOK() || crd.isUdf() )
	return LatLong::udf();

    return transformTo( *proj, crd );
}


Coord Coords::Projection::fromGeographic( const LatLong& ll, bool wgs84 ) const
{
    const Coords::Projection* proj = wgs84 ? getWGS84Proj() : this;
    if ( !proj || !proj->isOK() || ll.isUdf() )
	return Coord::udf();

    return proj->transformTo( *this, ll );
}

Coord Coords::Projection::transformTo( const Coords::Projection& target,
				       LatLong ll ) const
{
    return Coord::udf();
}

LatLong Coords::Projection::transformTo( const Coords::Projection& target,
					 Coord pos ) const
{
    return LatLong::udf();
}


bool Coords::Projection::isOrthogonal() const
{
    return true;
}

bool Coords::Projection::isFeet() const
{
    return !isMeter();
}

bool Coords::Projection::isMeter() const
{
    return true;
}

Coords::AuthorityCode Coords::Projection::getGeodeticAuthCode() const
{
    return Coords::AuthorityCode::sWGS84AuthCode();
}

BufferString Coords::Projection::getProjDispString() const
{
    return BufferString::empty();
}

BufferString Coords::Projection::getGeodeticProjDispString() const
{
    return BufferString::empty();
}

BufferString Coords::Projection::sWGS84ProjDispString()
{
    const Coords::Projection* wgs84proj = getWGS84Proj();
    return wgs84proj ? wgs84proj->getProjDispString() :
			Coords::AuthorityCode::sWGS84AuthCode().toString();
}

#ifndef OD_NO_PROJ

#include "proj.h"

namespace Coords
{
class ProjProjection : public Projection
{
public:
			ProjProjection(Coords::AuthorityCode);
			~ProjProjection();

    const char*		userName() const override;
    bool		isOK() const override;
    bool		isOrthogonal() const override;
    bool		isLatLong() const override;
    bool		isMeter() const override;
    bool		isFeet() const override;

    Coords::AuthorityCode	getGeodeticAuthCode() const override;

    BufferString	getProjDispString() const override;
    BufferString	getGeodeticProjDispString() const override;

    Coord		transformTo(const Projection&,LatLong) const override;
    LatLong		transformTo(const Projection&,Coord) const override;

    inline PJ*		getProj() const		{ return proj_; }
    inline PJ*		getLLProj() const
			{ return llproj_ ? llproj_ : proj_; }

protected:

    bool		init();

    PJ*			proj_		= nullptr;
    PJ*			llproj_		= nullptr;

    void		calcData();
};

} // namespace Coords


static Coord convertCoordFromPJToPJ( const Coord& pos, PJ* from, PJ* to )
{
    PJ* pjtr = proj_create_crs_to_crs_from_pj( PJ_DEFAULT_CTX, from, to,
						nullptr, nullptr );
    if ( !pjtr )
	return Coord::udf();

    PJ* pjtrnorm = proj_normalize_for_visualization( PJ_DEFAULT_CTX, pjtr );
    proj_destroy( pjtr );
    if ( !pjtrnorm )
	return Coord::udf();

    const PJ_COORD inpcrd = proj_coord( pos.x, pos.y, 0, 0 );
    PJ_COORD retcoord = proj_trans( pjtrnorm, PJ_FWD, inpcrd );

    proj_destroy( pjtrnorm );
    if ( retcoord.v[0] == HUGE_VAL || retcoord.v[1] == HUGE_VAL )
	return Coord::udf();

    return Coord( retcoord.v[0], retcoord.v[1] );
}


Coords::ProjProjection::ProjProjection( Coords::AuthorityCode code )
    : Projection(code)
{
    init();
}


Coords::ProjProjection::~ProjProjection()
{
    proj_destroy( proj_ );
    proj_destroy( llproj_ );
}


const char* Coords::ProjProjection::userName() const
{
    return proj_ ? proj_get_name( proj_ ) : "Unknown";
}


bool Coords::ProjProjection::isOK() const
{
    return proj_;
}


bool Coords::ProjProjection::init()
{
    proj_ = proj_create( PJ_DEFAULT_CTX, authCode().toURNString().buf() );
    if ( proj_ && !isLatLong() )
    {
	llproj_ = proj_crs_get_geodetic_crs( PJ_DEFAULT_CTX, proj_ );
	calcData();
    }

    return proj_;
}


Coords::AuthorityCode Coords::ProjProjection::getGeodeticAuthCode() const
{
    if ( !llproj_ && (!proj_ || !isLatLong()) )
	return Coords::AuthorityCode::sWGS84AuthCode();

    PJ* llproj = getLLProj();
    return Coords::AuthorityCode( proj_get_id_auth_name(llproj,0),
				  proj_get_id_code(llproj,0) );
}


static BufferString makeProjDispString( PJ* pj )
{
    if ( !pj )
	return BufferString::empty();

    BufferString ret( "[" );
    ret.add( proj_get_id_auth_name(pj,0) ).add( ":" )
	.add( proj_get_id_code(pj,0) ).add( "] " )
	.add( proj_get_name(pj) );
    return ret;
}


BufferString Coords::ProjProjection::getProjDispString() const
{
    return makeProjDispString( proj_ );
}


BufferString Coords::ProjProjection::getGeodeticProjDispString() const
{
    if ( !llproj_ && !isLatLong() )
	return BufferString::empty();

    return makeProjDispString( getLLProj() );
}


bool Coords::ProjProjection::isLatLong() const
{
    if ( !proj_ )
	return false;

    PJ_TYPE pjtype = proj_get_type( proj_ );
    return pjtype == PJ_TYPE_GEOGRAPHIC_CRS ||
	   pjtype == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
	   pjtype == PJ_TYPE_GEOGRAPHIC_3D_CRS;
}


bool Coords::ProjProjection::isMeter() const
{
    return uom_ && uom_->scaler().isEmpty();
}


bool Coords::ProjProjection::isFeet() const
{
    return uom_ && uom_->name() == "Feet";
}


void Coords::ProjProjection::calcData()
{
    BufferString str( proj_as_projjson(PJ_DEFAULT_CTX, proj_, nullptr) );
    OD::JSON::Object obj( nullptr );
    const uiRetVal retval = obj.parseJSon( str.getCStr(), str.size() );
    if ( retval.isError() )
	return;

    const OD::JSON::Object* crsobj = obj.getObject( "coordinate_system" );
    if ( !crsobj )
	return;

    const OD::JSON::Array* crsarr = crsobj->getArray( "axis" );
    if ( !crsarr || crsarr->isEmpty() )
	return;

    const OD::JSON::Object axisobj = crsarr->object( 0 );
    OD::JSON::ValueSet::ValueType type = axisobj.valueType(
						    axisobj.indexOf("unit") );
    BufferString unitstr;
    if ( type == OD::JSON::ValueSet::Data )
	unitstr = axisobj.getStringValue( "unit" );
    else
    {
	const OD::JSON::Object* unitobj = axisobj.getObject( "unit" );
	if ( unitobj )
	    unitstr = unitobj->getStringValue( "name" );
    }

    uom_ = UnitOfMeasure::getGuessed( unitstr );
    if ( !uom_ )
    {
	if ( unitstr.matches("*met*",OD::CaseInsensitive) )
	    uom_ = UnitOfMeasure::getGuessed( "Meter" );
	else if ( unitstr.matches("*foot*",OD::CaseInsensitive) )
	    uom_ = UnitOfMeasure::getGuessed( "Feet" );
	else if ( unitstr.matches("*yard*",OD::CaseInsensitive) )
	    uom_ = UnitOfMeasure::getGuessed( "Yard" );
    }

    if ( uom_ )
	convfac_ = uom_->scaler().factor;
    else if ( !uom_ && type == OD::JSON::ValueSet::SubObject )
    {
	const OD::JSON::Object* unitobj = axisobj.getObject( "unit" );
	if ( unitobj )
	    convfac_ = unitobj->getDoubleValue( "conversion_factor" );
    }
}


Coord Coords::ProjProjection::transformTo( const Coords::Projection& target,
					    LatLong ll ) const
{
    if ( !isOK() || !target.isOK() )
	return Coord::udf();

    PJ* srcpj = getLLProj();
    mDynamicCastGet(const ProjProjection*,targetproj,&target)
    PJ* targetpj = targetproj ? targetproj->proj_ : nullptr;
    if ( !srcpj || !targetpj || targetproj->isLatLong() )
	return Coord::udf();

    const Coord pos( ll.lng_, ll.lat_ );
    return convertCoordFromPJToPJ( pos, srcpj, targetpj );
}


LatLong Coords::ProjProjection::transformTo( const Coords::Projection& target,
					      Coord pos ) const
{
    if ( !isOK() || !target.isOK() )
	return LatLong::udf();

    mDynamicCastGet(const ProjProjection*,targetproj,&target)
    PJ* targetpj = targetproj ? targetproj->getLLProj() : nullptr;
    if ( !targetpj )
	return LatLong::udf();

    const Coord llpos = convertCoordFromPJToPJ( pos, proj_, targetpj );
    return LatLong( llpos.y, llpos.x );
}


bool Coords::ProjProjection::isOrthogonal() const
{ return !isLatLong();	}


Coord Coords::Projection::convert( const Coord& pos,
				   const Coords::Projection& from,
				   const Coords::Projection& to )
{
    if ( pos.isUdf() )
	return Coord::udf();

    mDynamicCastGet(const ProjProjection*,fromproj,&from)
    mDynamicCastGet(const ProjProjection*,toproj,&to)
    if ( !fromproj || !toproj )
	return Coord::udf();

    PJ* frompj = fromproj->getProj();
    PJ* topj = toproj->getProj();
    if ( !frompj || !topj )
	return Coord::udf();

    return convertCoordFromPJToPJ( pos, frompj, topj );
}


Coords::Projection* Coords::Projection::getByAuthCode(
						Coords::AuthorityCode code )
{
    Coords::Projection* newproj = new Coords::ProjProjection( code );
    if ( !newproj->isOK() )
    {
	delete newproj;
	return nullptr;
    }

    return newproj;
}


class ProjCRSInfoList : public Coords::CRSInfoList
{
public:
ProjCRSInfoList( bool orthogonal )
{
    PJ_TYPE orthogonalprojtypes[] = { PJ_TYPE_PROJECTED_CRS };
    PJ_TYPE geodeticprojtypes[] = { PJ_TYPE_GEOGRAPHIC_CRS };
    PROJ_CRS_LIST_PARAMETERS* params = proj_get_crs_list_parameters_create();
    if ( params )
    {
	params->types = orthogonal ? orthogonalprojtypes : geodeticprojtypes;
	params->typesCount = 1;
    }

    infos_ = proj_get_crs_info_list_from_database( nullptr, nullptr, params,
						   &sz_ );
    proj_get_crs_list_parameters_destroy( params );
}

~ProjCRSInfoList()
{
    proj_crs_info_list_destroy( infos_ );
}

int size() const
{
    return sz_;
}

const char* authCode( int index ) const
{
    const PROJ_CRS_INFO* crsinfo = getInfo( index );
    return crsinfo ? crsinfo->code : nullptr;
}

const char* authName( int index ) const
{
    const PROJ_CRS_INFO* crsinfo = getInfo( index );
    return crsinfo ? crsinfo->auth_name : nullptr;
}

const char* name( int index ) const
{
    const PROJ_CRS_INFO* crsinfo = getInfo( index );
    return crsinfo ? crsinfo->name : nullptr;
}

const char* areaName( int index ) const
{
    const PROJ_CRS_INFO* crsinfo = getInfo( index );
    return crsinfo ? crsinfo->area_name : nullptr;
}

const char* projMethod( int index ) const
{
    const PROJ_CRS_INFO* crsinfo = getInfo( index );
    return crsinfo ? crsinfo->projection_method_name : nullptr;
}

int indexOf( const Coords::AuthorityCode& authcode ) const
{
    for ( int idx=0; idx<sz_; idx++ )
    {
	if ( infos_[idx] &&
		StringView(authcode.authority()) == infos_[idx]->auth_name &&
		StringView(authcode.code()) == infos_[idx]->code )
	    return idx;
    }

    return -1;
}

private:

const PROJ_CRS_INFO* getInfo( int index ) const
{
    if ( index < 0 || index >= sz_ )
	return nullptr;

    return infos_[index];
}

    PROJ_CRS_INFO**	infos_	= nullptr;
    int			sz_	= 0;
};


void Coords::initCRSDatabase()
{
    FilePath fp( mGetSetupFileName("CRS"), "proj.db" );
    if ( File::exists(fp.fullPath()) )
	proj_context_set_database_path( PJ_DEFAULT_CTX, fp.fullPath(),
					nullptr, nullptr );
}


Coords::CRSInfoList* Coords::getCRSInfoList( bool orthogonal )
{
    return new ProjCRSInfoList( orthogonal );
}

#else

Coord Coords::Projection::convert( const Coord& pos,
				   const Coords::Projection& from,
				   const Coords::Projection& to )
{
    return Coord::udf();
}


Coords::Projection* Coords::Projection::getByAuthCode(
						Coords::AuthorityCode code )
{
    return nullptr;
}


void Coords::initCRSDatabase()
{
}


Coords::CRSInfoList* Coords::getCRSInfoList( bool orthogonal )
{
    return nullptr;
}

#endif // OD_NO_PROJ


// CRSInfoList

uiString Coords::CRSInfoList::getDispString( int idx ) const
{
    return toUiString( "[%1:%2] %3" ).arg( authName(idx) )
				     .arg( authCode(idx) )
				     .arg( name(idx) );
}


uiString Coords::CRSInfoList::getDescString( int idx ) const
{
    BufferString areastr( areaName(idx) );
    BufferString areapretty;
    int cidx = 1;
    while ( cidx<areastr.size()-1 )
    {
	const char c = areastr[cidx++];
	if ( c == ';' || c == '.' || c == ',' )
	{
	    BufferString segment( areastr );
	    segment[cidx] = '\0';
	    segment.trimBlanks();
	    areapretty.addTab().add( segment ).addNewLine();
	    areastr = BufferString( areastr.getCStr() + cidx );
	    cidx = 1;
	}
    }

    areastr.trimBlanks();
    if ( !areastr.isEmpty() )
	areapretty.addTab().add( areastr ).addNewLine();

    return toUiString( "Authority:\t%1\nCode:\t%2\nName:\t%3\n"
			"Projection method:\t%4\nArea of use:\t%5\n" )
			.arg( authName(idx) ).arg( authCode(idx) )
			.arg( name(idx) ).arg( projMethod(idx) )
			.arg( areapretty );
}
