/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattribtransdlg.h"

#include "uislider.h"
#include "vissurvobj.h"

uiAttribTransDlg::uiAttribTransDlg( uiParent* p, visSurvey::SurveyObject& so,
				    int attrib )
    : uiDialog( p, uiDialog::Setup("Attribute transparency",0,mNoHelpID) )
    , so_( so )
    , attrib_( attrib )
    , initaltrans_( so.getAttribTransparency(attrib) )
{
    uiSliderExtra::Setup ss( "Transparency" ); ss.withedit(true);
    slider_ = new uiSliderExtra( this, ss, "Transparency slider" );
    slider_->sldr()->setMinValue( 0 );
    slider_->sldr()->setMaxValue( 100 );
    slider_->sldr()->setStep( 1 );
    slider_->sldr()->setValue( 100*initaltrans_/255.f ); 

    slider_->sldr()->valueChanged.notify( mCB(this,uiAttribTransDlg,changeCB) );
}


void uiAttribTransDlg::changeCB( CallBacker* )
{
    const int val = 255*slider_->sldr()->getIntValue()/100;
    so_.setAttribTransparency( attrib_, mCast(unsigned char,val) );
}


bool uiAttribTransDlg::rejectOK( CallBacker* )
{
    so_.setAttribTransparency( attrib_, initaltrans_ );
    return true;
}

bool uiAttribTransDlg::acceptOK( CallBacker* )
{
    slider_->processInput();
    return true;
}
