/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki
Date:	       March 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiodviewer2dposdlg.h"

#include "uibutton.h"
#include "uiflatviewstdcontrol.h"
#include "uimsg.h"
#include "uiodviewer2dmgr.h"
#include "uiodviewer2dposgrp.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"


uiODViewer2DPosDlg::uiODViewer2DPosDlg( uiODMain& appl )
    : uiDialog(&appl,uiDialog::Setup(tr("2D Viewer Launcher"),
				     tr("Select Position and Data"),
				     mODHelpKey(mODViewer2DPosDlgHelpID)))
    , odappl_(appl)
    , initialx1pospercm_(mUdf(float))
    , initialx2pospercm_(mUdf(float))
{
    uiFlatViewStdControl::getGlobalZoomLevel(
	    initialx1pospercm_, initialx2pospercm_, true );

    posgrp_  = new uiODViewer2DPosGrp( this, new Viewer2DPosDataSel(), false );

    uiPushButton* zoomlevelbut = new uiPushButton( this, m3Dots(tr("Advanced")),
	    		mCB(this,uiODViewer2DPosDlg,zoomLevelCB), true );
    zoomlevelbut->attach( alignedBelow, posgrp_ );
}


void uiODViewer2DPosDlg::zoomLevelCB( CallBacker* )
{
    uiFlatViewZoomLevelDlg zoomlvldlg(
	    this, initialx1pospercm_, initialx2pospercm_, true );
    zoomlvldlg.go();
}


bool uiODViewer2DPosDlg::acceptOK( CallBacker* )
{
    if ( !posgrp_->commitSel( true ) )
	return false;

    IOPar seldatapar;
    posgrp_->fillPar( seldatapar );
    Viewer2DPosDataSel posdatasel;
    posdatasel.usePar( seldatapar );

    if ( posdatasel.postype_ == Viewer2DPosDataSel::RdmLine &&
	 posdatasel.rdmlineid_==-1 )
    {
	uiMSG().error( tr("Selected RandomLine is not valid.") );
	return false;
    }

    odappl_.viewer2DMgr().displayIn2DViewer( posdatasel,
					     false, initialx1pospercm_,
	   				     initialx2pospercm_ );
    return true;
}
