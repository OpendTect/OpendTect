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
#include "uihorizonshiftdlg.h"
#include "uihorsavefieldgrp.h"
#include "uiimphorizon2d.h"
#include "uiseiseventsnapper.h"
#include "uimsg.h"

#include "attribdescset.h"
#include "datapointset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "ioobj.h"
#include "posvecdataset.h"
#include "typeset.h"

static const DataColDef	siddef_( "Section ID" );

const DataColDef& uiEMAttribPartServer::sidDef() const
{ return siddef_; }

uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , nlamodel_(0)
    , horshiftdlg_(0)
    , shiftidx_(10)
    , attribidx_(0)
    , uiimphor2ddlg_(0)
    , uiseisevsnapdlg_(0)
    , aroundhor2ddlg_(0)
    , aroundhor3ddlg_(0)
    , betweenhor2ddlg_(0)
    , betweenhor3ddlg_(0)
    , surfattr2ddlg_(0)
    , surfattr3ddlg_(0)
{
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
}


#define mGlobAttrSet(is2d) Attrib::DescSet::global(is2d)

#define mEMAttrDlg(dlgobj,caption) \
    { \
	if ( !dlgobj ) \
	{ \
	    dlgobj = new uiAttrTrcSelOut( parent(), mGlobAttrSet(is2d), \
			 nlamodel_, nlaid_, type==AroundHor ); \
	    dlgobj->setCaption( caption ); \
	} \
	else \
	    dlgobj->updateAttributes( mGlobAttrSet(is2d), nlamodel_, nlaid_ ); \
	dlgobj->show(); \
    }

#define mSurfAttrDlg(surfdlg,surfdlgcap) \
    { \
	if ( !surfdlg ) \
	{ \
	    surfdlg = new uiAttrSurfaceOut( parent(), mGlobAttrSet(is2d), \
						       nlamodel_, nlaid_ ); \
	    surfdlg->setCaption( surfdlgcap ); \
	} \
	else \
	    surfdlg->updateAttributes( mGlobAttrSet(is2d), nlamodel_,nlaid_ ); \
	surfdlg->go(); \
    }


void uiEMAttribPartServer::createHorizonOutput( HorOutType type, bool is2d )
{
    if ( type==OnHor )
    {
	if ( is2d )
	    mSurfAttrDlg(surfattr2ddlg_,tr("Calculate Horizon Data from 2D"))
	else
	    mSurfAttrDlg(surfattr3ddlg_,tr("Calculate Horizon Data from 3D"))
    }
    else
    {
	if ( is2d )
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


void uiEMAttribPartServer::snapHorizon( const DBKey& emid, bool is2d )
{
    PtrMan<IOObj> ioobj = emid.getIOObj();
    if ( !ioobj )
	return;

    if ( uiseisevsnapdlg_ ) delete uiseisevsnapdlg_;
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


void uiEMAttribPartServer::showHorShiftDlg( const DBKey& id,
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
	    Attrib::DescSet::global(false), initialshift, canaddattrib );
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
    horshiftdlg_ = 0;
}


void uiEMAttribPartServer::calcDPS( CallBacker* )
{
    setAttribID( horshiftdlg_->attribID() );
    sendEvent( uiEMAttribPartServer::evCalcShiftAttribute() );
}


const char* uiEMAttribPartServer::getAttribBaseNm() const
{ return horshiftdlg_ ? horshiftdlg_->getAttribBaseName() : 0; }


void uiEMAttribPartServer::fillHorShiftDPS( ObjectSet<DataPointSet>& dpsset,
					    TaskRunner* )
{
    uiUserShowWait usw( parent(), uiStrings::sCollectingData() );

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
    datarow.data_[0] = 0;
    const int nrknots = hor3dgeom.geometryElement()->nrKnots();

    for ( int idx=0; idx<nrknots; idx++ )
    {
	const RowCol knotrc =
		hor3dgeom.geometryElement()->getKnotRowCol(idx);
	const float realz = (float) (
	    hor3dgeom.geometryElement()->getKnot( knotrc, false ).z_ );
	if ( mIsUdf(realz) )
	    continue;

	datarow.pos_.set( BinID(knotrc) );

	for ( int shiftidx=0; shiftidx<nrshifts; shiftidx++ )
	{
	    const float shift = intv.atIndex(shiftidx);
	    datarow.pos_.setZ( realz+shift );
	    dpsset[shiftidx]->addRow( datarow );
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

int uiEMAttribPartServer::getShiftedObjectVisID() const
{
    return horshiftdlg_ ? horshiftdlg_->visID() : -1;
}
