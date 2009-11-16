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

    mImplFileNameFld;
    fnmfld_->attach( alignedBelow, hghtfld_ );
}


uiGoogleExportSurvey::~uiGoogleExportSurvey()
{
}


bool uiGoogleExportSurvey::acceptOK( CallBacker* )
{
    mCreateWriter( "Survey area", si_->name() );

    const StepInterval<int> inlrg = si_->inlRange( false );
    const StepInterval<int> crlrg = si_->crlRange( false );
    TypeSet<Coord> coords;
    coords += si_->transform(BinID(inlrg.start,crlrg.start));
    coords += si_->transform(BinID(inlrg.start,crlrg.stop));
    coords += si_->transform(BinID(inlrg.stop,crlrg.stop));
    coords += si_->transform(BinID(inlrg.stop,crlrg.start));

    wrr.writePolyStyle( "survey", colfld_->color(), 1.5 );
    wrr.writePoly( "survey", si_->name(), coords, hghtfld_->getfValue(), si_ );

    return true;
}
