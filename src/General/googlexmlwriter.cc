/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "googlexmlwriter.h"

#include "latlong.h"
#include "od_ostream.h"
#include "pickset.h"
#include "survinfo.h"
#include "uistrings.h"

#define mErrRet(s) { errmsg_ = s; return false; }

namespace ODGoogle
{

static void setProperties( const Pick::Set::Disp& disp,
			   const GIS::FeatureType& typ,
			   GIS::Property& props )
{
    props.type_ = typ;
    if ( typ == GIS::FeatureType::Undefined )
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

} // namespace ODGoogle

ODGoogle::KMLWriter::KMLWriter()
{
}


ODGoogle::KMLWriter::~KMLWriter()
{
    close();
}


GIS::Writer& ODGoogle::KMLWriter::setSurveyName( const char* survnm )
{
    survnm_ = survnm;
    return GIS::Writer::setSurveyName( survnm );
}


GIS::Writer& ODGoogle::KMLWriter::setElemName( const char* nm )
{
    elemnm_ = nm;
    return GIS::Writer::setElemName( nm );
}


GIS::Writer& ODGoogle::KMLWriter::setStream( const char* fnm, bool useexisting )
{
    delete strm_;
    strm_ = new od_ostream( fnm );
    open( fnm, useexisting );
    return *this;
}


GIS::Writer& ODGoogle::KMLWriter::setDescription( const char* desc )
{
    desc_ = desc;
    return GIS::Writer::setDescription( desc );
}


bool ODGoogle::KMLWriter::open( const char* fnm, bool useexisting )
{
    errmsg_.setEmpty();

    if ( !fnm || !*fnm )
	mErrRet( tr("No file name provided"))

    if ( !strm_ || !strm_->isOK() )
	mErrRet( uiStrings::phrCannotOpenForWrite( fnm ) )

    strm() << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<kml xmlns=\"http://www.opengis.net/kml/2.2\" "
	    "xmlns:gx=\"http://www.google.com/kml/ext/2.2\" "
	    "xmlns:kml=\"http://www.opengis.net/kml/2.2\" "
	    "xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
	    "<Document>\n";

    BufferString treenm = survnm_.isEmpty() ? SI().name() : survnm_;
    if ( !elemnm_.isEmpty() )
	treenm.add( ": " ).add( elemnm_.str() );

    strm() << "<name>" << treenm << "</name>";
    if ( !strm().isOK() )
    {
	uiString emsg( tr("Error during write of XML header info") );
	strm().addErrMsgTo( emsg );
	mErrRet(emsg)
    }

    stlidx_ = 0;

    return true;
}


bool ODGoogle::KMLWriter::close()
{
    if ( !isOK() )
	return false;

    strm() << "\n</Document>\n</kml>" << od_endl;
    stlidx_ = -1;
    return GIS::Writer::close();
}


void ODGoogle::KMLWriter::getDefaultProperties( const GIS::FeatureType& typ,
					    GIS::Property& properties ) const
{
    properties.type_ = typ;
    if ( typ == GIS::FeatureType::Undefined )
	return;

    if ( properties.isPoint() )
    {
	properties.color_ = OD::Color::Green();
	properties.pixsize_ = properties.isMulti() ? 1 : 2;
	properties.linestyle_.width_ = 2;
	properties.linestyle_.color_ = OD::Color::NoColor();
	properties.dofill_ = false;
	properties.fillcolor_ = OD::Color::NoColor();
    }
    else
    {
	properties.color_ = OD::Color::NoColor();
	properties.pixsize_ = 2;
	properties.linestyle_.width_ = 2;
	properties.linestyle_.color_ = OD::Color::Green();
	const bool ispoly = properties.isPolygon();
	properties.dofill_ = ispoly;
	properties.fillcolor_ = ispoly ? OD::Color::Red()
				       : OD::Color::NoColor();
    }
}


bool ODGoogle::KMLWriter::writePoint( const Coord& coord, const char* nm )
{
    properties_.type_ = GIS::FeatureType::Point;
    const bool res = putIconStyles() && putPlaceMark( coord, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writePoint( const Coord3& coord, const char* nm )
{
    properties_.type_ = GIS::FeatureType::Point;
    const bool res = putIconStyles() && putPlaceMark( coord, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writePoint( const LatLong& ll, const char* nm,
				      double z )
{
    properties_.type_ = GIS::FeatureType::Point;
    const bool res = putIconStyles() && putPlaceMark( ll, z, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writeLine( const TypeSet<Coord>& coords,
				     const char* nm )
{
    properties_.type_ = GIS::FeatureType::LineString;
    const bool res = putPolyStyle() && putLine( coords, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writeLine( const TypeSet<Coord3>& coords,
				     const char* nm )
{
    properties_.type_ = GIS::FeatureType::LineString;
    const bool res = putPolyStyle() && putLine( coords, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writeLine( const Pick::Set& pickset )
{
    TypeSet<Coord3> coords;
    pickset.getLocations( coords );
    ODGoogle::setProperties( pickset.disp_, GIS::FeatureType::LineString,
			     properties_ );
    const bool res = putPolyStyle() && putLine( coords, pickset.name() );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writePolygon( const TypeSet<Coord>& coords,
					const char* nm )
{
    properties_.type_ = GIS::FeatureType::Polygon;
    const bool res = putPolyStyle() && putPoly( coords, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writePolygon( const TypeSet<Coord3>& coords,
					const char* nm )
{
    properties_.type_ = GIS::FeatureType::Polygon;
    const bool res = putPolyStyle() && putPoly( coords, nm );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writePolygon( const Pick::Set& pickset )
{
    TypeSet<Coord3> coords;
    pickset.getLocations( coords );
    ODGoogle::setProperties( pickset.disp_, GIS::FeatureType::Polygon,
			     properties_ );
    const bool res = putPolyStyle() && putPoly( coords, pickset.name() );
    stlidx_++;
    return res;
}


bool ODGoogle::KMLWriter::writePoints( const TypeSet<Coord>& coords,
				       const char* nm )
{
    if ( !putFolder(nm) )
	return false;

    properties_.type_ = GIS::FeatureType::MultiPoint;
    if	( !putIconStyles() )
	return false;

    for ( const auto& crd : coords )
	putPlaceMark( crd, nullptr );

    return closeFolder();
}


bool ODGoogle::KMLWriter::writePoints( const TypeSet<Coord3>& coords,
				       const char* nm )
{
    if ( !putFolder(nm) )
	return false;

    properties_.type_ = GIS::FeatureType::MultiPoint;
    if	( !putIconStyles() )
	return false;

    for ( const auto& crd : coords )
	putPlaceMark( crd, nullptr );

    return closeFolder();
}


bool ODGoogle::KMLWriter::writePoints( const Pick::Set& pickset )
{
    TypeSet<Coord3> coords;
    pickset.getLocations( coords );
    ODGoogle::setProperties( pickset.disp_, GIS::FeatureType::MultiPoint,
			     properties_ );
    return writePoints( coords, pickset.name() );
}


bool ODGoogle::KMLWriter::writeLines( const Pick::Set& pickset )
{
    if ( pickset.nrSets() < 2 )
	{ pErrMsg("Incorrect usage: use GIS::Writer::writeLine"); }

    if ( !putFolder(pickset.name().buf()) )
	return false;

    ODGoogle::setProperties( pickset.disp_, GIS::FeatureType::MultiLineString,
			     properties_ );
    if	( !putPolyStyle() )
	return false;

    for ( int iset=0; iset<pickset.nrSets(); iset++ )
    {
	TypeSet<Coord3> coords;
	pickset.getLocations( coords, iset );
	putLine( coords, nullptr );
    }

    return closeFolder();
}


bool ODGoogle::KMLWriter::writePolygons( const Pick::Set& pickset )
{
    if ( pickset.nrSets() < 2 )
	{ pErrMsg("Incorrect usage: use GIS::Writer::writePolygon"); }

    if ( !putFolder(pickset.name().buf()) )
	return false;

    ODGoogle::setProperties( pickset.disp_, GIS::FeatureType::MultiPolygon,
			     properties_ );
    if	( !putPolyStyle() )
	return false;

    for ( int iset=0; iset<pickset.nrSets(); iset++ )
    {
	TypeSet<Coord3> coords;
	pickset.getLocations( coords, iset );
	putPoly( coords, nullptr );
    }

    return closeFolder();
}


bool ODGoogle::KMLWriter::putFolder( const char* nm )
{
    if ( !isOK() )
	return false;

    strm() << "\n\t<Folder>\n" "\t\t<name>" << nm << "</name>";
    if ( !desc_.isEmpty() )
	strm() << "\n\t\t<description>" << desc_ << "</description>";

    folderopen_ = true;
    return true;
}


bool ODGoogle::KMLWriter::closeFolder()
{
    if ( !isOK() )
	return false;

    strm() << "\n\t</Folder>";
    folderopen_ = false;
    stlidx_++;
    return true;
}


#define mDeclIconStNm \
    const bool haveiconnm = !properties_.iconnm_.isEmpty() && \
				     !properties_.iconnm_.isEqual("NONE"); \
    BufferString icnnm( "s_od_icon_", \
			haveiconnm ? properties_.iconnm_.buf() : "noicon" ); \
    icnnm.addSpace().add( stlidx_ )

bool ODGoogle::KMLWriter::putIconStyles()
{
    if ( !isOK() )
	return false;

    mDeclIconStNm;
    strm() << "\n\t<Style id=\"" << icnnm << "\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.3</scale>\n";

    strm() << "\t\t</IconStyle>\n";
    strm() << "\t\t<LineStyle>\n\t\t\t<color>";
    strm() << properties_.color_.getStdStr( false, -1 );
    strm() << "</color>\n\t\t\t<width>";
    strm() << properties_.pixsize_;
    strm() << "</width>\n\t\t</LineStyle>\n";

    strm() << "\t</Style>";

    strm() << "\n\t<StyleMap id=\"m" << icnnm << "\">\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>normal</key>\n"
	"\t\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>highlight</key>\n"
	"\t\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t</StyleMap>";

    return true;
}


bool ODGoogle::KMLWriter::putPlaceMark( const Coord& crd, const char* nm )
{
    const LatLong ll = LatLong::transform( crd, true, inpcrs_.ptr() );
    return putPlaceMark( ll, 0., nm );
}


bool ODGoogle::KMLWriter::putPlaceMark( const Coord3& crd, const char* nm )
{
    const LatLong ll = LatLong::transform( crd.coord(), true, inpcrs_.ptr() );
    return putPlaceMark( ll, crd.z_, nm );
}


bool ODGoogle::KMLWriter::putPlaceMark( const LatLong& ll, double z,
					const char* nm )
{
    if ( !isOK() )
	return false;

    mDeclIconStNm;
    strm() << "\n\t<Placemark>";
    if ( nm && *nm )
	strm() << "\n\t\t<name>" << nm << "</name>";

    if ( !desc_.isEmpty() && !folderopen_ )
	strm() << "\n\t\t<description>" << desc_ << "</description>";

    const BufferString latstr( ::toString(ll.lat_));
    const BufferString lngstr(::toString(ll.lng_) );
    const bool hasdepth = !mIsZero(z,1e-2);
    strm() << "\n\t\t<LookAt>\n"
	"\t\t\t<longitude>" << lngstr << "</longitude>\n";
    strm() << "\t\t\t<latitude>" << latstr << "</latitude>\n";
    strm() << "\t\t\t<altitude>0</altitude>\n"
	"\t\t\t<range>500</range>\n"
	"\t\t\t<tilt>20</tilt>\n"
	"\t\t\t<heading>0</heading>\n"
	"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	"\t\t</LookAt>\n"
	"\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t<Point>\n";
    if ( hasdepth )
    {
	strm() << "\t\t\t<gx:altitudeMode>relativeToSeaFloor"
		  "</gx:altitudeMode>\n";
    }

    strm() << "\t\t\t<coordinates>";
    strm() << lngstr << ',';
    strm() << latstr << ",";
    strm() << -z;
    strm() << "</coordinates>\n"
	"\t\t</Point>\n"
	"\t</Placemark>";

    return true;
}


bool ODGoogle::KMLWriter::putLine( const TypeSet<Coord>& crds, const char* nm )
{
    TypeSet<Coord3> coords( crds.size(), Coord3::udf() );
    for ( int idx=0; idx<crds.size(); idx++ )
	coords[idx] = Coord3( crds[idx], 0. );

    return putLine( coords, nm, false );
}


#define mDeclPolyStNm \
    const BufferString stnm( properties_.isPolygon() ? "s_od_poly_" \
						     : "s_od_line", stlidx_ )

bool ODGoogle::KMLWriter::putLine( const TypeSet<Coord3>& crds, const char* nm,
				   bool hasdepths )
{
    if ( !isOK() )
	return false;

    mDeclPolyStNm;
    strm() << "\n\t<Placemark>";
    if ( nm && *nm )
	strm() << "\n\t\t<name>" << nm << "</name>";

    if ( !desc_.isEmpty() && !folderopen_ )
	strm() << "\n\t\t<description>" << desc_ << "</description>";

    strm() << "\n\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t<LineString>\n"
	"\t\t\t<tessellate>1</tessellate>\n"
	"\t\t\t<coordinates>\n";

    for ( const auto& crd : crds )
    {
	const LatLong ll =
	    LatLong::transform( crd.coord(), true, inpcrs_.ptr() );
	strm() << "\t\t\t\t" << ll.lng_;
	strm() << ',' << ll.lat_ << ',' << -crd.z_ << '\n';
    }

    strm() << "\t\t\t</coordinates>\n"
	"\t\t</LineString>\n"
	"\t</Placemark>";

    return true;
}


bool ODGoogle::KMLWriter::putPolyStyle()
{
    if ( !isOK() )
	return false;

    mDeclPolyStNm;
    strm() << "\n\t<Style id=\"" << stnm << "\">\n"
	"\t\t<LineStyle>\n"
	"\t\t\t<color>" << properties_.linestyle_.color_.getStdStr( false, -1 )
							    << "</color>\n"
	"\t\t\t<width>" << properties_.linestyle_.width_ << "</width>\n"
	"\t\t</LineStyle>";

    if ( properties_.isPolygon() )
    {
	strm() << "\n\t\t<PolyStyle>";
	strm() << "\n\t\t\t<color>"
	       << properties_.fillcolor_.getStdStr(false,-1)
	       << "</color>";
	strm() << "\n\t\t\t<fill>" << ::toString((properties_.dofill_ ? 1 : 0))
	       << "</fill>";
	strm() << "\n\t\t\t<outline>0</outline>"
		  "\n\t\t</PolyStyle>";
    }

    strm() << "\n\t</Style>";

    return true;
}


bool ODGoogle::KMLWriter::putPoly( const TypeSet<Coord>& crds,
				   const char* nm )
{
    TypeSet<Coord3> coords( crds.size(), Coord3::udf() );
    for ( int idx=0; idx<crds.size(); idx++ )
	coords[idx] = Coord3( crds[idx], 0. );

    return putPoly( coords, nm, false );
}


bool ODGoogle::KMLWriter::putPoly( const TypeSet<Coord3>& crds,
				   const char* nm, bool hasdepths )
{
    if ( !isOK() )
	return false;

    mDeclPolyStNm;
    strm() << "\n\t<Placemark>";
    if ( nm && *nm )
	strm() << "\n\t\t<name>" << nm << "</name>";

    if ( !desc_.isEmpty() && !folderopen_ )
	strm() << "\n\t\t<description>" << desc_ << "</description>";

    strm () << "\n\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t<Polygon>\n"
	"\t\t\t<extrude>1</extrude>\n"
	"\t\t\t<altitudeMode>clampedToGround</altitudeMode>\n"
	"\t\t\t<outerBoundaryIs>\n"
	"\t\t\t\t<LinearRing>\n"
	"\t\t\t\t\t<coordinates>\n";

    for ( const auto& crd : crds )
    {
	const LatLong ll =
	    LatLong::transform( crd.coord(), true, inpcrs_.ptr() );
	strm() << "\t\t\t\t\t\t" << ll.lng_;
	strm() << ',' << ll.lat_ << ',' << -crd.z_ << '\n';
    }

    strm() <<	"\t\t\t\t\t</coordinates>\n"
		"\t\t\t\t</LinearRing>\n"
		"\t\t\t</outerBoundaryIs>\n"
		"\t\t</Polygon>\n"
		"\t</Placemark>";

    return true;
}
