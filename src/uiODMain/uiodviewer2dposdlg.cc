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
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uitaskrunner.h"

#include "datapack.h"
#include "randomlinegeom.h"


uiODViewer2DPosDlg::uiODViewer2DPosDlg( uiODMain& appl )
    : uiDialog(&appl,uiDialog::Setup(tr("2D Viewer launcher"),
				     tr("Select position & data"),
				     mODHelpKey(mODViewer2DPosDlgHelpID)))
    , odappl_(appl)
{
    posgrp_  = new uiODViewer2DPosGrp( this, posdatasel_, true );
}


bool uiODViewer2DPosDlg::acceptOK( CallBacker* )
{
    if ( !posgrp_->acceptOK() )
	return false;
    DataPack::ID vwr2ddpid = DataPack::cNoID();
    uiAttribPartServer* attrserv = odappl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( posdatasel_.selspec_ );
    const bool isrl = !posdatasel_.rdmlineid_.isUdf();
    if ( isrl )
    {
	TypeSet<BinID> knots, path;
	posgrp_->getRdmLineGeom( knots, &posdatasel_.tkzs_.zsamp_ );
	Geometry::RandomLine::getPathBids( knots, path );
	vwr2ddpid = attrserv->createRdmTrcsOutput(
				posdatasel_.tkzs_.zsamp_, &path, &knots );
    }
    else if ( posdatasel_.geomid_ != Survey::GeometryManager::cUndefGeomID() )
    {
	uiTaskRunner taskrunner( this );
	vwr2ddpid =
	    attrserv->create2DOutput( posdatasel_.tkzs_, posdatasel_.geomid_,
				      taskrunner );
    }
    else
    {
	vwr2ddpid =
	    attrserv->createOutput( posdatasel_.tkzs_, DataPack::cNoID() );
    }

    odappl_.viewer2DMgr().displayIn2DViewer( vwr2ddpid, false );
    return true;
}
