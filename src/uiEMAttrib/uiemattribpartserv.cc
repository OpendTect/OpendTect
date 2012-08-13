/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiemattribpartserv.cc,v 1.25 2012-08-13 09:36:57 cvsaneesh Exp $";


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uihorizonshiftdlg.h"
#include "uihorsavefieldgrp.h"
#include "uiimpfaultstickset2d.h"
#include "uiimphorizon2d.h"
#include "uiseiseventsnapper.h"

#include "datapointset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"
#include "posvecdataset.h"
#include "typeset.h"

static const DataColDef	siddef_( "Section ID" );

const DataColDef& uiEMAttribPartServer::sidDef() const
{ return siddef_; }

uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , nlamodel_(0)
    , descset_(0)
    , horshiftdlg_(0)
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


bool uiEMAttribPartServer::snapHorizon( const EM::ObjectID& emid, MultiID& mid,
       					bool& displaynew, bool is2d )
{
    IOObj* ioobj = IOM().get( EM::EMM().getMultiID(emid) );
    if ( !ioobj )
	return false;

    uiSeisEventSnapper dlg( parent(), ioobj, is2d );
    dlg.go();
    delete ioobj;

    displaynew = dlg.saveFldGrp()->displayNewHorizon() &&
		 dlg.saveFldGrp()->getNewHorizon();
    if ( displaynew )
	mid = dlg.saveFldGrp()->getNewHorizon()->multiID(); 

    return dlg.saveFldGrp()->overwriteHorizon();
}


void uiEMAttribPartServer::import2DHorizon() const
{
    uiImportHorizon2D dlg( parent() );
    dlg.go();
}


float uiEMAttribPartServer::getShift() const
{
    return horshiftdlg_ ? horshiftdlg_->getShift() : mUdf(float);
}


StepInterval<float> uiEMAttribPartServer::shiftRange() const
{
    StepInterval<float> res;
    if ( horshiftdlg_ )
	res = horshiftdlg_->shiftRg();

    return res;
}


void uiEMAttribPartServer::showHorShiftDlg( const EM::ObjectID& id,
					    const BoolTypeSet& attrenabled,
					    float initialshift,
					    bool canaddattrib)
{
    sendEvent( uiEMAttribPartServer::evShiftDlgOpened() );

    initialshift_ = initialshift;
    initialattribstatus_ = attrenabled;
    setAttribIdx( mUdf(int) );
    horshiftdlg_ = new uiHorizonShiftDialog( appserv().parent(), id, *descset_,
	    				     initialshift, canaddattrib );
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
	    sendEvent( uiEMAttribPartServer::evStoreShiftHorizons() );

	sendEvent( uiEMAttribPartServer::evShiftDlgClosedOK() );
    }
    else
	sendEvent( uiEMAttribPartServer::evShiftDlgClosedCancel() );
}


void uiEMAttribPartServer::calcDPS( CallBacker* cb )
{
    setAttribID( horshiftdlg_->attribID() );
    sendEvent( uiEMAttribPartServer::evCalcShiftAttribute() );
}


const char* uiEMAttribPartServer::getAttribBaseNm() const
{ return horshiftdlg_ ? horshiftdlg_->getAttribBaseName() : 0; }


void uiEMAttribPartServer::fillHorShiftDPS( ObjectSet<DataPointSet>& dpsset,
       					    TaskRunner* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    StepInterval<float> intv = horshiftdlg_->shiftRg();

    const int nrshifts = horshiftdlg_->nrSteps();
    const EM::Horizon3DGeometry& hor3dgeom =
	horshiftdlg_->horizon3D().geometry();
    for ( int idx=0; idx<nrshifts; idx++ )
    {
	TypeSet<DataPointSet::DataRow> drset;
	BufferStringSet nmset;
	DataPointSet* dps = new DataPointSet( drset, nmset, false, true );

	dps->dataSet().add( new DataColDef( siddef_ ) );
	dps->bivSet().setNrVals( dps->nrFixedCols() + 2 );
	dpsset += dps;
    }

    DataPointSet::DataRow datarow;
    datarow.data_.setSize( 1, mUdf(float) );
    for ( int sididx=0; sididx<hor3dgeom.nrSections(); sididx++ )
    {
	const EM::SectionID sid = hor3dgeom.sectionID(sididx);
	datarow.data_[0] = sid;
	const int nrknots = hor3dgeom.sectionGeometry(sid)->nrKnots();

	for ( int idx=0; idx<nrknots; idx++ )
	{
	    const BinID bid =
		    hor3dgeom.sectionGeometry(sid)->getKnotRowCol(idx);
	    const float realz = (float) (
		hor3dgeom.sectionGeometry(sid)->getKnot( bid, false ).z );
	    if ( mIsUdf(realz) )
		continue;

	    datarow.pos_.binid_ = bid;

	    for ( int shiftidx=0; shiftidx<nrshifts; shiftidx++ )
	    {
		const float shift = intv.atIndex(shiftidx);
		datarow.pos_.z_ = realz+shift;
		dpsset[shiftidx]->addRow( datarow );
	    }

	}
    }

    for ( int shiftidx=0; shiftidx<nrshifts; shiftidx++ )
	dpsset[shiftidx]->dataChanged();
}


int uiEMAttribPartServer::textureIdx() const
{ return horshiftdlg_->curShiftIdx(); }


void uiEMAttribPartServer::setAttribIdx( int idx )
{
    attribidx_ = idx;
}


void uiEMAttribPartServer::horShifted( CallBacker* cb )
{
    shiftidx_ = horshiftdlg_->curShiftIdx();
    sendEvent( uiEMAttribPartServer::evHorizonShift() );
}


void uiEMAttribPartServer::import2DFaultStickset( const char* type )
{
    uiImportFaultStickSet2D fssdlg( parent(), type );
    fssdlg.setModal( true );
    fssdlg.go();
}
