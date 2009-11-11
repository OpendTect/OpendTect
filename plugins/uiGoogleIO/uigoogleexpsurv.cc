/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpsurv.h"
#include "odgooglexmlwriter.h"
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
				      "Specify output parameters","0.3.10") )
    , si_(uisurv->curSurvInfo())
{
    const Color defcol( 255, 170, 0, 100 );
    colfld_ = new uiColorInput( this,
	    			uiColorInput::Setup(defcol).lbltxt("Color") );
    colfld_->enableAlphaSetting( true );

    hghtfld_ = new uiGenInput( this, "Border height", FloatInpSpec(50) );
    hghtfld_->attach( alignedBelow, colfld_ );

    fnmfld_ = new uiFileInput( this, "Output file",
		uiFileInput::Setup(uiFileDialog::Gen,GetBaseDataDir())
		.forread(false).filter("*.kml") );
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

    ODGoogle::XMLWriter wrr( "Survey area", fnm, si_->name() );
    if ( !wrr.isOK() )
	mErrRet(wrr.errMsg())

    wrr.strm() << "\t<Style id=\"ODSurvey\">\n"
	    "\t\t<LineStyle>\n"
	    "\t\t\t<width>1.5</width>\n"
	    "\t\t</LineStyle>\n"
	    "\t\t<PolyStyle>\n";
    wrr.strm() << "\t\t\t<color>" << colfld_->color().getStdStr(false,-1)
				  << "</color>\n"
	    "\t\t</PolyStyle>\n"
	    "\t</Style>\n";
    wrr.strm() << "\t<Placemark>\n"
	    "\t\t<name>" << si_->name() << "</name>\n"
	    "\t\t<styleUrl>#ODSurvey</styleUrl>\n"
	    "\t\t<Polygon>\n"
	    "\t\t\t<extrude>1</extrude>\n"
	    "\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	    "\t\t\t<outerBoundaryIs>\n"
	    "\t\t\t\t<LinearRing>\n";

    const float hght = hghtfld_->getfValue();
    StepInterval<int> inlrg = si_->inlRange( false );
    StepInterval<int> crlrg = si_->crlRange( false );
    const Coord corner1( si_->transform(BinID(inlrg.start,crlrg.start)) );
    const Coord corner2( si_->transform(BinID(inlrg.start,crlrg.stop)) );
    const Coord corner3( si_->transform(BinID(inlrg.stop,crlrg.stop)) );
    const Coord corner4( si_->transform(BinID(inlrg.stop,crlrg.start)) );
    const LatLong ll1( si_->latlong2Coord().transform(corner1) );
    const LatLong ll2( si_->latlong2Coord().transform(corner2) );
    const LatLong ll3( si_->latlong2Coord().transform(corner3) );
    const LatLong ll4( si_->latlong2Coord().transform(corner4) );

#define mWrLL(ll) \
    wrr.strm() << "\t\t\t\t\t\t" << getStringFromDouble(0,ll.lng_); \
    wrr.strm() << ',' << getStringFromDouble(0,ll.lat_) \
	       << ',' << hght << '\n'
    wrr.strm() << "\t\t\t\t\t<coordinates>\n";
    mWrLL(ll1);
    mWrLL(ll2);
    mWrLL(ll3);
    mWrLL(ll4);
    mWrLL(ll1);
    wrr.strm() << "\t\t\t\t\t</coordinates>\n";

    wrr.strm() << "\t\t\t\t</LinearRing>\n"
	    "\t\t\t</outerBoundaryIs>\n"
	    "\t\t</Polygon>\n"
	    "\t</Placemark>\n";

    wrr.close();
    return true;
}
