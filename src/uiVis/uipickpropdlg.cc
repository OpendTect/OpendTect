/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.cc,v 1.2 2006-08-09 10:04:41 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"


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
