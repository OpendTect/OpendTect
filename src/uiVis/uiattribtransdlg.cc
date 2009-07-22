/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribtransdlg.cc,v 1.7 2009-07-22 16:01:43 cvsbert Exp $";

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
    slider_ = new uiSliderExtra( this, uiSliderExtra::Setup("Transparency"),
	    			 "Transparency slider" );
    slider_->sldr()->setMinValue( 0 );
    slider_->sldr()->setMaxValue( 255 );
    slider_->sldr()->setStep( 1 );
    slider_->sldr()->setValue( initaltrans_ ); 

    slider_->sldr()->valueChanged.notify( mCB(this,uiAttribTransDlg,changeCB) );
}


void uiAttribTransDlg::changeCB( CallBacker* )
{
    so_.setAttribTransparency( attrib_, slider_->sldr()->getIntValue() );
}


bool uiAttribTransDlg::rejectOK( CallBacker* )
{
    so_.setAttribTransparency( attrib_, initaltrans_ );
    return true;
}

