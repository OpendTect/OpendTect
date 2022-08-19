/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "googlexmlwriter.h"
#include "coordsystem.h"
#include "survinfo.h"
#include "latlong.h"
#include "color.h"
#include "coordsystem.h"
#include "uistrings.h"
#include "enums.h"

#define mErrRet(s) { errmsg_ = s; return false; }

ODGoogle::KMLWriter::KMLWriter()
{}

ODGoogle::KMLWriter::~KMLWriter()
{
}

bool ODGoogle::KMLWriter::open( const char* fnm )
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
    treenm += ": "; treenm += elemnm_;
    strm() << "<name>" << treenm << "</name>" << od_endl;

    if ( !strm().isOK() )
    {
	uiString emsg( tr("Error during write of XML header info") );
	strm().addErrMsgTo( emsg );
	mErrRet(emsg)
    }

    return true;
}


void ODGoogle::KMLWriter::setStream( const BufferString& fnm )
{
    strm_ = new od_ostream( fnm );
    open( fnm );
}


bool ODGoogle::KMLWriter::close()
{
    errmsg_.setEmpty();

    if ( strm_ && strm_->isOK() )
    {
	strm() << "</Document>\n</kml>" << od_endl;
	deleteAndZeroPtr( strm_ );
    }
    else
	return false;

    return true;
}


#define mDeclIconStNm \
    const bool haveiconnm = !properties_.iconnm_.isEmpty() && \
				     !properties_.iconnm_.isEqual("NONE"); \
    const BufferString icnnm( "s_od_icon_", \
			haveiconnm ? properties_.iconnm_.buf() : "noicon" ) \

bool ODGoogle::KMLWriter::writePolygon(
				const RefObjectSet<const Pick::Set>& picks )
{
    for ( int i=0; i<picks.size(); i++ )
    {
	const Pick::Set* pick = picks.get( i );
	if ( !pick )
	    continue;

	const TypeSet<Pick::Location>& locations = pick->locations();
	if ( locations.size() < 3 )
	    continue;

	TypeSet<Coord3> coords;
	for ( const auto& loc : locations )
	    coords += loc.pos();

	if ( coords.first() != coords.last() )
	    coords.add( coords.first() );

	properties_.color_ = pick->disp_.color_;
	putPolyStyle();
	putPoly( coords, pick->name() );
    }

    return true;
}


bool ODGoogle::KMLWriter::writePolygon( const TypeSet<Coord>& coords,
					const char* nm )
{
    TypeSet<Coord3> crdset;
    for ( auto coord : coords )
    {
	Coord3 crd( coord, 0 );
	crdset +=  crd;
    }

    return putPolyStyle() && putPoly( crdset, nm );
}


bool ODGoogle::KMLWriter::writePolygon( const TypeSet<Coord3>& coords,
					const char* nm )
{
    return putPolyStyle() && putPoly( coords, nm );
}


bool ODGoogle::KMLWriter::writeLine( const RefObjectSet<const Pick::Set>& picks)
{
    putPolyStyle();


    for ( int i=0; i<picks.size(); i++ )
    {
	const Pick::Set* pick = picks.get( i );
	if ( !pick )
	    continue;

	const TypeSet<Pick::Location>& locations = pick->locations();
	TypeSet<Coord> coords;
	for ( const auto& loc : locations )
	    coords += loc.pos();

	putLine( coords , pick->name() );
    }

    return true;
}


bool ODGoogle::KMLWriter::writeLine( const TypeSet<Coord>& crdset,
				     const char*nm )
{
    return putPolyStyle() && putLine( crdset, nm );
}


bool ODGoogle::KMLWriter::writePoint(
				const RefObjectSet<const Pick::Set>& picks )
{
    if ( !putIconStyles() )
	return false;

    for ( int i=0; i<picks.size(); i++ )
    {
	const Pick::Set* pick = picks.get( i );
	if ( !pick )
	    continue;

	const TypeSet<Pick::Location>& locations = pick->locations();
	for ( const auto& loc : locations )
	    putPlaceMark( loc.pos() , pick->name() );
    }

    return true;
}


bool ODGoogle::KMLWriter::writePoint( const LatLong& ll, const char* nm )
{
    return putIconStyles() && putPlaceMark( ll, nm );
}


bool ODGoogle::KMLWriter::writePoint( const Coord& coord, const char* nm )
{
    return putIconStyles() && putPlaceMark( coord, nm );
}


bool ODGoogle::KMLWriter::writePoints( const TypeSet<Coord>& crds,
				       const BufferStringSet& nms )
{
    if ( !putIconStyles() )
	return false;

    for ( int idx=0; idx<crds.size(); idx++  )
	putPlaceMark( crds.get(idx), nms.get(idx) );

    return true;
}


bool ODGoogle::KMLWriter::putIconStyles()
{
    if ( !isOK() )
	return false;

    mDeclIconStNm;

    strm() << "\t<Style id=\"" << icnnm << "\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.3</scale>\n";

    strm() << "\t\t</IconStyle>\n";
    strm() << "\t\t<LineStyle>\n\t\t\t<color>";
    strm() << properties_.color_.getStdStr( false, -1 );
    strm() << "</color>\n\t\t\t<width>";
    strm() << properties_.width_;
    strm() << "</width>\n\t\t</LineStyle>\n";

    strm() << "\t</Style>\n\n";

    strm() << "\t<StyleMap id=\"m" << icnnm << "\">\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>normal</key>\n"
	"\t\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>highlight</key>\n"
	"\t\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t</StyleMap>\n\n" << od_endl;

    return true;
}


bool ODGoogle::KMLWriter::putPlaceMark( const Coord& crd, const char* nm )
{
    return putPlaceMark( LatLong::transform(crd, true,coordsys_), nm );
}


bool ODGoogle::KMLWriter::putPlaceMark( const LatLong& ll, const char* nm )
{
    if ( !isOK() )
	return false;

    mDeclIconStNm;
    strm() << "\n\t<Placemark>\n"
	<< "\t\t<name>" << nm << "</name>\n";
    if ( !desc_.isEmpty() )
	strm() << "\t\t<description>" << desc_ << "</description>\n";
    const BufferString latstr( ::toString(ll.lat_)),
						lngstr(::toString(ll.lng_) );
    strm() << "\t\t<LookAt>\n"
	"\t\t\t<longitude>" << lngstr << "</longitude>\n";
    strm() << "\t\t\t<latitude>" << latstr << "</latitude>\n";
    strm() << "\t\t\t<altitude>0</altitude>\n"
	"\t\t\t<range>500</range>\n"
	"\t\t\t<tilt>20</tilt>\n"
	"\t\t\t<heading>0</heading>\n"
	"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	"\t\t</LookAt>\n"
	"\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t<Point>\n"
	"\t\t\t<coordinates>" << lngstr;
    strm() << ',' << latstr << ",0</coordinates>\n"
	"\t\t</Point>\n"
	"\t</Placemark>\n" << od_endl;

    return true;
}


bool ODGoogle::KMLWriter::putLine( const TypeSet<Coord>& crds, const char* nm )
{
    if ( !isOK() )
	return false;

    mDeclIconStNm;
    strm() << "\n\t<Placemark>\n"
	"\t\t<name>" << nm << " [line]</name>\n"
	"\t\t<styleUrl>#" << icnnm << "</styleUrl>\n"
	"\t\t<LineString>\n"
	"\t\t\t<tessellate>1</tessellate>\n"
	"\t\t\t<coordinates>\n";

    for ( int idx=0; idx<crds.size(); idx++ )
    {
	const LatLong ll( LatLong::transform(crds[idx], true, coordsys_) );
	strm() << ll.lng_ << ','; // keep sep from next line
	strm() << ll.lat_ << ",0 ";
    }

    strm() << "\t\t\t</coordinates>\n"
	"\t\t</LineString>\n"
	"\t</Placemark>\n" << od_endl;

    return true;
}


#define mDeclPolyStNm \
    const BufferString stnm( "s_od_poly_", properties_.stlnm_ )

bool ODGoogle::KMLWriter::putPolyStyle()
{
    if ( !isOK() )
	return false;

    mDeclPolyStNm;
    strm() << "\t<Style id=\"" << properties_.stlnm_ << "\">\n"
	"\t\t<LineStyle>\n"
	"\t\t\t<width>" << properties_.width_ << "</width>\n"
	"\t\t\t<color>" << properties_.color_.getStdStr( false, -1 )
							    << "</color>\n"
	"\t\t</LineStyle>\n"
	"\t\t<PolyStyle>\n"
	"\t\t\t<color>" << properties_.color_.getStdStr( false, -1 )
			<< "</color>\n";
    strm() << "\t\t</PolyStyle>\n"
	"\t</Style>\n" << od_endl;

    return true;
}


bool ODGoogle::KMLWriter::putPoly( const TypeSet<Coord3>& coords,
				   const char* nm )
{
    if ( !isOK() )
	return false;

    mDeclPolyStNm;
    strm() << "\t<Placemark>\n"
	"\t\t<name>" << nm << "</name>\n"
	"\t\t<styleUrl>#" << properties_.stlnm_ << "</styleUrl>\n"
	"\t\t<Polygon>\n"
	"\t\t\t<extrude>1</extrude>\n"
	"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	"\t\t\t<outerBoundaryIs>\n"
	"\t\t\t\t<LinearRing>\n"
	"\t\t\t\t\t<coordinates>\n";

    for ( int idx=0; idx<coords.size(); idx++ )
    {
	Coord3 crd = coords[idx];
	const LatLong ll(LatLong::transform( crd.coord(),
						true, coordsys_) );
	strm() << "\t\t\t\t\t\t" << ll.lng_;
	strm() << ',' << ll.lat_ << ',' << crd.z << '\n';
    }
    strm() <<	"\t\t\t\t\t</coordinates>\n"
		"\t\t\t\t</LinearRing>\n"
		"\t\t\t</outerBoundaryIs>\n"
		"\t\t</Polygon>\n"
		"\t</Placemark>\n" << od_endl;

    return true;
}
