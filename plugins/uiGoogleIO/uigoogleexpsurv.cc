/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID mUnusedVar = "$Id: uigoogleexpsurv.cc,v 1.22 2012-07-10 08:05:27 cvskris Exp $";

#include "uigoogleexpsurv.h"
#include "googlexmlwriter.h"
#include "uisurvey.h"
#include "uisellinest.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "latlong.h"
#include "draw.h"
#include <iostream>


uiGoogleExportSurvey::uiGoogleExportSurvey( uiSurvey* uisurv )
    : uiDialog(uisurv,uiDialog::Setup("Export Survey boundaries to KML",
				      "Specify output parameters","0.3.10") )
    , si_(uisurv->curSurvInfo())
{
    const LineStyle ls( LineStyle::Solid, 20, Color(255,170,80,100) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

    hghtfld_ = new uiGenInput( this, "Border height", FloatInpSpec(500) );
    hghtfld_->attach( alignedBelow, lsfld_ );

    mImplFileNameFld("survbounds");
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
    coords += si_->transform(BinID(inlrg.start,crlrg.start));

    const float reqwdth = lsfld_->getWidth() * 0.1;
    wrr.writePolyStyle( "survey", lsfld_->getColor(), mNINT32(reqwdth) );
    wrr.writePoly( "survey", si_->name(), coords, hghtfld_->getfValue(), si_ );

    return true;
}
