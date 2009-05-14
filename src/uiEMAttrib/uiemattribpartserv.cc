/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiemattribpartserv.cc,v 1.9 2009-05-14 09:05:51 cvssatyaki Exp $";


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uihorizonshiftdlg.h"
#include "uiimphorizon2d.h"
#include "uiimpfaultstickset2d.h"
#include "uiseiseventsnapper.h"

#include "datapointset.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "typeset.h"


uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , nlamodel_(0)
    , descset_(0)
    , horshiftdlg_(0)
    , shiftrg_(-100,100,10)
    , shiftidx_(10)
    , attribidx_(0)
{}


void uiEMAttribPartServer::createHorizonOutput( HorOutType type )
{
    if ( !descset_ ) return;

    if ( type==OnHor )
    {
	uiAttrSurfaceOut dlg( parent(), *descset_, nlamodel_, nlaid_ );
	dlg.go();
    }
    else
    {
	uiAttrTrcSelOut dlg( parent(), *descset_, nlamodel_, nlaid_,
			     type==AroundHor );
	dlg.go();
    }
}


void uiEMAttribPartServer::snapHorizon( const EM::ObjectID& emid )
{
    IOObj* ioobj = IOM().get( EM::EMM().getMultiID(emid) );
    uiSeisEventSnapper dlg( parent(), ioobj );
    dlg.go();
    delete ioobj;
}


void uiEMAttribPartServer::import2DHorizon() const
{
    uiImportHorizon2D dlg( parent() );
    dlg.go();
}


float uiEMAttribPartServer::getShift() const
{
    if ( horshiftdlg_ )
	return horshiftdlg_->curShift();
    return shiftrg_.atIndex( shiftidx_ );
}


void uiEMAttribPartServer::showHorShiftDlg( uiParent* p,const EM::ObjectID& id,
       					    const TypeSet<int>& attribids )
{
    setAttribIdx( -1 );
    horshiftdlg_ = new uiHorizonShiftDialog( p, id, *descset_ );
    horshiftdlg_->setAttribIds( attribids );
    sendEvent( uiEMAttribPartServer::evShiftDlgOpened() );
    horshiftdlg_->calcAttribPushed.notify(
	    mCB(this,uiEMAttribPartServer,calcDPS) );
    horshiftdlg_->horShifted.notify(
	    mCB(this,uiEMAttribPartServer,horShifted) );
    horshiftdlg_->windowClosed.notify(
	    mCB(this,uiEMAttribPartServer,shiftDlgClosed));
    horshiftdlg_->go();
    horshiftdlg_->setDeleteOnClose( true );
}


void uiEMAttribPartServer::shiftDlgClosed( CallBacker* cb )
{
    if ( horshiftdlg_->uiResult()==1 )
    {
	if ( horshiftdlg_->doStore() )
	{
	    shiftattrbasename_ = horshiftdlg_->getAttribName();
	    sendEvent( uiEMAttribPartServer::evStoreShiftHorizons() );
	}
	else
	    sendEvent( uiEMAttribPartServer::evShiftDlgFinalised() );
    }
    sendEvent( uiEMAttribPartServer::evShiftDlgClosed() );
}


void uiEMAttribPartServer::calcDPS( CallBacker* cb )
{
    shiftattrbasename_ = horshiftdlg_->getAttribName();
    shiftrg_ = horshiftdlg_->shiftIntv();
    shiftidx_ = horshiftdlg_->curShiftIdx();
    setAttribID( horshiftdlg_->attribID() );
    sendEvent( uiEMAttribPartServer::evCalcShiftAttribute() );
}


void uiEMAttribPartServer::fillHorShiftDPS( ObjectSet<DataPointSet>& dpsset )
{
    const EM::Horizon3DGeometry& hor3dgeom =
		horshiftdlg_->horizon3D().geometry();
    const EM::SectionID sid = hor3dgeom.sectionID(0);
    const StepInterval<float> intv = horshiftdlg_->shiftIntv();
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    for ( int idx=0; idx<=intv.nrSteps(); idx++ )
    {
	const float shift = intv.atIndex(idx) / SI().zFactor();
	TypeSet<DataPointSet::DataRow> drset;
	BufferStringSet nmset;
	DataPointSet* dps = new DataPointSet( drset, nmset, false, true );
	dps->bivSet().setNrVals( 1 );
	getDataPointSet( horshiftdlg_->emID(), sid, *dps, shift );
	dpsset += dps;
    }
}


int uiEMAttribPartServer::textureIdx() const
{ return horshiftdlg_->curShiftIdx(); }


void uiEMAttribPartServer::setAttribIdx( int idx )
{
    attribidx_ = idx;
    
    if ( idx <0 || !&horshiftdlg_->attribIds() )
	return;

    horshiftdlg_->attribIds() += idx;
}


const TypeSet<int>& uiEMAttribPartServer::attribIds() const
{ return horshiftdlg_->attribIds(); }


void uiEMAttribPartServer::horShifted( CallBacker* cb )
{
    shiftrg_ = horshiftdlg_->shiftIntv();
    shiftidx_ = horshiftdlg_->curShiftIdx();
    setAttribID( horshiftdlg_->attribID() );
    sendEvent( uiEMAttribPartServer::evHorizonShift() );
}


void uiEMAttribPartServer::getDataPointSet( const EM::ObjectID& emid,
					    const EM::SectionID& sid,
				      	    DataPointSet& dps, float shift)
{
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, emobj )
    BinIDValueSet& bidvalset = dps.bivSet();
    const EM::Horizon3DGeometry& hor3dgeom = hor3d->geometry();
    const int nrknots = hor3dgeom.sectionGeometry(sid)->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid =
	hor3dgeom.sectionGeometry(sid)->getKnotRowCol(idx);
	Coord3 coord =
	hor3dgeom.sectionGeometry( sid )->getKnot( bid, false );
	TypeSet<float> zvalues;
	bidvalset.add( bid, coord.z + shift );
    }
    dps.dataChanged();
}


void uiEMAttribPartServer::import2DFaultStickset( const char* type )
{
    uiImportFaultStickSet2D fssdlg( parent(), type );
}
