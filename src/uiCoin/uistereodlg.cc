/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uistereodlg.h"

#include "uimsg.h"
#include "uislider.h"
#include "ui3dviewer.h"


uiStereoDlg::uiStereoDlg( uiParent* p, ObjectSet<ui3DViewer>& vwrs_ )
	: uiDialog(p, uiDialog::Setup("Stereo viewing",
		    		      "Set stereo offset","50.0.2")
		      .canceltext(""))
	, vwrs(vwrs_)
{
    sliderfld = new uiSliderExtra( this, 
	    		uiSliderExtra::Setup("Stereo offset").withedit(true),
	   		"Offset slider" );
    sliderfld->sldr()->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );

    preFinalise().notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    float offset = vwrs[0]->getStereoOffset();
    int minval = (int)(offset - 1000); minval /= 100; minval *= 100;
    int maxval = (int)(offset + 1000); maxval /= 100; maxval *= 100;
    if ( minval < 10 ) minval = 10;

    sliderfld->sldr()->setMinValue( minval );
    sliderfld->sldr()->setMaxValue( maxval );
    sliderfld->sldr()->setValue( offset );
}


bool uiStereoDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    float slval = sliderfld->sldr()->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
	vwrs[idx]->setStereoOffset( slval );

    if ( mIsEqual(sliderfld->sldr()->maxValue(),slval,mDefEps) )
	uiMSG().message( "Open this dialog again for higher offsets" );

    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    float slval = sliderfld->sldr()->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
        vwrs[idx]->setStereoOffset( slval );
}
