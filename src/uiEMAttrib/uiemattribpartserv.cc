/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uicreate2dgrid.h"
#include "uiflatunflatcube.h"
#include "uihorizonshiftdlg.h"
#include "uihorsavefieldgrp.h"
#include "uiimphorizon2d.h"
#include "uiseiseventsnapper.h"
#include "uitaskrunner.h"

#include "attribdescset.h"
#include "datapointset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "hiddenparam.h"
#include "ioman.h"
#include "ioobj.h"
#include "paralleltask.h"
#include "posvecdataset.h"
#include "typeset.h"

static HiddenParam<uiEMAttribPartServer,uiFlatUnflatCube*> hp_flatdlg(nullptr);

static const DataColDef	siddef_( "Section ID" );

const DataColDef& uiEMAttribPartServer::sidDef() const
{ return siddef_; }

uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
    hp_flatdlg.setParam( this, nullptr );
}


uiEMAttribPartServer::~uiEMAttribPartServer()
{
    delete horshiftdlg_;
    delete aroundhor2ddlg_;
    delete aroundhor3ddlg_;
    delete betweenhor2ddlg_;
    delete betweenhor3ddlg_;
    delete surfattr2ddlg_;
    delete surfattr3ddlg_;
    delete crgriddlg_;

    hp_flatdlg.removeAndDeleteParam( this );
}


#define mEMAttrDlg(dlgobj,caption) \
    { \
	if ( !dlgobj ) \
	{ \
	    dlgobj = new uiAttrTrcSelOut( parent(), *descset_, nlamodel_, \
					  nlaid_, type==AroundHor ); \
	    dlgobj->setCaption( caption ); \
	} \
	else \
	    dlgobj->updateAttributes( *descset_,nlamodel_,nlaid_ ); \
	dlgobj->show(); \
    }

#define mSurfAttrDlg(surfdlg,surfdlgcap) \
    { \
	if ( !surfdlg ) \
	{ \
	    surfdlg = new uiAttrSurfaceOut( parent(), *descset_, \
						       nlamodel_, nlaid_ ); \
	    surfdlg->setCaption( surfdlgcap ); \
	} \
	else \
	    surfdlg->updateAttributes( *descset_,nlamodel_,nlaid_ ); \
	surfdlg->go(); \
    }


void uiEMAttribPartServer::createHorizonOutput( HorOutType type )
{
    createHorizonOutput( type, MultiID::udf() );
}


void uiEMAttribPartServer::createHorizonOutput( HorOutType type,
						const MultiID& key )
{
    if ( type==FlattenSingle )
    {
	auto* dlg = hp_flatdlg.getParam( this );
	if ( !dlg )
	{
	    dlg = new uiFlatUnflatCube( parent() );
	    dlg->setModal( false );
	    hp_flatdlg.setParam( this, dlg );
	}

	if ( !key.isUdf() )
	    dlg->setHorizon( key );

	dlg->show();
	return;
    }

    if ( !descset_ )
	return;

    if ( type==OnHor )
    {
	if ( descset_->is2D() )
	    mSurfAttrDlg(surfattr2ddlg_,tr("Calculate Horizon Data from 2D"))
	else
	    mSurfAttrDlg(surfattr3ddlg_,tr("Calculate Horizon Data from 3D"))
    }
    else
    {
	if ( descset_->is2D() )
	{
	    if ( type==AroundHor )
		mEMAttrDlg(aroundhor2ddlg_,
			   tr("Create Attribute Output Along a 2D Horizon"))
	    else
		mEMAttrDlg(betweenhor2ddlg_,
			   tr("Create Attribute Output Between 2D Horizons"))
	}
	else
	{
	    if ( type==AroundHor )
		mEMAttrDlg(aroundhor3ddlg_,
			   tr("Create Attribute Output Along a 3D Horizon"))
	    else
		mEMAttrDlg(betweenhor3ddlg_,
			   tr("Create Attribute Output Between 3D Horizons"))
	}
    }
}


void uiEMAttribPartServer::snapHorizon( const EM::ObjectID& emid, bool is2d )
{
    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid) );
    if ( !ioobj ) return;

    delete uiseisevsnapdlg_;
    uiseisevsnapdlg_ = new uiSeisEventSnapper( parent(), ioobj, is2d );
    uiseisevsnapdlg_->readyForDisplay.notify(
		mCB(this,uiEMAttribPartServer,readyForDisplayCB) );
    uiseisevsnapdlg_->show();
}


void uiEMAttribPartServer::import2DHorizon()
{
    if ( uiimphor2ddlg_ )
    {
	uiimphor2ddlg_->show();
	uiimphor2ddlg_->raise();
	return;
    }

    uiimphor2ddlg_ = new uiImportHorizon2D( parent() );
    uiimphor2ddlg_->readyForDisplay.notify(
		mCB(this,uiEMAttribPartServer,readyForDisplayCB) );
    uiimphor2ddlg_->show();
}


void uiEMAttribPartServer::create2DGrid( const Geometry::RandomLine* rdl )
{
    delete crgriddlg_;
    crgriddlg_ = new uiCreate2DGrid( parent(), rdl );
    crgriddlg_->setModal( false );
    crgriddlg_->show();
}


void uiEMAttribPartServer::readyForDisplayCB( CallBacker* cb )
{
    emobjids_.erase();
    if ( uiseisevsnapdlg_ && cb==uiseisevsnapdlg_ )
    {
	uiHorSaveFieldGrp* grp = uiseisevsnapdlg_->saveFldGrp();
	if ( !grp->getNewHorizon() )
	    return;

	emobjids_ += grp->getNewHorizon()->id();
    }
    else if ( uiimphor2ddlg_ && cb==uiimphor2ddlg_ )
	uiimphor2ddlg_->getEMObjIDs( emobjids_ );

    if ( !emobjids_.isEmpty() )
	sendEvent( evDisplayEMObject() );
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
					    int visid,
					    const BoolTypeSet& attrenabled,
					    float initialshift,
					    bool canaddattrib)
{
    initialshift_ = initialshift;
    initialattribstatus_ = attrenabled;
    setAttribIdx( mUdf(int) );

    if ( horshiftdlg_ )
	horshiftdlg_->close();

    horshiftdlg_ = new uiHorizonShiftDialog( appserv().parent(), id, visid,
					     *descset_, initialshift,
					     canaddattrib );
    horshiftdlg_->calcAttribPushed.notify(
	    mCB(this,uiEMAttribPartServer,calcDPS) );
    horshiftdlg_->horShifted.notify(
	    mCB(this,uiEMAttribPartServer,horShifted) );
    horshiftdlg_->windowClosed.notify(
	    mCB(this,uiEMAttribPartServer,shiftDlgClosed) );
    horshiftdlg_->setDeleteOnClose( true );
    horshiftdlg_->show();
}


void uiEMAttribPartServer::shiftDlgClosed( CallBacker* )
{
    if ( horshiftdlg_->uiResult()==1 )
    {
	if ( horshiftdlg_->doStore() )
	    sendEvent( uiEMAttribPartServer::evStoreShiftHorizons() );

	sendEvent( uiEMAttribPartServer::evShiftDlgClosedOK() );
    }
    else
	sendEvent( uiEMAttribPartServer::evShiftDlgClosedCancel() );

    horshiftdlg_->calcAttribPushed.remove(
	    mCB(this,uiEMAttribPartServer,calcDPS) );
    horshiftdlg_->horShifted.remove(
	    mCB(this,uiEMAttribPartServer,horShifted) );
    horshiftdlg_->windowClosed.remove(
	    mCB(this,uiEMAttribPartServer,shiftDlgClosed) );
    horshiftdlg_ = nullptr;
}


void uiEMAttribPartServer::calcDPS( CallBacker* )
{
    setAttribID( horshiftdlg_->attribID() );
    sendEvent( uiEMAttribPartServer::evCalcShiftAttribute() );
}


const char* uiEMAttribPartServer::getAttribBaseNm() const
{
    return horshiftdlg_ ? horshiftdlg_->getAttribBaseName() : nullptr;
}

mDefParallelCalc3Pars(HorShiftDPSFiller,
		     od_static_tr("HorShiftDPSFiller","Make datapointset"),
		     ObjectSet<DataPointSet>&, dpsset,
		     const EM::Horizon3DGeometry&, h3dgm,
		     const StepInterval<float>&, intv)
mDefParallelCalcBody(
    DataPointSet::DataRow datarow;
    datarow.data_.setSize( 1, mUdf(float) );
,
    const float shift = intv_.atIndex( idx );
    for ( int sididx=0; sididx<h3dgm_.nrSections(); sididx++ )
    {
	const EM::SectionID sid = h3dgm_.sectionID(sididx);
	const auto* sectgeom = h3dgm_.sectionGeometry(sid);
	datarow.data_[0] = sid;
	const int nrknots = sectgeom->nrKnots();
	for ( int knidx=0; knidx<nrknots; knidx++ )
	{
	    const BinID bid = sectgeom->getKnotRowCol(knidx);
	    const float realz = (float) ( sectgeom->getKnot( bid, false ).z );
	    if ( mIsUdf(realz) )
		continue;

	    datarow.pos_.binid_ = bid;
	    datarow.pos_.z_ = realz+shift;
	    dpsset_[idx]->addRow( datarow );
	}
    }
,)

void uiEMAttribPartServer::fillHorShiftDPS( ObjectSet<DataPointSet>& dpsset,
					    TaskRunner* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    const EM::ObjectID& emid = horshiftdlg_->emID();
    mDynamicCastGet(const EM::Horizon3D*,hor3d,EM::EMM().getObject(emid))
    if ( !hor3d )
	return;

    const StepInterval<float> intv = horshiftdlg_->shiftRg();
    const int nrshifts = horshiftdlg_->nrSteps();
    const EM::Horizon3DGeometry& hor3dgeom = hor3d->geometry();
    for ( int idx=0; idx<nrshifts; idx++ )
    {
	TypeSet<DataPointSet::DataRow> drset;
	BufferStringSet nmset;
	DataPointSet* dps = new DataPointSet( drset, nmset, false, true );

	dps->dataSet().add( new DataColDef( siddef_ ) );
	dps->bivSet().setNrVals( dps->nrFixedCols() + 2 );
	dpsset += dps;
    }

    HorShiftDPSFiller dpsFiller( nrshifts, dpsset, hor3dgeom, intv );
    uiTaskRunner taskrunner( parent() );
    TaskRunner::execute( &taskrunner, dpsFiller );

    for ( int idx=0; idx<nrshifts; idx++ )
	dpsset[idx]->dataChanged();
}


int uiEMAttribPartServer::textureIdx() const
{ return horshiftdlg_->curShiftIdx(); }


void uiEMAttribPartServer::setAttribIdx( int idx )
{
    attribidx_ = idx;
}


void uiEMAttribPartServer::horShifted( CallBacker* )
{
    shiftidx_ = horshiftdlg_->curShiftIdx();
    sendEvent( uiEMAttribPartServer::evHorizonShift() );
}


int uiEMAttribPartServer::getShiftedObjectVisID() const
{
    return horshiftdlg_ ? horshiftdlg_->visID() : -1;
}
