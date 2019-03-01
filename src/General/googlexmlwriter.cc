/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "googlexmlwriter.h"
#include "survinfo.h"
#include "latlong.h"
#include "color.h"
#include "coordsystem.h"
#include "uistrings.h"
#include "enums.h"

#define mErrRet(s) { errmsg_ = s; return false; }

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


void ODGoogle::KMLWriter::close()
{
    errmsg_.setEmpty();

    if ( strm_->isOK() )
	strm() << "</Document>\n</kml>" << od_endl;
    delete strm_; strm_ = 0;
}


#define mDeclIconStNm \
    const bool haveiconnm = !properties_.iconnm_.isEmpty(); \
    const BufferString stnm( "s_od_icon_", \
			haveiconnm ? properties_.iconnm_ : "noicon" ) \

void ODGoogle::KMLWriter::writePolygon( const pickset& picks )
{
    putPolyStyle();

    for ( int i = 0; i < picks.size(); i++ )
    {
	TypeSet<Coord3d> coords;
	BufferString nm;
	Coord3d crd;
	const Pick::Set* pick = picks.get( i );
	nm = pick->name();
	Pick::SetIter psiter( *pick );
	while (psiter.next())
	{
	    crd.setXY( psiter.getPos() );
	    crd.z_ = psiter.getZ();
	    coords += crd;
	}
	psiter.retire();
	putPoly( coords, nm );
    }
}


void ODGoogle::KMLWriter::writePolygon( const coord2dset& coords,
								const char* nm )
{
    coord3dset crdset;
    for ( auto coord : coords )
    {
	Coord3d crd( coord, 0 );
	crdset +=  crd;
    }
    putPolyStyle();
    putPoly( crdset, nm );
}


void ODGoogle::KMLWriter::writePolygon( const coord3dset& coords,
								const char* nm )
{
    putPoly( coords, nm );
}


void ODGoogle::KMLWriter::writeLine( const pickset& picks )
{
    for ( int i = 0; i < picks.size(); i++ )
    {
	BufferString nm;
	TypeSet<Coord> coords;
	const Pick::Set* pick = picks.get( i );
	nm = pick->name();
	pick->getLocations( coords );
	putLine( coords, nm );
    }
}


void ODGoogle::KMLWriter::writeLine( const coord2dset& crdset, const char*nm )
{
    putLine( crdset, nm );
}


void ODGoogle::KMLWriter::writePoint( const pickset& picks )
{
    putIconStyles();
    TypeSet<Coord> coords;
    BufferString nm;
    for ( int i = 0; i < picks.size(); i++ )
    {
	const Pick::Set* pick = picks.get( i );
	nm = pick->name();
	Pick::SetIter psiter( *pick );
	coords += psiter.getPos();
	psiter.retire();
	for ( auto coord : coords )
	    putPlaceMark( coord, nm );
    }
}


void ODGoogle::KMLWriter::writePoint( const Coord& coord, const char* nm )
{
    TypeSet<Coord> crds; crds += coord;
    putIconStyles();
    putPlaceMark( coord, nm );
}


void ODGoogle::KMLWriter::writePoints( const coord2dset& crds,
						const BufferStringSet& nms )
{
    putIconStyles();
    for ( int idx=0; idx<crds.size(); idx++  )
	putPlaceMark( crds.get(idx), nms.get(idx) );

}


void ODGoogle::KMLWriter::putIconStyles()
{
    if ( !isOK() ) return; mDeclIconStNm;

    strm() << "\t<Style id=\"" << stnm << "\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.3</scale>\n";
    if ( !properties_.iconnm_.isEqual("NONE") )
	strm() << "\t\t\t<Icon></Icon>\n";
    else
	strm() << "\t\t\t<Icon>\n"
	"\t\t\t\t<href>http://opendtect.org/images/od-"
	<< properties_.iconnm_ << ".png</href>\n"
	"\t\t\t</Icon>\n"
	"\t\t<hotSpot x=\"" << properties_.xpixoffs_ <<
	"\" y=\"2\" xunits=\"pixels\" yunits=\"pixels\"/>\n";
    strm() << "\t\t</IconStyle>\n";
    strm() << "\t\t<OD::LineStyle>\n\t\t\t<color>";
    strm() << properties_.color_.getStdStr( false, -1 );
    strm() << "</color>\n\t\t\t<width>";
    strm() << properties_.width_;
    strm() << "</width>\n\t\t</OD::LineStyle>\n";

    strm() << "\t</Style>\n\n";

    strm() << "\t<StyleMap id=\"m" << stnm << "\">\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>normal</key>\n"
	"\t\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>highlight</key>\n"
	"\t\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t</StyleMap>\n\n" << od_endl;

}


void ODGoogle::KMLWriter::putPlaceMark( const Coord& crd, const char* nm )
{
    putPlaceMark( LatLong::transform(crd, true,coordsys_), nm );
}


void ODGoogle::KMLWriter::putPlaceMark( const LatLong& ll, const char* nm )
{
    if ( !isOK() ) return; mDeclIconStNm;

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
	"\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t<Point>\n"
	"\t\t\t<coordinates>" << lngstr;
    strm() << ',' << latstr << ",0</coordinates>\n"
	"\t\t</Point>\n"
	"\t</Placemark>\n" << od_endl;
}


void ODGoogle::KMLWriter::putLine( const TypeSet<Coord>& crds, const char* nm )
{
    if ( !isOK() ) return; mDeclIconStNm;

    strm() << "\n\t<Placemark>\n"
	"\t\t<name>" << nm << " [line]</name>\n"
	"\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t<LineString>\n"
	"\t\t\t<tessellate>1</tessellate>\n"
	"\t\t\t<coordinates>\n";

    for ( int idx = 0; idx < crds.size(); idx++ )
    {
	const LatLong ll( LatLong::transform(crds[idx], true, coordsys_) );
	strm() << ll.lng_ << ','; // keep sep from next line
	strm() << ll.lat_ << ",0 ";
    }

    strm() << "\t\t\t</coordinates>\n"
	"\t\t</LineString>\n"
	"\t</Placemark>\n" << od_endl;
}


#define mDeclPolyStNm \
    const BufferString stnm( "s_od_poly_", properties_.stlnm_ )

void ODGoogle::KMLWriter::putPolyStyle()
{
    if ( !isOK() ) return; mDeclPolyStNm;

    strm() << "\t<Style id=\"" << properties_.stlnm_ << "\">\n"
	"\t\t<OD::LineStyle>\n"
	"\t\t\t<width>" << properties_.width_ << "</width>\n"
	"\t\t</OD::LineStyle>\n"
	"\t\t<PolyStyle>\n"
	"\t\t\t<color>" << properties_.color_.getStdStr( false, -1 )
			<< "</color>\n";
    strm() << "\t\t</PolyStyle>\n"
	"\t</Style>\n" << od_endl;
}


void ODGoogle::KMLWriter::putPoly( const TypeSet<Coord3d>& coords,
								const char* nm )
{
    if ( !isOK() ) return; mDeclPolyStNm;

    strm() << "\t<Placemark>\n"
	"\t\t<name>" << nm << "</name>\n"
	"\t\t<styleUrl>#" << properties_.stlnm_ << "</styleUrl>\n"
	"\t\t<Polygon>\n"
	"\t\t\t<extrude>1</extrude>\n"
	"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	"\t\t\t<outerBoundaryIs>\n"
	"\t\t\t\t<LinearRing>\n"
	"\t\t\t\t\t<coordinates>\n";

    for ( int idx = 0; idx < coords.size(); idx++ )
    {
	Coord3d crd = coords[idx];
	const LatLong ll(LatLong::transform( crd.getXY(),
						true, coordsys_) );
	strm() << "\t\t\t\t\t\t" << ll.lng_;
	strm() << ',' << ll.lat_ << ',' << crd.z_ << '\n';
    }
    strm() << "\t\t\t\t\t</coordinates>\n"
	"\t\t\t\t</LinearRing>\n"
	"\t\t\t</outerBoundaryIs>\n"
	"\t\t</Polygon>\n"
	"\t</Placemark>\n" << od_endl;
}
