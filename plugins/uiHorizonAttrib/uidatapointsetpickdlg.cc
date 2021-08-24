/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2013
________________________________________________________________________

-*/

#include "uidatapointsetpickdlg.h"

#include "uiarray2dinterpol.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "visdataman.h"
#include "vispicksetdisplay.h"
#include "visselman.h"
#include "uidatapointsetman.h"

#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "arrayndalgo.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "keystrs.h"
#include "pickset.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"


uiDataPointSetPickDlg::uiDataPointSetPickDlg( uiParent* p, int sceneid )
    : uiDialog(p,Setup(tr("DataPointSet picking"),mNoDlgTitle,
			mODHelpKey(mDataPointSetPickDlgHelpID)).modal(false))
    , sceneid_(sceneid)
    , dps_(*new DataPointSet(false,false))
    , picksetmgr_(Pick::SetMgr::getMgr("DPSPicks"))
    , psd_(0)
    , changed_(false)
{
    setCtrlStyle( CloseOnly );

    tb_ = new uiToolBar( this, "ToolBar" );
    pickbutid_ = tb_->addButton( "seedpickmode", uiString::emptyString(),
	mCB(this,uiDataPointSetPickDlg,pickModeCB), true );
    tb_->addButton( "open", tr("Open DataPointSet"),
	mCB(this,uiDataPointSetPickDlg,openCB) );
    savebutid_ = tb_->addButton( "save", tr("Save DataPointSet"),
	mCB(this,uiDataPointSetPickDlg,saveCB) );
    tb_->addButton( "saveas", tr("Save DataPointSet As"),
	mCB(this,uiDataPointSetPickDlg,saveasCB) );

    table_ = new uiTable( this, uiTable::Setup(10,6), "Position table" );
    table_->setPrefWidth( 500 );
    uiStringSet lbls;
    lbls.add( uiStrings::sInline() ).add( uiStrings::sCrossline() )
	.add( uiStrings::sX() ).add( uiStrings::sY() )
	.add( uiStrings::sZ() ).add( uiStrings::sValue() );
    table_->setColumnLabels( lbls );
    table_->valueChanged.notify( mCB(this,uiDataPointSetPickDlg,valChgCB) );
    table_->rowClicked.notify( mCB(this,uiDataPointSetPickDlg,rowClickCB) );

    dps_.dataSet().add( new DataColDef("Value") );
    initPickSet();
    updateButtons();

    windowClosed.notify( mCB(this,uiDataPointSetPickDlg,winCloseCB) );
    visBase::DM().selMan().selnotifier.notify(
			mCB(this,uiDataPointSetPickDlg,objSelCB) );
}


uiDataPointSetPickDlg::~uiDataPointSetPickDlg()
{
    visBase::DM().selMan().selnotifier.remove(
			mCB(this,uiDataPointSetPickDlg,objSelCB) );
    if ( psd_ ) cleanUp();
}


void uiDataPointSetPickDlg::winCloseCB( CallBacker* )
{ cleanUp(); }

void uiDataPointSetPickDlg::objSelCB( CallBacker* cb )
{ tb_->turnOn( pickbutid_, false ); }


void uiDataPointSetPickDlg::cleanUp()
{
    table_->clearTable();
    delete &dps_;

    visBase::DataObject* obj = visBase::DM().getObject( sceneid_ );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene && psd_ )
    {
	const int objidx = scene->getFirstIdx( psd_->id() );
	scene->removeObject( objidx );
	psd_->unRef();
	psd_ = 0;
    }

    picksetmgr_.locationChanged.remove(
			mCB(this,uiDataPointSetPickDlg,locChgCB) );
    picksetmgr_.setChanged.remove( mCB(this,uiDataPointSetPickDlg,pickCB) );
}


static MultiID getMultiID( int sceneid )
{
    // Create dummy multiid, I don't want to save these picks
    BufferString mid( "9999.", sceneid );
    return MultiID( mid.buf() );
}


void uiDataPointSetPickDlg::initPickSet()
{
    psd_ = new visSurvey::PickSetDisplay();
    psd_->ref();

    Pick::Set* ps = new Pick::Set( "DPS picks" );
    ps->disp_.color_ = Color( 255, 0, 0 );
    psd_->setSet( ps );
    psd_->setSetMgr( &picksetmgr_ );
    picksetmgr_.set( getMultiID(sceneid_), ps );
    picksetmgr_.locationChanged.notify(
			mCB(this,uiDataPointSetPickDlg,locChgCB) );
    picksetmgr_.setChanged.notify( mCB(this,uiDataPointSetPickDlg,pickCB) );

    visBase::DataObject* obj = visBase::DM().getObject( sceneid_ );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    scene->addObject( psd_ );
}


void uiDataPointSetPickDlg::pickModeCB( CallBacker* )
{
    if ( !psd_ ) return;

    if ( tb_->isOn(pickbutid_) )
	visBase::DM().selMan().select( psd_->id() );
    else
	visBase::DM().selMan().deSelect( psd_->id() );
}


void uiDataPointSetPickDlg::openCB( CallBacker* )
{
    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( this, ctio );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return;

    PosVecDataSet pvds;
    uiString errmsg;
    const bool rv = pvds.getFrom( ioobj->fullUserExpr(true), errmsg );
    if ( !rv )
	{ uiMSG().error( errmsg ); return; }
    if ( pvds.data().isEmpty() )
    { uiMSG().error(uiDataPointSetMan::sSelDataSetEmpty()); return; }

    Pick::Set* pickset = psd_ ? psd_->getSet() : 0;
    if ( !pickset ) return;

    values_.erase();
    pickset->erase();
    DataPointSet newdps( pvds, false );
    for ( int idx=0; idx<newdps.size(); idx++ )
    {
	const DataPointSet::Pos pos( newdps.pos(idx) );
	Pick::Location loc( pos.coord(), pos.z() );
	(*pickset) += loc;
	values_ += newdps.value(0,idx);
    }

    if ( psd_ ) psd_->redrawAll();

    updateDPS();
    updateTable();
    changed_ = false;
    updateButtons();

    setCaption( toUiString(ioobj->name()) );
}


void uiDataPointSetPickDlg::saveCB( CallBacker* )
{ doSave( false ); }


void uiDataPointSetPickDlg::saveasCB( CallBacker* )
{ doSave( true ); }


void uiDataPointSetPickDlg::doSave( bool saveas )
{
    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return;

    PosVecDataSet pvds;
    uiString errmsg;
    const bool rv = dps_.dataSet().putTo( ioobj->fullUserExpr(true),
					  errmsg, false );
    if ( !rv )
	{ uiMSG().error( errmsg ); return; }

    setCaption( toUiString(ioobj->name()) );
    changed_ = false;
    updateButtons();
}


void uiDataPointSetPickDlg::valChgCB( CallBacker* )
{
    const int col = table_->notifiedCell().col();
    if ( col < 5 ) return;

    const int row = table_->notifiedCell().row();
    const float val = table_->getFValue(RowCol(row,5) );
    dps_.setValue( 0, row, val );
    dps_.dataChanged();

    Pick::Set* set = psd_ ? psd_->getSet() : 0;
    if ( !set ) return;

    const DataPointSet::Pos pos( dps_.pos(row) );
    const Coord3 dpscrd( pos.coord(), pos.z() );
    double mindist = mUdf( double );
    int locidx = -1;
    for ( int idx=0; idx<set->size(); idx++ )
    {
	const double dst = dpscrd.distTo( (*set)[idx].pos_ );
	if ( dst > mindist ) continue;

	mindist = dst;
	locidx = idx;
    }

    if ( values_.validIdx(locidx) )
	values_[locidx] = val;
}


void uiDataPointSetPickDlg::rowClickCB( CallBacker* cb )
{
//    mCBCapsuleUnpack(int,row,cb);
//    const DataPointSet::Pos pos( dps_.pos(row) );
//    TODO: Highlight this pick in 3D scene
}


void uiDataPointSetPickDlg::locChgCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd || !psd_ || (cd->set_ != psd_->getSet()) )
	return;

    if ( cd->ev_ == Pick::SetMgr::ChangeData::Added )
    {
	values_ += mUdf(float);
    }
    else if ( cd->ev_ == Pick::SetMgr::ChangeData::ToBeRemoved )
    {
	if ( values_.validIdx(cd->loc_) )
	    values_.removeSingle( cd->loc_ );
    }
    else if ( cd->ev_ == Pick::SetMgr::ChangeData::Changed )
    {
    }

    pickCB( cb );
}


void uiDataPointSetPickDlg::pickCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb);
    if ( !cd || !cd->set_ ) return;

    changed_ = true;
    updateDPS();
    updateTable();
    updateButtons();
}


void uiDataPointSetPickDlg::updateButtons()
{
    tb_->setSensitive( savebutid_, changed_ );
}


void uiDataPointSetPickDlg::updateDPS()
{
    dps_.clearData();
    const Pick::Set* set = psd_ ? psd_->getSet() : 0;
    if ( !set )
    {
	dps_.dataChanged();
	return;
    }

    for ( int idx=0; idx<set->size(); idx++ )
    {
	DataPointSet::Pos pos;
	const Pick::Location loc = (*set)[idx];
	pos.set( loc.pos_ );
	DataPointSet::DataRow row( pos );
	row.data_ += values_[idx];
	dps_.addRow( row );
    }

    dps_.dataChanged();
}


void uiDataPointSetPickDlg::updateTable()
{
    NotifyStopper ns( table_->valueChanged );
    table_->clearTable();
    if ( table_->nrRows() < dps_.size() )
	table_->setNrRows( dps_.size() );

    for ( int idx=0; idx<dps_.size(); idx++ )
    {
	const DataPointSet::Pos pos( dps_.pos(idx) );
	table_->setValue( RowCol(idx,0), pos.binid_.inl() );
	table_->setValue( RowCol(idx,1), pos.binid_.crl() );
	table_->setValue( RowCol(idx,2), pos.coord().x );
	table_->setValue( RowCol(idx,3), pos.coord().y );
	table_->setValue( RowCol(idx,4), pos.z() );
	table_->setValue( RowCol(idx,5), dps_.value(0,idx) );
    }
}


bool uiDataPointSetPickDlg::acceptOK( CallBacker* )
{
    return true;
}



// uiEMDataPointSetPickDlg
uiEMDataPointSetPickDlg::uiEMDataPointSetPickDlg( uiParent* p, int sceneid,
						  EM::ObjectID emid )
    : uiDataPointSetPickDlg(p,sceneid)
    , emid_(emid)
    , emdps_(*new DataPointSet(false,true))
    , readyForDisplay(this)
    , interpol_(0)
    , dataidx_(-1)
{
    setCaption( toUiString("Surface data picking") );

    auto* interpolbut = new uiPushButton( this, tr("Interpolate"), true );
    interpolbut->activated.notify(
	mCB(this,uiEMDataPointSetPickDlg,interpolateCB) );
    interpolbut->attach( alignedBelow, table_ );

    auto* settbut = new uiToolButton( this, "settings",
				tr("Interpolation settings"),
				mCB(this,uiEMDataPointSetPickDlg,settCB) );
    settbut->attach( rightOf, interpolbut );

    emdps_.dataSet().add( new DataColDef("Section ID") );
    emdps_.dataSet().add( new DataColDef("AuxData") );


    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj ) emobj->ref();
}


uiEMDataPointSetPickDlg::~uiEMDataPointSetPickDlg()
{
}


void uiEMDataPointSetPickDlg::cleanUp()
{
    uiDataPointSetPickDlg::cleanUp();

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj ) emobj->unRef();

    delete &emdps_;
}


int uiEMDataPointSetPickDlg::addSurfaceData()
{
    emdps_.clearData();
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)

    float auxvals[3];
    const EM::SectionID sid = hor3d->sectionID( 0 );
    tks_ = hor3d->range();
    auxvals[1] = sid;
    PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	auxvals[0] = (float) hor3d->getPos( pid ).z;
	auxvals[2] = mUdf( float );
	BinID bid = BinID::fromInt64( pid.subID() );
	emdps_.bivSet().add( bid, auxvals );
    }

    emdps_.dataChanged();
    return 1;
}


void uiEMDataPointSetPickDlg::interpolateCB( CallBacker* )
{
    if ( !interpol_ ) settCB(0);
    if ( !interpol_ ) return;

    if ( dataidx_ < 0 )
	dataidx_ = addSurfaceData();

    int nrinl = tks_.nrInl();
    int nrcrl = tks_.nrCrl();
    Array2DImpl<float> arr( nrinl, nrcrl );
    arr.setAll( mUdf(float) );
    for ( int idx=0; idx<dps_.size(); idx++ )
    {
	DataPointSet::Pos pos = dps_.pos( idx );
	const int inlidx = tks_.inlIdx( pos.binid_.inl() );
	const int crlidx = tks_.crlIdx( pos.binid_.crl() );
	const float* vals = dps_.getValues( idx );
	arr.set( inlidx, crlidx, vals[0] );
    }

    uiTaskRunner uitr( this );
    if ( !interpol_->setArray(arr,&uitr) )
	return;

    if ( !uitr.execute(*interpol_) )
	return;

    BinIDValueSet& bivs = emdps_.bivSet();
    BIDValSetArrAdapter adapter( bivs, 2, tks_.step_ );
    Array2DCopier<float> copier( arr, tks_, tks_, adapter );
    copier.executeParallel( true );

    readyForDisplay.trigger();
}


void uiEMDataPointSetPickDlg::settCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this,
		uiDialog::Setup( tr("Interpolate Horizon Data"),
		mNoDlgTitle,mODHelpKey(muiEMDataPointSetPickDlgHelpID)) );
    uiArray2DInterpolSel* settings =
	new uiArray2DInterpolSel( &dlg, false, false, true, interpol_, false );
    settings->setDistanceUnit( SI().xyInFeet() ? tr("[ft]") : tr("[m]") );
    dlg.setGroup( settings );
    if ( !dlg.go() ) return;

    delete interpol_;
    interpol_ = settings->getResult();
    if ( !interpol_ ) return;

    interpol_->setFillType( Array2DInterpol::Full );
    interpol_->setRowStep( SI().inlDistance() * tks_.step_.inl() );
    interpol_->setColStep( SI().crlDistance() * tks_.step_.crl() );
}
