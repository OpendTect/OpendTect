/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpsurv.h"
#include "uisurvey.h"
#include "uifileinput.h"
#include "oddirs.h"


uiGoogleExportSurvey::uiGoogleExportSurvey( uiSurvey* uisurv )
    : uiDialog(uisurv,uiDialog::Setup("Export to KML",
				      "Specify output parameters","0.0.0") )
    , si_(uisurv->curSurvInfo())
{
    fnmfld_ = new uiFileInput( this, "Output file",
	    			uiFileInput::Setup(GetBaseDataDir())
					.forread(false)
	    				.filter("*.kml") );
}


bool uiGoogleExportSurvey::acceptOK( CallBacker* )
{
    return true;
}
