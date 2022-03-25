/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiseedpropdlg.h"

#include "uimarkerstyle.h"


uiSeedPropDlg::uiSeedPropDlg( uiParent* p, EM::EMObject* emobj, int posattr )
    : uiMarkerStyleDlg( p, tr("Seed properties") )
    , emobject_( emobj )
    , posattr_(posattr)
    , markerstyle_(emobject_->getPosAttrMarkerStyle(posattr))
{
    stylefld_->setMarkerStyle( markerstyle_ );
}


void uiSeedPropDlg::doFinalize( CallBacker* )
{
    stylefld_->setMarkerStyle( markerstyle_ );
}


void uiSeedPropDlg::sliderMove( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    if ( markerstyle_==style )
	return;

    markerstyle_ = style;

    updateMarkerStyle();
}


void uiSeedPropDlg::typeSel( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    if ( markerstyle_==style )
	return;

    markerstyle_ = style;

    updateMarkerStyle();
}


void uiSeedPropDlg::colSel( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    if ( markerstyle_==style )
	return;

    markerstyle_ = style;

    updateMarkerStyle();
}


void uiSeedPropDlg::updateMarkerStyle()
{
    emobject_->setPosAttrMarkerStyle( posattr_, markerstyle_ );
}
