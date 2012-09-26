/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID mUsedVar = "$Id";

#include "googlexmlwriter.h"
#include "survinfo.h"
#include "strmprov.h"
#include "latlong.h"
#include "color.h"
#include <iostream>

ODGoogle::XMLWriter::XMLWriter( const char* enm, const char* fnm,
				const char* snm )
    : sd_(*new StreamData)
    , elemnm_(enm)
    , survnm_(snm)
{
    open( fnm );
}


std::ostream& ODGoogle::XMLWriter::strm()
{
    return *sd_.ostrm;
}


bool ODGoogle::XMLWriter::isOK() const
{
    return sd_.usable() && strm().good();
}


#define mErrRet(s,s2) { errmsg_ = s; if ( s2 ) errmsg_ += s2; return false; }

bool ODGoogle::XMLWriter::open( const char* fnm )
{
    close();

    if ( !fnm || !*fnm )
	mErrRet("No file name provided",0)
    sd_ = StreamProvider(fnm).makeOStream();
    if ( !sd_.usable() )
	mErrRet("Cannot create file: ",fnm)

    strm() << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<kml xmlns=\"http://www.opengis.net/kml/2.2\" "
	    "xmlns:gx=\"http://www.google.com/kml/ext/2.2\" "
	    "xmlns:kml=\"http://www.opengis.net/kml/2.2\" "
	    "xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
	    "<Document>\n";
    BufferString treenm = survnm_.isEmpty() ? SI().name() : survnm_;
    treenm += ": "; treenm += elemnm_;
    strm() << "<name>" << treenm << "</name>" << std::endl;

    if ( !strm().good() )
	mErrRet("Error during write of XML header info",fnm)

    return true;
}


void ODGoogle::XMLWriter::close()
{
    errmsg_.setEmpty();
    if ( !sd_.usable() ) return;

    strm() << "</Document>\n</kml>" << std::endl;
    sd_.close();
}


void ODGoogle::XMLWriter::start( const ODGoogle::XMLItem& itm )
{
}



void ODGoogle::XMLWriter::finish( const ODGoogle::XMLItem& itm )
{
}


#define mDeclIconStNm \
    const bool haveiconnm = iconnm && *iconnm; \
    const BufferString stnm( "s_od_icon_", haveiconnm ? iconnm : "noicon" )

void ODGoogle::XMLWriter::writeIconStyles( const char* iconnm, int xpixoffs,
					   const char* ins )
{
    if ( !isOK() ) return; mDeclIconStNm;

    strm() <<	"\t<Style id=\"" << stnm << "\">\n"
		"\t\t<IconStyle>\n"
		"\t\t\t<scale>1.3</scale>\n";
    if ( !haveiconnm )
	strm() << "\t\t\t<Icon></Icon>\n";
    else
	strm() << "\t\t\t<Icon>\n"
		  "\t\t\t\t<href>http://opendtect.org/images/od-"
						<< iconnm << ".png</href>\n"
		  "\t\t\t</Icon>\n"
		  "\t\t<hotSpot x=\"" << xpixoffs <<
			  "\" y=\"2\" xunits=\"pixels\" yunits=\"pixels\"/>\n";
    strm() <<	"\t\t</IconStyle>\n";
    if ( ins && *ins )
	strm() << ins << '\n';
    strm() <<	"\t</Style>\n\n";

    strm() << "\t<StyleMap id=\"m"<< stnm << "\">\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>normal</key>\n"
	"\t\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>highlight</key>\n"
	"\t\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t</StyleMap>\n\n" << std::endl;

}


void ODGoogle::XMLWriter::writePlaceMark( const char* iconnm, const Coord& crd,
					  const char* nm )
{
    writePlaceMark( iconnm, SI().latlong2Coord().transform(crd), nm );
}


void ODGoogle::XMLWriter::writePlaceMark( const char* iconnm,
					  const LatLong& ll, const char* nm,
       					  const char* desc )
{
    if ( !isOK() ) return; mDeclIconStNm;

    strm() << "\n\t<Placemark>\n"
	   << "\t\t<name>" << nm << "</name>\n";
    if ( desc && *desc )
	strm() << "\t\t<description>" << desc << "</description>\n";
    char lngstr[255]; getStringFromDouble(0,ll.lng_, lngstr );
    strm() << "\t\t<LookAt>\n"
	      "\t\t\t<longitude>" << lngstr << "</longitude>\n";
    char latstr[255]; getStringFromDouble(0,ll.lat_, latstr );
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
	"\t</Placemark>\n" << std::endl;
}


void ODGoogle::XMLWriter::writeLine( const char* iconnm,
				     const TypeSet<Coord>& crds,
       				     const char* nm )
{
    if ( !isOK() ) return; mDeclIconStNm;

    strm() << "\n\t<Placemark>\n"
	      "\t\t<name>" << nm << " [line]</name>\n"
	      "\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
	      "\t\t<LineString>\n"
	      "\t\t\t<tessellate>1</tessellate>\n"
	      "\t\t\t<coordinates>\n";

    char str[255];
    for ( int idx=0; idx<crds.size(); idx++ )
    {
	const LatLong ll( SI().latlong2Coord().transform(crds[idx]) );

	getStringFromDouble( 0, ll.lng_, str );
	strm() << str << ',';
	getStringFromDouble( 0, ll.lat_, str );
	strm() << str << ",0 ";
    }

    strm() << "\t\t\t</coordinates>\n"
	      "\t\t</LineString>\n"
	      "\t</Placemark>\n" << std::endl;
}


#define mDeclPolyStNm \
    const BufferString stnm( "s_od_poly_", stylnm )

void ODGoogle::XMLWriter::writePolyStyle( const char* stylnm, const Color& col,
					  int wdth )
{
    if ( !isOK() ) return; mDeclPolyStNm;

    strm() <<	"\t<Style id=\"" << stnm << "\">\n"
		"\t\t<LineStyle>\n"
		"\t\t\t<width>" << wdth << "</width>\n"
		"\t\t</LineStyle>\n"
		"\t\t<PolyStyle>\n"
		"\t\t\t<color>" << col.getStdStr(false,-1) << "</color>\n";
    strm() <<	"\t\t</PolyStyle>\n"
		"\t</Style>\n" << std::endl;
}


void ODGoogle::XMLWriter::writePoly( const char* stylnm, const char* nm,
				     const TypeSet<Coord>& coords, float hght,
       				     const SurveyInfo* si )
{
    if ( !isOK() ) return; mDeclPolyStNm;
    if ( !si ) si = &SI();

    strm() <<	"\t<Placemark>\n"
		"\t\t<name>" << nm << "</name>\n"
		"\t\t<styleUrl>#" << stnm << "</styleUrl>\n"
		"\t\t<Polygon>\n"
		"\t\t\t<extrude>1</extrude>\n"
		"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
		"\t\t\t<outerBoundaryIs>\n"
		"\t\t\t\t<LinearRing>\n"
		"\t\t\t\t\t<coordinates>\n";

    char str[255];
    for ( int idx=0; idx<coords.size(); idx++ )
    {
	const LatLong ll( si->latlong2Coord().transform(coords[idx]) );
	getStringFromDouble( 0, ll.lng_, str );
	strm() << "\t\t\t\t\t\t" << str;
	getStringFromDouble( 0, ll.lat_, str );
	strm() << ',' << str << ',' << hght << '\n';
    }
    strm() <<	"\t\t\t\t\t</coordinates>\n"
		"\t\t\t\t</LinearRing>\n"
		"\t\t\t</outerBoundaryIs>\n"
		"\t\t</Polygon>\n"
		"\t</Placemark>\n" << std::endl;
}
