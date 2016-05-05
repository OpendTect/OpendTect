/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "picksetmgr.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimarkerstyle.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "vistristripset.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set,
			      visSurvey::PickSetDisplay* psd )
    : uiMarkerStyleDlg( p, tr("PickSet Display Properties") )
    , set_( set )
    , psd_( psd )
{
    setTitleText( uiString::emptyString() );
    usedrawstylefld_ = new uiCheckBox( this, tr("Connect picks") );
    const bool hasbody = psd && psd->isBodyDisplayed();
    const bool hassty = hasbody
		     || set_.connection()==Pick::Set::Disp::Close;
    usedrawstylefld_->setChecked( hassty );
    usedrawstylefld_->activated.notify( mCB(this,uiPickPropDlg,drawSel) );

    drawstylefld_ = new uiGenInput( this, tr("with"),
				   BoolInpSpec(true,tr("Line"),tr("Surface")) );
    drawstylefld_->setValue( !hasbody );
    drawstylefld_->valuechanged.notify( mCB(this,uiPickPropDlg,drawStyleCB) );
    drawstylefld_->attach( rightOf, usedrawstylefld_ );

    stylefld_->attach( alignedBelow, usedrawstylefld_ );

    drawSel( 0 );
}


void uiPickPropDlg::drawSel( CallBacker* )
{
    const bool usestyle = usedrawstylefld_->isChecked();
    drawstylefld_->display( usestyle );

    if ( !usestyle )
    {
	if ( set_.connection() == Pick::Set::Disp::Close )
	{
	    set_.setConnection( Pick::Set::Disp::None );
	    Pick::Mgr().reportDispChange( this, set_ );
	}

	if ( psd_ )
	    psd_->displayBody( false );
    }
    else
	drawStyleCB( 0 );
}


void uiPickPropDlg::drawStyleCB( CallBacker* )
{
    const bool showline = drawstylefld_->getBoolValue();
    if ( psd_ )
	psd_->displayBody( !showline );

    if ( showline )
    {
	set_.setConnection( Pick::Set::Disp::Close );
	Pick::Mgr().reportDispChange( this, set_ );
    }
    else
    {
	if ( !psd_ ) return;
	set_.setConnection( Pick::Set::Disp::None );
	Pick::Mgr().reportDispChange( this, set_ );

	if ( !psd_->getDisplayBody() )
	    psd_->setBodyDisplay();
    }
}


void uiPickPropDlg::doFinalise( CallBacker* )
{
    stylefld_->setMarkerStyle( set_.markerStyle() );
}


void uiPickPropDlg::sliderMove( CallBacker* cb )
{
    typeSel( cb );
}


void uiPickPropDlg::typeSel( CallBacker* )
{
    OD::MarkerStyle3D style( set_.markerStyle() );
    stylefld_->getMarkerStyle( style );
    set_.setMarkerStyle( style );
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::colSel( CallBacker* cb )
{
    typeSel( cb );
}


bool uiPickPropDlg::acceptOK( CallBacker* )
{
    return true;
}
