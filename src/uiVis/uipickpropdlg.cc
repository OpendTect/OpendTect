/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.cc,v 1.1 2006-05-29 08:02:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uipickpropdlg.h"
#include "pickset.h"
#include "uislider.h"
#include "uigeninput.h"
#include "uicolor.h"
#include "color.h"
#include "draw.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& ps )
	: uiDialog(p,
		   uiDialog::Setup("Pick properties","Set display properties")
		   .canceltext(""))
	, set_(ps)
{
    typefld = new uiGenInput( this, "Type", 
	    		      StringListInpSpec(MarkerStyle3D::TypeNames) );
    typefld->valuechanged.notify( mCB(this,uiPickPropDlg,typeSel) );

    sliderfld = new uiSliderExtra( this, 
	    			   uiSliderExtra::Setup("Size").withedit(true));
    sliderfld->sldr()->setMinValue( 1 );
    sliderfld->sldr()->setMaxValue( 15 );
    sliderfld->sldr()->valueChanged.notify( mCB(this,uiPickPropDlg,sliderMove));
    sliderfld->attach( alignedBelow, typefld );

    colselfld = new uiColorInput( this, Color::White, "Color" );
    colselfld->attach( alignedBelow, sliderfld );
    colselfld->colorchanged.notify( mCB(this,uiPickPropDlg,colSel) );

    finaliseStart.notify( mCB(this,uiPickPropDlg,doFinalise) );
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
    set_.disp_.markertype_ = typefld->getIntValue();
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::colSel( CallBacker* )
{
    set_.disp_.color_ = colselfld->color();
    Pick::Mgr().reportDispChange( this, set_ );
}


bool uiPickPropDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    sliderMove(0);
    return true;
}
