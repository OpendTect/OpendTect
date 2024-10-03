/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "giswriter.h"

#include "keystrs.h"
#include "od_ostream.h"
#include "survinfo.h"


mDefineNameSpaceEnumUtils(GIS,FeatureType,"Feature Type")
{
    sKey::Undef(),
    "Point",
    "LineString",
    "Polygon",
    "MultiPoint",
    "MultiLineString",
    "MultiPolygon",
    nullptr
};


// GIS::Property

GIS::Property::Property()
{
}


GIS::Property::Property( const Property& oth )
{
    *this = oth;
}


GIS::Property::~Property()
{
}


GIS::Property& GIS::Property::operator =( const Property& oth )
{
    if ( &oth == this )
	return *this;

    NamedObject::operator =( oth );
    type_	= oth.type_;
    color_	= oth.color_;
    pixsize_	= oth.pixsize_;
    linestyle_	= oth.linestyle_;
    dofill_	= oth.dofill_;
    fillcolor_	= oth.fillcolor_;
    iconnm_	= oth.iconnm_;
    nmkeystr_	= oth.nmkeystr_;

    return *this;
}


GIS::Property& GIS::Property::setType( const FeatureType& typ )
{
    type_ = typ;
    return *this;
}


bool GIS::Property::isPoint() const
{
    return type_ == FeatureType::Point ||
	   type_ == FeatureType::MultiPoint;
}


bool GIS::Property::isLine() const
{
    return type_ == FeatureType::LineString ||
	   type_ == FeatureType::MultiLineString;
}


bool GIS::Property::isPolygon() const
{
    return type_ == FeatureType::Polygon ||
	   type_ == FeatureType::MultiPolygon;
}


bool GIS::Property::isMulti() const
{
    return type_ == FeatureType::MultiPoint ||
	   type_ == FeatureType::MultiLineString ||
	   type_ == FeatureType::MultiPolygon;
}


// GIS::Writer

mImplFactory(GIS::Writer,GIS::Writer::factory);

GIS::Writer::Writer()
    : inpcrs_(SI().getCoordSystem())
    , coordsys_(Coords::CoordSystem::getWGS84LLSystem())
{
}


GIS::Writer::~Writer()
{
    if ( strm_ )
    {
	pErrMsg("Should already have been closed");
	strm_->close();
	delete strm_;
    }
}


bool GIS::Writer::isOK() const
{
    return strm_ && strm_->isOK() && coordsys_;
}


bool GIS::Writer::close()
{
    if ( strm_ )
	strm_->close();

    deleteAndNullPtr( strm_ );
    return true;
}


GIS::Writer& GIS::Writer::setElemName( const char* /* nm */ )
{ return *this; }


GIS::Writer& GIS::Writer::setSurveyName( const char* /* survnm */ )
{ return *this; }


GIS::Writer& GIS::Writer::setDescription( const char* /* desc*/ )
{ return *this; }


GIS::Writer& GIS::Writer::setProperties( const Property& props )
{
    properties_ = props;
    return *this;
}


GIS::Writer& GIS::Writer::setInputCoordSys( const Coords::CoordSystem* crs )
{
    inpcrs_ = crs;
    return *this;
}


const Coords::CoordSystem* GIS::Writer::getOutputCRS() const
{
    return coordsys_.ptr();
}


bool GIS::Writer::doLineCheck( int sz )
{
    if ( sz > 1 )
	return true;

    errmsg_ = tr("The line to write is too small: need at least two points");
    return false;
}


bool GIS::Writer::doPolygonCheck( int sz )
{
    if ( sz > 1 )
	return true;

    errmsg_ = tr("The polygon to write is too small: need at least"
		 " three points");
    return false;
}


uiString GIS::Writer::successMsg()
{
    return tr("Successfully created %1. Do you want to create more?")
						.arg( factoryDisplayName() );
}
