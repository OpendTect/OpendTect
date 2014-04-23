/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistereodlg.h"

#include "uimsg.h"
#include "uislider.h"
#include "ui3dviewer.h"
#include "od_helpids.h"


uiStereoDlg::uiStereoDlg( uiParent* p, ObjectSet<ui3DViewer>& vwrs_ )
	: uiDialog(p, uiDialog::Setup("Stereo viewing",
				      "Set stereo offset",
                                      mODHelpKey(mStereoDlgHelpID) )
		      .canceltext(""))
	, vwrs(vwrs_)
{
    sliderfld = new uiSlider( this,
			uiSlider::Setup("Stereo offset").withedit(true),
			"Offset slider" );
    sliderfld->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );

    preFinalise().notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    float offset = vwrs[0]->getStereoOffset();
    int minval = (int)(offset - 1000); minval /= 100; minval *= 100;
    int maxval = (int)(offset + 1000); maxval /= 100; maxval *= 100;
    if ( minval < 10 ) minval = 10;

    sliderfld->setMinValue( (float) minval );
    sliderfld->setMaxValue( (float) maxval );
    sliderfld->setValue( offset );
}


bool uiStereoDlg::acceptOK( CallBacker* )
{
    float slval = sliderfld->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
	vwrs[idx]->setStereoOffset( slval );

    if ( mIsEqual(sliderfld->maxValue(),slval,mDefEps) )
	uiMSG().message( "Open this dialog again for higher offsets" );

    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    float slval = sliderfld->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
        vwrs[idx]->setStereoOffset( slval );
}
