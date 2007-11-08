/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpsurv.h"
#include "uisurvey.h"
#include "uicolor.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "latlong.h"
#include <iostream>


uiGoogleExportSurvey::uiGoogleExportSurvey( uiSurvey* uisurv )
    : uiDialog(uisurv,uiDialog::Setup("Export Survey boundaries to KML",
				      "Specify output parameters","0.0.0") )
    , si_(uisurv->curSurvInfo())
{
    colfld_ = new uiColorInput( this, Color(255,0,0,255), "Color" );
    colfld_->enableAlphaSetting( true );

    hghtfld_ = new uiGenInput( this, "Border height", FloatInpSpec(5) );
    hghtfld_->attach( alignedBelow, colfld_ );

    fnmfld_ = new uiFileInput( this, "Output file",
	    			uiFileInput::Setup(GetBaseDataDir())
					.forread(false)
	    				.filter("*.kml;;*.kmz") );
    fnmfld_->attach( alignedBelow, hghtfld_ );
}


uiGoogleExportSurvey::~uiGoogleExportSurvey()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGoogleExportSurvey::acceptOK( CallBacker* )
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet("Please enter a file name" )
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet("Cannot create file" )

    std::ostream& strm = *sd.ostrm;
    strm << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
	    "<Document>\n" << std::endl;
    strm << "\t<name>" << fnm << "</name>\n";
    strm << "\t<Style id=\"ODSurvey\">\n"
	    "\t\t<LineStyle>\n"
	    "\t\t\t<width>1.5</width>\n"
	    "\t\t</LineStyle>\n"
	    "\t\t<PolyStyle>\n"
	    //TODO use user specified color: colfld_->color()
	    "\t\t\t<color>7d0000ff</color>\n"
	    "\t\t</PolyStyle>\n"
	    "\t</Style>\n";
    strm << "\t<Placemark>\n"
	    "\t\t<name>" << si_->name() << "</name>\n"
	    "\t\t<styleUrl>#ODSurvey</styleUrl>\n"
	    "\t\t<Polygon>\n"
	    "\t\t\t<extrude>1</extrude>\n"
	    "\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	    "\t\t\t<outerBoundaryIs>\n"
	    "\t\t\t\t<LinearRing>\n";

    const float hght = hghtfld_->getfValue();
    const Coord corner1( si_->minCoord(false) );
    const Coord corner3( si_->maxCoord(false) );
    const Coord corner2( corner3.x, corner1.y );
    const Coord corner4( corner1.x, corner3.y );
    const LatLong ll1( si_->latlong2Coord().transform(corner1) );
    const LatLong ll2( si_->latlong2Coord().transform(corner2) );
    const LatLong ll3( si_->latlong2Coord().transform(corner3) );
    const LatLong ll4( si_->latlong2Coord().transform(corner4) );
    strm << "\t\t\t\t\t<coordinates>\n"
	 << ll1.lat_ << ',' << ll1.lng_ << ',' << hght << ' '
	 << ll2.lat_ << ',' << ll2.lng_ << ',' << hght << ' '
	 << ll3.lat_ << ',' << ll3.lng_ << ',' << hght << ' '
	 << ll4.lat_ << ',' << ll4.lng_ << ',' << hght << ' '
	 << ll1.lat_ << ',' << ll1.lng_ << ',' << hght << '\n'
	 << "\t\t\t\t\t</coordinates>\n";

    strm << "\t\t\t\t</LinearRing>\n"
	    "\t\t\t</outerBoundaryIs>\n"
	    "\t\t</Polygon>\n"
	    "\t</Placemark>\n"
	    "</Document>\n"
	    "</kml>\n";

    sd.close();
    return true;
}
