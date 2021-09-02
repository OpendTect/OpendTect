/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uistereodlg.h"

#include "uimsg.h"
#include "uislider.h"
#include "ui3dviewer.h"
#include "od_helpids.h"


uiStereoDlg::uiStereoDlg( uiParent* p, ObjectSet<ui3DViewer>& vwrs )
	: uiDialog(p, uiDialog::Setup(tr("Stereo viewing"),
				      tr("Set stereo offset"),
                                      mODHelpKey(mStereoDlgHelpID) )
		      .canceltext(uiString::emptyString()))
	, vwrs_(vwrs)
{
    sliderfld_ = new uiSlider( this,
			uiSlider::Setup(tr("Stereo offset")).withedit(true),
			"Offset slider" );
    sliderfld_->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );
    sliderfld_->setScale( 0.1f, 0.0f );
    sliderfld_->setInterval( StepInterval<float>(0.0f,10.f,0.1f) );
    preFinalise().notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    const float offset = vwrs_[0]->getStereoOffset();
    sliderfld_->setValue( offset );
}

bool uiStereoDlg::acceptOK( CallBacker* )
{
    const float slval = sliderfld_->getFValue();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setStereoOffset( slval );

    if ( mIsEqual(sliderfld_->maxValue(),slval,mDefEps) )
	uiMSG().message( tr("Open this dialog again for higher offsets") );

    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    const float slval = sliderfld_->getFValue();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
        vwrs_[idx]->setStereoOffset( slval );
}
