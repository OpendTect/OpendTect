/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipickpropdlg.cc,v 1.14 2009-07-22 16:01:43 cvsbert Exp $";

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
    const bool hasbody = psd && psd->isBodyDisplayed();
    const bool hassty = set_.disp_.connect_==Pick::Set::Disp::Close || hasbody;
    usedrawstylefld_->setChecked( hassty );
    usedrawstylefld_->activated.notify( mCB(this,uiPickPropDlg,drawSel) );

    drawstylefld_ = new uiGenInput( this, "with", 
	    			    BoolInpSpec( true, "Line", "Surface" ) );
    drawstylefld_->setValue( hassty && !hasbody );
    drawstylefld_->valuechanged.notify( mCB(this,uiPickPropDlg,drawStyleCB) );
    drawstylefld_->attach( rightOf, usedrawstylefld_ );
    
    typefld->attach( alignedBelow, usedrawstylefld_ );
    typefld->setName( "Shape" );
    sliderfld->label()->setText( "Size" );
    colselfld->setLblText( "Color" );

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
    	    psd_->setBodyDisplay();
    	    needtriangulate_ = false;
    	}
    }
}


void uiPickPropDlg::doFinalise( CallBacker* )
{
    sliderfld->sldr()->setValue( set_.disp_.pixsize_ );
    colselfld->setColor( set_.disp_.color_ );
    typefld->setValue( set_.disp_.markertype_ );
}


void uiPickPropDlg::sliderMove( CallBacker* )
{
    const float sldrval = sliderfld->sldr()->getValue();
    set_.disp_.pixsize_ = mNINT(sldrval);
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::typeSel( CallBacker* )
{
    set_.disp_.markertype_ = typefld->getIntValue();//Didn't inlude None -1
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
