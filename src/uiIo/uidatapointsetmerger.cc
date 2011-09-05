/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/
static const char* rcsID = "$";

#include "uidatapointsetmerger.h"

#include "datacoldef.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "ioman.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uicombobox.h"
#include "uitable.h"
#include "uitaskrunner.h"


static int sAttrCol = 1;
static int sChkCol = 2;


DPSMerger::DPSMerger( const DPSMergerProp& prop )
    : Executor( "Merging Positions" )
    , rowdone_(-1)
    , prop_(prop)
{
    DataPack* mdp = DPM( DataPackMgr::PointID() ).obtain( prop.masterDPID() );
    mDynamicCast(DataPointSet*,mdps_,mdp);
    DataPack* sdp = DPM( DataPackMgr::PointID() ).obtain( prop.slaveDPID() );
    mDynamicCast(DataPointSet*,sdps_,sdp);
    newdps_ = new DataPointSet( *mdps_ );
    addNewCols();
}


void DPSMerger::addNewCols()
{
    TypeSet<int> slavecolids = prop_.slaveColIDs();
    TypeSet<int> mastercolids = prop_.masterColIDs();
    for ( int col=0; col<mastercolids.size(); col++ )
    {
	if ( mIsUdf(mastercolids[col]) )
	{
	    newdps_->dataSet().add(
		    new DataColDef(sdps_->colName(slavecolids[col])) );
	}
    }
}


int DPSMerger::nextStep()
{
    if ( !newdps_ || !mdps_ || !sdps_ )
	return ErrorOccurred();

    if ( rowdone_ >= sdps_->size()-1 )
	return Finished();

    rowdone_++;
    DataPointSet::DataRow dr = sdps_->dataRow( rowdone_ );

    int matchrow = findMatchingMrowID( rowdone_ );
    if ( matchrow<0 )
	newdps_->addRow( getNewDataRow(rowdone_) );
    else
	newdps_->setRow( getDataRow(rowdone_,matchrow) );

    return MoreToDo();
}


int DPSMerger::findMatchingMrowID( int srowid )
{
    if ( prop_.matchPolicy()==DPSMergerProp::NoMatch )
	return -1;

    DataPointSet::Pos pos = sdps_->pos( srowid );
    return mdps_->find( pos );
}


DataPointSet::DataRow DPSMerger::getNewDataRow( int srowid )
{
    TypeSet<int> mastercolids = prop_.masterColIDs();
    DataPointSet::DataRow mdr = mdps_->dataRow( 0 );
    DataPointSet::DataRow sdr = sdps_->dataRow( srowid );
    TypeSet<float> rowdata = mdr.data();
    for ( int mcolid=0; mcolid<rowdata.size(); mcolid++ )
    {
	if ( !mastercolids.isPresent(mcolid) )
	    rowdata[mcolid] = mUdf(float);
	else
	{
	    const int slaveidx = getSlaveColID( mcolid );
	    if ( slaveidx < 0 )
		continue;
	    rowdata[mcolid] = sdr.data()[slaveidx];
	}
    }

    mdr.data_ = rowdata;
    return mdr;
}


int DPSMerger::getSlaveColID( int mcolid )
{
    const int idx = prop_.masterColIDs().indexOf( mcolid );
    if ( idx<0 )
	return -1;
    return prop_.slaveColIDs()[idx];
}


DataPointSet::DataRow DPSMerger::getDataRow( int srowid, int mrowid )
{
    TypeSet<int> slavecolids = prop_.slaveColIDs();
    TypeSet<int> mastercolids = prop_.masterColIDs();
    DataPointSet::DataRow sdr = sdps_->dataRow( srowid );
    DataPointSet::DataRow mdr = mdps_->dataRow( mrowid );
    for ( int col=0; col<mastercolids.size(); col++ )
    {
	mdr.data_[mastercolids[col]] =
	    prop_.replacePolicy()==DPSMergerProp::Average
	    	? (sdr.data()[slavecolids[col]]+
		   mdr.data()[mastercolids[col]])/(float)2
		: sdr.data()[slavecolids[col]];
    }

    return mdr;
}


void DPSMergerProp::setColid( int mastercolid, int slavecolid )
{
    if ( mIsUdf(mastercolid) || (!mIsUdf(mastercolid) &&
				 !mastercolids_.isPresent(mastercolid)) )
    {
	mastercolids_ += mastercolid;
	slavecolids_ += slavecolid;
    }
    else if ( !mIsUdf(mastercolid) && mastercolids_.isPresent(mastercolid) )
    {
	const int idx = mastercolids_.indexOf( mastercolid );
	slavecolids_[idx] = slavecolid;
    }
}


uiDataPointSetMerger::uiDataPointSetMerger( uiParent* p, DataPointSet* mdps,
					    DataPointSet* sdps )
    : uiDialog(p,uiDialog::Setup("Merge crossplot","",mTODOHelpID) )
    , mdps_(mdps)
    , sdps_(sdps)
    , ctio_(PosVecDataSetTranslatorGroup::ioContext())
{
    DPM( DataPackMgr::PointID() ).addAndObtain( mdps_ );
    DPM( DataPackMgr::PointID() ).addAndObtain( sdps_ );
 
    BufferString capt( "Merge " );
    capt += mdps->name();
    capt += " with ";
    capt += sdps_->name();
    setCaption( capt );

    tbl_ =
	new uiTable( this, uiTable::Setup(mdps_->nrCols(),3).rowgrow(true), "");
    setTable();


    BufferStringSet matchopts;
    matchopts.add( "Exact match" );
    matchopts.add( "Nearby match" );
    matchopts.add( "Never match, add all new" );
    uiLabeledComboBox* mlcbox =
	new uiLabeledComboBox( this, matchopts,
			       "How do you want to match positions?" );
    mlcbox->attach( leftAlignedBelow, tbl_ );
    matchpolfld_ = mlcbox->box();

    BufferStringSet replaceopts;
    replaceopts.add( "Keep original" );
    replaceopts.add( "Replace with new" );
    replaceopts.add( "Average of both" );
    uiLabeledComboBox* rlcbox =
	new uiLabeledComboBox( this, replaceopts,
			       "Replace policy for matching positions" );
    rlcbox->attach( leftAlignedBelow, mlcbox );
    replacepolfld_ = rlcbox->box();

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Crossplot" );
    outfld_->attach( leftAlignedBelow, rlcbox ); 
}


uiDataPointSetMerger::~uiDataPointSetMerger()
{
    DPM( DataPackMgr::PointID() ).release( mdps_->id() );
    DPM( DataPackMgr::PointID() ).release( sdps_->id() );
}


void uiDataPointSetMerger::setTable()
{
    tbl_->setColumnLabel( 0, mdps_->name() );
    tbl_->setColumnLabel( 1, sdps_->name() );
    tbl_->setColumnLabel( 2, "Merger option" );
    BufferStringSet colnames;
    for ( int colnr=0; colnr<sdps_->nrCols(); colnr++ )
	colnames.add( sdps_->colName(colnr) );

    for ( int rowidx=0; rowidx<mdps_->nrCols(); rowidx++ )
    {
	tbl_->setText( RowCol(rowidx,0), mdps_->colName(rowidx) );
	uiComboBox* lb = new uiComboBox( 0, colnames, "Attributes" );
	tbl_->setCellObject( RowCol(rowidx,sAttrCol), lb );
	uiCheckBox* cb = new uiCheckBox( 0, "Merge" );
	tbl_->setCellObject( RowCol(rowidx,sChkCol), new uiCheckBox(0,"Merge"));
    }
}


bool uiDataPointSetMerger::acceptOK( CallBacker* )
{
    if ( !outfld_->ioobj() )
    { uiMSG().error( "Select output first." ); return false; }

    PtrMan<IOObj> dpsobj = outfld_->getIOObj();
    DPSMergerProp dpsmrfprop( dpsobj->key(), mdps_->id(), sdps_->id());
    dpsmrfprop.setMatchPolicy(
	    (DPSMergerProp::MatchPolicy)matchpolfld_->currentItem() );
    dpsmrfprop.setReplacePolicy(
	    (DPSMergerProp::ReplacePolicy)replacepolfld_->currentItem() );
    for ( int rowidx=0; rowidx<tbl_->nrRows(); rowidx++ )
    {
	uiObject* cbobj = tbl_->getCellObject( RowCol(rowidx,sAttrCol) );
	mDynamicCastGet(uiComboBox*,combobox,cbobj);
	if ( !combobox )
	    continue;

	uiObject* obj = tbl_->getCellObject( RowCol(rowidx,sChkCol) );
	mDynamicCastGet(uiCheckBox*,cb,obj);
	if ( cb && cb->isChecked() )
	    dpsmrfprop.setColid( rowidx, combobox->currentItem() );
    }

    uiTaskRunner tr( this );
    DPSMerger merger( dpsmrfprop ); 
    tr.execute( merger );

    BufferString errmsg;
    const bool ret =
	merger.getNewDPS()->dataSet().putTo( dpsobj->fullUserExpr(false),
					     errmsg, false );
    if ( !ret )
	uiMSG().error( errmsg );

    return true;
}
