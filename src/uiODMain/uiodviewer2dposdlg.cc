/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki
Date:	       March 2015
________________________________________________________________________

-*/


#include "uiodviewer2dposdlg.h"

#include "uibutton.h"
#include "uiflatviewstdcontrol.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uiodviewer2dposgrp.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "probemanager.h"
#include "attribprobelayer.h"
#include "uistrings.h"


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

    uiPushButton* zoomlevelbut = new uiPushButton( this,
			m3Dots(uiStrings::sAdvanced()),
			mCB(this,uiODViewer2DPosDlg,zoomLevelCB), true );
    zoomlevelbut->attach( alignedBelow, posgrp_ );
}


void uiODViewer2DPosDlg::zoomLevelCB( CallBacker* )
{
    uiFlatViewZoomLevelDlg zoomlvldlg(
	    this, initialx1pospercm_, initialx2pospercm_, true );
    zoomlvldlg.go();
}


bool uiODViewer2DPosDlg::acceptOK()
{
    if ( !posgrp_->commitSel( true ) )
	return false;

    IOPar seldatapar;
    posgrp_->fillPar( seldatapar );
    Viewer2DPosDataSel posdatasel;
    posdatasel.usePar( seldatapar );
    Probe* newprobe = posdatasel.createNewProbe();
    if ( !newprobe )
	return false;

    SilentTaskRunnerProvider trprov;
    if ( !ProbeMGR().store(*newprobe,trprov).isOK() )
       return false;

    uiODViewer2D::DispSetup su;
    su.initialx1pospercm_ = initialx1pospercm_;
    su.initialx2pospercm_ = initialx2pospercm_;
    odappl_.viewer2DMgr().displayIn2DViewer(
	    *newprobe, newprobe->getLayerByIdx(0)->getID(), su );
    return true;
}
