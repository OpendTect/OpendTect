/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    preFinalize().notify( mCB(this,uiStereoDlg,doFinalize) );
}


void uiStereoDlg::doFinalize( CallBacker* )
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
