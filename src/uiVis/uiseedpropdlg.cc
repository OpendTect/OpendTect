/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseedpropdlg.cc,v 1.5 2009-01-09 10:58:54 cvsranojay Exp $";

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
    , markerstyle_( emobject_->getPosAttrMarkerStyle(EM::EMObject::sSeedNode()) )
{
}


void uiSeedPropDlg::doFinalise( CallBacker* )
{
    sliderfld->sldr()->setValue( markerstyle_.size_ );
    colselfld->setColor( markerstyle_.color_ );
    typefld->setValue( markerstyle_.type_ - MarkerStyle3D::None );
}


void uiSeedPropDlg::sliderMove( CallBacker* )
{
    const float sldrval = sliderfld->sldr()->getValue();
    const int newsize = mNINT(sldrval);
    if ( markerstyle_.size_ == newsize ) 
	return;
    markerstyle_.size_ = newsize;
    updateMarkerStyle();
}


void uiSeedPropDlg::typeSel( CallBacker* )
{
    const MarkerStyle3D::Type newtype = 
	(MarkerStyle3D::Type) (MarkerStyle3D::None + typefld->getIntValue());
    if ( markerstyle_.type_ == newtype ) 
	return;
    markerstyle_.type_ = newtype;
    updateMarkerStyle();
}


void uiSeedPropDlg::colSel( CallBacker* )
{
    const Color newcolor = colselfld->color();
    if ( markerstyle_.color_ == newcolor )
    	return;
    markerstyle_.color_ = newcolor;
    updateMarkerStyle();
}


void uiSeedPropDlg::updateMarkerStyle()
{
    emobject_->setPosAttrMarkerStyle( EM::EMObject::sSeedNode(), markerstyle_ );
}
