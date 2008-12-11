/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipickpropdlg.cc,v 1.8 2008-12-11 16:15:12 cvsyuancheng Exp $";

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "vistristripset.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set, 
			      visSurvey::PickSetDisplay* psd )
    : uiMarkerStyleDlg( p, "Pick properties" )
    , set_( set )
    , psd_( psd )
    , needtriangulate_( true )		 
{
    setTitleText( "Specity picks style" );
    usedrawstylefld_ = new uiCheckBox( this, "Connect picks" );
    const bool hasstyle = set_.disp_.connect_==Pick::Set::Disp::Close || 
			  psd && psd->isLocationBodyDisplayed();
    usedrawstylefld_->setChecked( hasstyle );
    usedrawstylefld_->activated.notify( mCB(this,uiPickPropDlg,drawSel) );

    drawstylefld_ = new uiGenInput( this, "Connecting type", 
	    BoolInpSpec( true, "Line", "Surface" ) );
    drawstylefld_->setValue( psd ? !psd->isLocationBodyDisplayed() : 1 );
    drawstylefld_->valuechanged.notify( mCB(this,uiPickPropDlg,drawStyleCB) );
    drawstylefld_->attach( alignedBelow, usedrawstylefld_ );
    
    typefld->attach( alignedBelow, drawstylefld_ );
    typefld->setName( "Marker shape" );
    sliderfld->label()->setText( "Marker size" );
    colselfld->setLblText( "Marker color" );

    drawSel( 0 );
}


void uiPickPropDlg::drawSel( CallBacker* )
{
    const bool usestyle = usedrawstylefld_->isChecked();
    drawstylefld_->display( usestyle );

    if ( !usestyle )
    {
	set_.disp_.connect_ = Pick::Set::Disp::None;
    	Pick::Mgr().reportDispChange( this, set_ );
	if ( psd_ )
	    psd_->displayLocationBody( false );
    }
    else
	drawStyleCB( 0 );
}


void uiPickPropDlg::drawStyleCB( CallBacker* cb )
{
    const bool showline = drawstylefld_->getBoolValue();
    if ( showline )
    {
	set_.disp_.connect_ = Pick::Set::Disp::Close;
    	Pick::Mgr().reportDispChange( this, set_ );
    }
    else
    {
     	if ( !psd_ ) return;
    	set_.disp_.connect_ = Pick::Set::Disp::None;
    	Pick::Mgr().reportDispChange( this, set_ );
	
    	if ( needtriangulate_ )
    	{
    	    psd_->setLocationBodyDisplay();
    	    needtriangulate_ = false;
    	}
    }

    if ( psd_ )
    	psd_->displayLocationBody( !showline );
}


void uiPickPropDlg::doFinalise( CallBacker* )
{
    sliderfld->sldr()->setValue( set_.disp_.pixsize_ );
    colselfld->setColor( set_.disp_.color_ );
    typefld->setValue( set_.disp_.markertype_ + 1 );
}


void uiPickPropDlg::sliderMove( CallBacker* )
{
    const float sldrval = sliderfld->sldr()->getValue();
    set_.disp_.pixsize_ = mNINT(sldrval);
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::typeSel( CallBacker* )
{
    set_.disp_.markertype_ = typefld->getIntValue() - 1;
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::colSel( CallBacker* )
{
    set_.disp_.color_ = colselfld->color();
    Pick::Mgr().reportDispChange( this, set_ );
}


bool uiPickPropDlg::acceptOK( CallBacker* )
{
    return true;
}
