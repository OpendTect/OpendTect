/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki
Date:	       March 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiodviewer2dposdlg.h"

#include "uiattribpartserv.h"
#include "uiodviewer2dmgr.h"
#include "uiodviewer2dposgrp.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uitaskrunner.h"

#include "datapack.h"
#include "randomlinegeom.h"


uiODViewer2DPosDlg::uiODViewer2DPosDlg( uiODMain& appl )
    : uiDialog(&appl,uiDialog::Setup(tr("2D Viewer Launcher"),
				     tr("Select Position and Data"),
				     mODHelpKey(mODViewer2DPosDlgHelpID)))
    , odappl_(appl)
{
    posgrp_  = new uiODViewer2DPosGrp( this, new Viewer2DPosDataSel(), false );
}


bool uiODViewer2DPosDlg::acceptOK( CallBacker* )
{
    if ( !posgrp_->commitSel( true ) )
	return false;

    IOPar seldatapar;
    posgrp_->fillPar( seldatapar );
    Viewer2DPosDataSel posdatasel;
    posdatasel.usePar( seldatapar );
    DataPack::ID vwr2ddpid = DataPack::cNoID();
    uiAttribPartServer* attrserv = odappl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( posdatasel.selspec_ );
    const bool isrl = !posdatasel.rdmlineid_.isUdf();
    if ( isrl )
    {
	TypeSet<BinID> knots, path;
	Geometry::RandomLineSet::getGeometry(
		posdatasel.rdmlineid_, knots, &posdatasel.tkzs_.zsamp_ );
	Geometry::RandomLine::getPathBids( knots, path );
	vwr2ddpid = attrserv->createRdmTrcsOutput(
				posdatasel.tkzs_.zsamp_, &path, &knots );
    }
    else if ( posdatasel.geomid_ != Survey::GeometryManager::cUndefGeomID() )
    {
	uiTaskRunner taskrunner( this );
	vwr2ddpid =
	    attrserv->create2DOutput( posdatasel.tkzs_, posdatasel.geomid_,
				      taskrunner );
    }
    else
    {
	vwr2ddpid =
	    attrserv->createOutput( posdatasel.tkzs_, DataPack::cNoID() );
    }

    odappl_.viewer2DMgr().displayIn2DViewer( vwr2ddpid, posdatasel.selspec_,
					     false );
    return true;
}
