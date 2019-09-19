/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimarkerstyledlg.h"

#include "uimarkerstyle.h"
#include "draw.h"


uiMarkerStyleDlg::uiMarkerStyleDlg( uiParent* p, const uiString& title )
	: uiDialog(p,
		   uiDialog::Setup(title,tr("Specify marker style properties"),
				   mNoHelpKey)
		   .canceltext(uiString::emptyString()))
{
    MarkerStyle3D::Type excludedtypes[] =
				{ MarkerStyle3D::None };
    stylefld_ = new uiMarkerStyle3D( this, true, Interval<int>( 1, 15 ),
	    2, excludedtypes );
    mAttachCB( stylefld_->typeSel(), uiMarkerStyleDlg::typeSel );
    mAttachCB( stylefld_->checkSel(), uiMarkerStyleDlg::typeSel );
    mAttachCB( stylefld_->sliderMove(), uiMarkerStyleDlg::sliderMove );
    mAttachCB( stylefld_->colSel(), uiMarkerStyleDlg::colSel );

    preFinalise().notify( mCB(this,uiMarkerStyleDlg,doFinalise) );
}


bool uiMarkerStyleDlg::acceptOK( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style ); //just to process text input on fld
    return true;
}
