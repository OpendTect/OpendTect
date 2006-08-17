/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseedpropdlg.cc,v 1.1 2006-08-17 14:08:28 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiseedpropdlg.h"

//#include "color.h"
//#include "draw.h"
//#include "pickset.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"


uiSeedPropDlg::uiSeedPropDlg( uiParent* p, EM::EMObject* emobj )
    : uiMarkerStyleDlg( p, "Seed properties" )
    , emobject_( emobj )
    , markerstyle_( emobject_->getPosAttrMarkerStyle(EM::EMObject::sSeedNode) )
{}


void uiSeedPropDlg::doFinalise( CallBacker* )
{
    sliderfld->sldr()->setValue( markerstyle_.size );
    colselfld->setColor( markerstyle_.color );
    typefld->setValue( markerstyle_.type );
}


void uiSeedPropDlg::sliderMove( CallBacker* )
{
    const float sldrval = sliderfld->sldr()->getValue();
    const int newsize = mNINT(sldrval);
    if ( markerstyle_.size == newsize ) 
	return;
    markerstyle_.size = newsize;
    updateMarkerStyle();
}


void uiSeedPropDlg::typeSel( CallBacker* )
{
    const MarkerStyle3D::Type newtype = 
			      (MarkerStyle3D::Type) typefld->getIntValue();
    if ( markerstyle_.type == newtype ) 
	return;
    markerstyle_.type = newtype;
    updateMarkerStyle();
}


void uiSeedPropDlg::colSel( CallBacker* )
{
    const Color newcolor = colselfld->color();
    if ( markerstyle_.color == newcolor )
    	return;
    markerstyle_.color = newcolor;
    updateMarkerStyle();
}


void uiSeedPropDlg::updateMarkerStyle()
{
    emobject_->setPosAttrMarkerStyle( EM::EMObject::sSeedNode, markerstyle_ );
}
