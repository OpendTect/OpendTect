/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattribtransdlg.h"

#include "uislider.h"
#include "vissurvobj.h"

uiAttribTransDlg::uiAttribTransDlg( uiParent* p, visSurvey::SurveyObject& so,
				    int attrib )
    : uiDialog( p, uiDialog::Setup(tr("Attribute transparency"),
		mNoDlgTitle,mNoHelpKey) )
    , so_( so )
    , attrib_( attrib )
    , initaltrans_( so.getAttribTransparency(attrib) )
{
    uiSlider::Setup ss( uiStrings::sTransparency() ); ss.withedit(true);
    slider_ = new uiSlider( this, ss, "Transparency slider" );
    slider_->setMinValue( 0 );
    slider_->setMaxValue( 100 );
    slider_->setStep( 1 );
    slider_->setValue( 100*initaltrans_/255.f );

    slider_->valueChanged.notify( mCB(this,uiAttribTransDlg,changeCB) );
}


uiAttribTransDlg::~uiAttribTransDlg()
{}


void uiAttribTransDlg::changeCB( CallBacker* )
{
    const int val = 255*slider_->getIntValue()/100;
    so_.setAttribTransparency( attrib_, mCast(unsigned char,val) );
}


bool uiAttribTransDlg::rejectOK( CallBacker* )
{
    so_.setAttribTransparency( attrib_, initaltrans_ );
    return true;
}

bool uiAttribTransDlg::acceptOK( CallBacker* )
{
    return true;
}
