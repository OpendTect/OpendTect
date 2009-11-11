/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "odgooglexmlwriter.h"
#include "survinfo.h"
#include "strmprov.h"
#include "latlong.h"
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
	    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
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


void ODGoogle::XMLWriter::writeIconStyles( const char* iconnm, const char* ins )
{
    if ( !isOK() ) return;

    strm() << "\t<Style id=\"sn_od_"	<< iconnm << "0\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.1</scale>\n"
	"\t\t\t<Icon>\n"
	"\t\t\t\t<href>http://opendtect.org/images/od-"
					<< iconnm << ".png</href>\n"
	"\t\t\t</Icon>\n"
	"\t\t<hotSpot x=\"20\" y=\"2\" xunits=\"pixels\" yunits=\"pixels\"/>\n"
	"\t\t</IconStyle>\n"
	"\t</Style>\n\n";

    strm() << "\t<StyleMap id=\"msn_od_"<< iconnm << "\">\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>normal</key>\n"
	"\t\t\t<styleUrl>#sn_od_"	<< iconnm << "0</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>highlight</key>\n"
	"\t\t\t<styleUrl>#od-"		<< iconnm << "</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t</StyleMap>\n\n";

    strm() << "\t<Style id=\"od-"	<< iconnm << "\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.3</scale>\n"
	"\t\t\t<Icon>\n"
	"\t\t\t\t<href>http://opendtect.org/images/od-"
					<< iconnm << ".png</href>\n"
	"\t\t\t</Icon>\n"
	"\t\t\t<hotSpot x=\"20\" y=\"2\" xunits=\"pixels\" "
					"yunits=\"pixels\"/>\n"
	"\t\t</IconStyle>\n";

    if ( ins && *ins )
	strm() << ins << '\n';

    strm() << "\t</Style>\n\n" << std::endl;
}


void ODGoogle::XMLWriter::writePlaceMark( const char* iconnm, const Coord& crd,
					  const char* nm )
{
    if ( !isOK() ) return;

    const LatLong ll( SI().latlong2Coord().transform(crd) );

    strm() << "\n\t<Placemark>\n"
	   << "\t\t<name>" << nm << "</name>\n"
	   << "\t\t<LookAt>\n";
    strm() << "\t\t\t<longitude>" << getStringFromDouble(0,ll.lng_)
				      << "</longitude>\n";
    strm() << "\t\t\t<latitude>" << getStringFromDouble(0,ll.lat_)
				      << "</latitude>\n";
    strm() << "\t\t\t<altitude>0</altitude>\n"
	"\t\t\t<range>500</range>\n"
	"\t\t\t<tilt>20</tilt>\n"
	"\t\t\t<heading>0</heading>\n"
	"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	"\t\t</LookAt>\n"
	"\t\t<styleUrl>#msn_od_" << iconnm << "</styleUrl>\n"
	"\t\t<Point>\n"
	"\t\t\t<coordinates>" << getStringFromDouble(0,ll.lng_);
    strm() << ',' << getStringFromDouble(0,ll.lat_) << ",0</coordinates>\n"
	"\t\t</Point>\n"
	"\t</Placemark>" << std::endl;
}
