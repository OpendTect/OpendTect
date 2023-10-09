/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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


uiODViewer2DPosDlg::~uiODViewer2DPosDlg()
{}


void uiODViewer2DPosDlg::zoomLevelCB( CallBacker* )
{
    const Viewer2DPosDataSel& pds = posgrp_->posDataSel();
    const TrcKeyZSampling tkzs = pds.tkzs_;
    uiFlatViewZoomLevelDlg zoomlvldlg( this,
		pds.tkzs_.hsamp_.start_.inl(), pds.tkzs_.hsamp_.start_.crl(),
		initialx1pospercm_, initialx2pospercm_, true );
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
	 !posdatasel.rdmlineid_.isValid() )
    {
	uiMSG().error( tr("Selected RandomLine is not valid.") );
	return false;
    }

    odappl_.viewer2DMgr().displayIn2DViewer( posdatasel,
					     FlatView::Viewer::VD,
					     initialx1pospercm_,
	   				     initialx2pospercm_ );
    return true;
}
