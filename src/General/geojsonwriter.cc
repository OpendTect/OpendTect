/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geojsonwriter.h"

#include "geojson.h"
#include "latlong.h"
#include "od_ostream.h"
#include "pickset.h"
#include "survinfo.h"
#include "uistrings.h"

namespace OD
{

namespace JSON
{

void setProperties( const Pick::Set::Disp& disp, GIS::Property& props )
{
    if ( props.type_ == GIS::FeatureType::Undefined )
	return;

    if ( props.isPoint() )
    {
	props.color_ = disp.color_;
	props.pixsize_ = disp.pixsize_;
	props.linestyle_.width_ = 2;
	props.linestyle_.color_ = OD::Color::NoColor();
	props.dofill_ = false;
	props.fillcolor_ = OD::Color::NoColor();
    }
    else
    {
	props.color_ = OD::Color::NoColor();
	props.pixsize_ = 2;
	props.linestyle_ = disp.linestyle_;
	const bool ispoly = props.isPolygon();
	props.dofill_ = ispoly ? disp.dofill_ : false;
	props.fillcolor_ = ispoly ? disp.fillcolor_: OD::Color::NoColor();
    }
}

} // namespace JSON

} // namespace OD

#define mErrRet(s) { errmsg_ = s; return false; }

OD::JSON::GeoJSONWriter::GeoJSONWriter()
    : geojsontree_(new GeoJsonTree())
{}


OD::JSON::GeoJSONWriter::~GeoJSONWriter()
{
    close();
    delete geojsontree_;
}


GIS::Writer& OD::JSON::GeoJSONWriter::setInputCoordSys(
						const Coords::CoordSystem* crs )
{
    geojsontree_->setInputCoordSys( crs );
    return GIS::Writer::setInputCoordSys( crs );
}


GIS::Writer& OD::JSON::GeoJSONWriter::setStream( const char* fnm,
						 bool useexisting )
{
    delete strm_;
    strm_ = new od_ostream( fnm );
    open( fnm, useexisting );
    return *this;
}


bool OD::JSON::GeoJSONWriter::open( const char* fnm, bool useexisting )
{
    errmsg_.setEmpty();

    if ( !fnm || !*fnm )
	mErrRet( tr("No file name provided"))

    if ( !strm_ || !strm_->isOK() )
	mErrRet( uiStrings::phrCannotOpenForWrite( fnm ) )

    if ( !strm().isOK() )
    {
	uiString emsg( tr("Error during write of GeoJSON header info") );
	strm().addErrMsgTo( emsg );
	mErrRet(emsg)
    }

    return true;
}


bool OD::JSON::GeoJSONWriter::close()
{
    if ( !isOK() )
	return false;

    BufferString str;
    geojsontree_->dumpJSon( str, true );
    strm() << str;
    return GIS::Writer::close();
}


bool OD::JSON::GeoJSONWriter::isOK() const
{
    return GIS::Writer::isOK() && geojsontree_;
}


void OD::JSON::GeoJSONWriter::getDefaultProperties( const GIS::FeatureType& typ,
					    GIS::Property& properties ) const
{
    properties.type_ = typ;
    if ( typ == GIS::FeatureType::Undefined )
	return;

    if ( properties.isPoint() )
    {
	properties.color_ = OD::Color( 126, 126, 126 );
	properties.pixsize_ = properties.isMulti() ? 1 : 2;
				// 2=medium; below=small, above=large
	properties.iconnm_ = "circle";
	properties.linestyle_.width_ = 2;
	properties.linestyle_.color_ = OD::Color::NoColor();
	properties.dofill_ = false;
	properties.fillcolor_ = OD::Color::NoColor();
    }
    else
    {
	properties.color_ = OD::Color::NoColor();
	properties.pixsize_ = 2;
	properties.iconnm_.setEmpty();
	properties.linestyle_.width_ = 2;
	properties.linestyle_.color_ = OD::Color( 85, 85, 85 );
	const bool ispoly = properties.isPolygon();
	properties.dofill_ = ispoly;
	properties.fillcolor_ = ispoly ? OD::Color( 85, 85, 85 )
				       : OD::Color::NoColor();
	if ( ispoly )
	    properties.fillcolor_.setTransparencyF( 0.5f );
    }
}


bool OD::JSON::GeoJSONWriter::writePoint( const Coord& coord, const char* nm )
{
    const LatLong ll = LatLong::transform( coord, true, inpcrs_.ptr() );
    return writePoint( ll, nm, mUdf(double) );
}


bool OD::JSON::GeoJSONWriter::writePoint( const Coord3& crd, const char* nm )
{
    const LatLong ll = LatLong::transform( crd.coord(), true, inpcrs_.ptr() );
    return writePoint( ll, nm, crd.z_ );
}


bool OD::JSON::GeoJSONWriter::writePoint( const LatLong& ll, const char* nm,
					  double z )
{
    if ( !geojsontree_ )
	return false;

    GIS::Property properties( properties_ );
    properties.setType( GIS::FeatureType::Point ).setName( nm );
    return geojsontree_->addPoint( ll, z, properties );
}


#define mAddFeatures( typ, obj, nm ) \
    if ( !geojsontree_ ) \
	return false; \
    if ( properties_.isLine() && !doLineCheck(obj.size()) ) \
	return false;\
    if ( properties_.isPolygon() && !doPolygonCheck(obj.size()) ) \
	return false; \
    GIS::Property properties( properties_ ); \
    properties.setType( typ ).setName( nm );


bool OD::JSON::GeoJSONWriter::writeLine( const TypeSet<Coord>& coords,
					 const char* nm )
{
    mAddFeatures( GIS::FeatureType::LineString, coords, nm );
    return geojsontree_->addFeatures( coords, properties );
}


bool OD::JSON::GeoJSONWriter::writeLine( const TypeSet<Coord3>& coords,
					 const char* nm )
{
    mAddFeatures( GIS::FeatureType::LineString, coords, nm );
    return geojsontree_->addFeatures( coords, properties );
}


bool OD::JSON::GeoJSONWriter::writeLine( const Pick::Set& pickset )
{
    mAddFeatures( GIS::FeatureType::LineString, pickset, pickset.name().buf() );
    OD::JSON::setProperties( pickset.disp_, properties );
    return geojsontree_->addFeatures( pickset, properties );
}


bool OD::JSON::GeoJSONWriter::writePolygon( const TypeSet<Coord>& coords,
					    const char* nm )
{
    mAddFeatures( GIS::FeatureType::Polygon, coords, nm );
    return geojsontree_->addFeatures( coords, properties );
}


bool OD::JSON::GeoJSONWriter::writePolygon( const TypeSet<Coord3>& coords,
					    const char* nm )
{
    mAddFeatures( GIS::FeatureType::Polygon, coords, nm );
    return geojsontree_->addFeatures( coords, properties );
}


bool OD::JSON::GeoJSONWriter::writePolygon( const Pick::Set& pickset )
{
    mAddFeatures( GIS::FeatureType::Polygon, pickset, pickset.name().buf() );
    OD::JSON::setProperties( pickset.disp_, properties );
    return geojsontree_->addFeatures( pickset, properties );
}


bool OD::JSON::GeoJSONWriter::writePoints( const TypeSet<Coord>& coords,
					   const char* nm )
{
    mAddFeatures( GIS::FeatureType::MultiPoint, coords, nm );
    return geojsontree_->addFeatures( coords, properties );
}


bool OD::JSON::GeoJSONWriter::writePoints( const TypeSet<Coord3>& coords,
					   const char* nm )
{
    mAddFeatures( GIS::FeatureType::MultiPoint, coords, nm );
    return geojsontree_->addFeatures( coords, properties );
}


bool OD::JSON::GeoJSONWriter::writePoints( const Pick::Set& pickset )
{
    mAddFeatures( GIS::FeatureType::MultiPoint, pickset, pickset.name().buf() );
    OD::JSON::setProperties( pickset.disp_, properties );
    return geojsontree_->addFeatures( pickset, properties );
}


bool OD::JSON::GeoJSONWriter::writeLines( const Pick::Set& pickset )
{
    mAddFeatures( GIS::FeatureType::MultiLineString, pickset,
		  pickset.name().buf() );
    OD::JSON::setProperties( pickset.disp_, properties );
    return geojsontree_->addFeatures( pickset, properties );
}


bool OD::JSON::GeoJSONWriter::writePolygons( const Pick::Set& pickset )
{
    mAddFeatures( GIS::FeatureType::MultiPolygon, pickset,
		  pickset.name().buf() );
    OD::JSON::setProperties( pickset.disp_, properties );
    return geojsontree_->addFeatures( pickset, properties );
}
