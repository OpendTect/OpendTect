/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"

#include "uimarkerstyle.h"
#include "draw.h"


uiMarkerStyleDlg::uiMarkerStyleDlg( uiParent* p, const uiString& title,
				    bool withnone )
	: uiDialog(p,uiDialog::Setup(title,mNoDlgTitle,mNoHelpKey)
		   .canceltext(uiString::emptyString()))
{
    if ( withnone )
	stylefld_ = new uiMarkerStyle3D( this, true, Interval<int>(1,15) );
    else
    {
	MarkerStyle3D::Type excludedtypes[] =
				{ MarkerStyle3D::None };
	stylefld_ = new uiMarkerStyle3D( this, true, Interval<int>( 1, 15 ),
					 1, excludedtypes );
    }

    mAttachCB( stylefld_->typeSel(), uiMarkerStyleDlg::typeSel );
    mAttachCB( stylefld_->sliderMove(), uiMarkerStyleDlg::sliderMove );
    mAttachCB( stylefld_->colSel(), uiMarkerStyleDlg::colSel );

    preFinalize().notify( mCB(this,uiMarkerStyleDlg,doFinalize) );
}


bool uiMarkerStyleDlg::acceptOK( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style ); //just to process text input on fld
    return true;
}
