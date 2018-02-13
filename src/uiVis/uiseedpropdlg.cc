/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiseedpropdlg.h"

#include "uimarkerstyle.h"
#include "emobject.h"


uiSeedPropDlg::uiSeedPropDlg( uiParent* p, EM::Object& emobj )
    : uiDialog(p,Setup(tr("Seed properties"),mNoDlgTitle,mTODOHelpKey))
    , emobject_(emobj)
    , markerstyle_(emobject_.getPosAttrMarkerStyle(EM::Object::sSeedNode()))
{
    TypeSet<OD::MarkerStyle3D::Type> excl;
    excl.add( OD::MarkerStyle3D::None );
    stylefld_ = new uiMarkerStyle3D( this, uiMarkerStyle::Setup(), &excl );
    stylefld_->setMarkerStyle( markerstyle_ );
    stylefld_->change.notify( mCB(this,uiSeedPropDlg,styleSel) );
}


void uiSeedPropDlg::styleSel( CallBacker* )
{
    OD::MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    if ( markerstyle_ == style )
	return;

    markerstyle_ = style;
    emobject_.setPosAttrMarkerStyle( EM::Object::sSeedNode(), markerstyle_ );
}
