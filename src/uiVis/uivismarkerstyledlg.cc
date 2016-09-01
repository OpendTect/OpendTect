/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uivismarkerstyledlg.h"

#include "uimarkerstyle.h"
#include "draw.h"


uiVisMarkerStyleDlg::uiVisMarkerStyleDlg( uiParent* p, const uiString& title )
	: uiDialog(p,
		   uiDialog::Setup(title,tr("Specify marker style properties"),
				   mNoHelpKey)
		   .canceltext(uiString::emptyString()))
{
    TypeSet<OD::MarkerStyle3D::Type> excludedtypes;
    excludedtypes.add( OD::MarkerStyle3D::None )
		 .add( OD::MarkerStyle3D::Point );
    stylefld_ = new uiMarkerStyle3D( this, true, Interval<int>( 1,
				uiMarkerStyle3D::cDefMaxMarkerSize() ),
				&excludedtypes );
    stylefld_->typeSel()->notify( mCB(this,uiVisMarkerStyleDlg,typeSel) );
    stylefld_->sizeChange()->notify( mCB(this,uiVisMarkerStyleDlg,sizeChg));
    stylefld_->colSel()->notify( mCB(this,uiVisMarkerStyleDlg,colSel) );

    preFinalise().notify( mCB(this,uiVisMarkerStyleDlg,doFinalise) );
}


bool uiVisMarkerStyleDlg::acceptOK()
{
    /* Bert: Can someone at least TELL me why this is necessary??
    OD::MarkerStyle3D style;
    stylefld_->getMarkerStyle( style ); //just to process text input on fld
    */
    return true;
}
