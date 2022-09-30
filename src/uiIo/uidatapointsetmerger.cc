/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidatapointsetmerger.h"

#include "datacoldef.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "ioman.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uidpsaddcolumndlg.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

// DPSMergerProp
DPSMergerProp::DPSMergerProp( const MultiID& id, PackID mid, PackID sid )
    : primarydpsid_(mid)
    , secondarydpsid_(sid)
    , newdpsid_(id)
{}


DPSMergerProp::~DPSMergerProp()
{}


DPSMergerProp::PackID DPSMergerProp::primaryDPID() const
{ return primarydpsid_; }

DPSMergerProp::PackID DPSMergerProp::secondaryDPID() const
{ return secondarydpsid_; }

const TypeSet<int>& DPSMergerProp::primaryColIDs() const
{ return primarycolids_; }

const TypeSet<int>& DPSMergerProp::secondaryColIDs() const
{ return secondarycolids_; }


// DPSMerger
DPSMerger::DPSMerger( const DPSMergerProp& prop )
    : Executor( "Merging Positions" )
    , rowdone_(-1)
    , prop_(prop)
{
    auto mdp = DPM(DataPackMgr::PointID()).getDP( prop.primaryDPID() );
    mDynamicCast(DataPointSet*,mdps_,mdp.ptr());
    auto sdp = DPM(DataPackMgr::PointID()).getDP( prop.secondaryDPID() );
    mDynamicCast(DataPointSet*,sdps_,sdp.ptr());
    newdps_ = new DataPointSet( *mdps_ );
}


DPSMerger::~DPSMerger()
{}


void DPSMerger::addNewCols( const BufferStringSet& clnms )
{
    if ( !clnms.size() ) return;

    for ( int col=0; col<clnms.size(); col++ )
	newdps_->dataSet().add( new DataColDef(clnms[col]->buf()) );
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
    return prop_.matchPolicy()==DPSMergerProp::Exact
			    ? mdps_->find( pos )
			    : mdps_->find( pos, prop_.maxAllowedHorDist(),
					   prop_.maxAllowedZDist() );
}


DataPointSet::DataRow DPSMerger::getNewDataRow( int srowid )
{
    TypeSet<int> primarycolids = prop_.primaryColIDs();
    TypeSet<int> secondarycolids = prop_.secondaryColIDs();
    DataPointSet::DataRow mdr = mdps_->dataRow( 0 );
    DataPointSet::DataRow sdr = sdps_->dataRow( srowid );
    TypeSet<float> rowdata = mdr.data();
    for ( int mcolid=0; mcolid<rowdata.size(); mcolid++ )
    {
	if ( !primarycolids.isPresent(mcolid) )
	    rowdata[mcolid] = mUdf(float);
	else
	{
	    const int secondaryidx = getSecondaryColID( mcolid );
	    if ( secondaryidx < 0 )
		continue;
	    rowdata[mcolid] = sdr.data()[secondaryidx];
	}
    }
    for ( int scolidx=0; scolidx<secondarycolids.size(); scolidx++ )
    {
	if ( primarycolids.validIdx(scolidx) &&
	     primarycolids[scolidx]>mdr.data().size()-1 )
	    rowdata += sdr.data()[secondarycolids[scolidx]];
    }

    mdr.pos_ = sdr.pos_;
    mdr.data_ = rowdata;
    return mdr;
}


int DPSMerger::getSecondaryColID( int mcolid )
{
    const int idx = prop_.primaryColIDs().indexOf( mcolid );
    if ( idx<0 )
	return -1;
    return prop_.secondaryColIDs()[idx];
}


int DPSMerger::getSlaveColID( int colid )
{
    return getSecondaryColID( colid );
}


DataPointSet::DataRow DPSMerger::getDataRow( int srowid, int mrowid )
{
    TypeSet<int> secondarycolids = prop_.secondaryColIDs();
    TypeSet<int> primarycolids = prop_.primaryColIDs();
    DataPointSet::DataRow sdr = sdps_->dataRow( srowid );
    DataPointSet::DataRow mdr = mdps_->dataRow( mrowid );
    for ( int col=0; col<primarycolids.size(); col++ )
    {
	int primarycolid = primarycolids[col];
	if ( primarycolid>mdr.data().size()-1 )
	    mdr.data_ += sdr.data()[secondarycolids[col]];

	const float primaryval = mdr.data_[primarycolid];
	const float secondaryval = sdr.data()[secondarycolids[col]];
	if ( prop_.overWriteUndef() &&
	     (mIsUdf(primaryval) || mIsUdf(secondaryval)) )
	{
	    if ( mIsUdf(primaryval) )
		mdr.data_[primarycolid] = secondaryval;
	}
	else if ( prop_.replacePolicy()!=DPSMergerProp::No )
	{
	    mdr.data_[primarycolid] =
		prop_.replacePolicy()==DPSMergerProp::Average
		    ? (secondaryval+ primaryval)/(float)2 : secondaryval;
	}
    }

    return mdr;
}


void DPSMergerProp::setColid( int primarycolid, int secondarycolid )
{
    if ( !primarycolids_.isPresent(primarycolid) )
    {
	primarycolids_ += primarycolid;
	secondarycolids_ += secondarycolid;
    }
    else
    {
	const int idx = primarycolids_.indexOf( primarycolid );
	secondarycolids_[idx] = secondarycolid;
    }
}


uiDataPointSetMerger::uiDataPointSetMerger( uiParent* p, DataPointSet* mdps,
					    DataPointSet* sdps )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrCrossPlot(uiStrings::phrData(
				 tr("merging"))), uiString::emptyString(),
                                 mODHelpKey(mDataPointSetMergerHelpID) ) )
    , mdps_(mdps)
    , sdps_(sdps)
{
    setPrefHeight( 500 );
    DPM( DataPackMgr::PointID() ).add( mdps_ );
    DPM( DataPackMgr::PointID() ).add( sdps_ );

    uiString capt = uiStrings::phrMerge(tr("'%1' with '%2'")
			       .arg(toUiString(mdps->name()))
			       .arg(toUiString(sdps_->name())));
    setTitleText( capt );

    tbl_ = new uiTable( this,uiTable::Setup(mdps_->nrCols(),1)
						.insertrowallowed(false)
						.removerowallowed(false), "" );
    setTable();
    uiLabel* tbllbl = new uiLabel( this, tr("Column matching") );
    tbllbl->attach( leftOf, tbl_ );

    BufferString addtxt( "Add new column to '" );
    addtxt += mdps_->name(); addtxt += "'";

    uiString addcolmsg = tr("Policy for unused columns of '%1'")
						.arg(toUiString(sdps_->name()));
    addcoloptfld_ =
	new uiGenInput(this, addcolmsg, BoolInpSpec(true,uiStrings::phrAdd(
				         uiStrings::sAll()),tr("ignore all")));
    addcoloptfld_->attach( leftAlignedBelow, tbllbl );
    addcoloptfld_->attach( ensureBelow, tbl_ );

    BufferStringSet matchopts;
    matchopts.add( "Exact match" );
    matchopts.add( "Nearby match" );
    matchopts.add( "Never match, add all new" );
    uiLabeledComboBox* mlcbox =
	new uiLabeledComboBox( this, matchopts,
			       tr("How do you want to match positions?") );
    mlcbox->attach( alignedBelow, addcoloptfld_ );
    matchpolfld_ = mlcbox->box();
    matchpolfld_->setHSzPol( uiObject::MedVar );
    matchpolfld_->setCurrentItem( 1 );
    matchpolfld_->selectionChanged.notify(
	    mCB(this,uiDataPointSetMerger,matchPolChangedCB) );

    uiString maxtxt = tr("Search within a horizontal radius %1 of")
		      .arg(SI().getUiXYUnitString());
    distfld_ = new uiGenInput( this, maxtxt, FloatInpSpec() );
    distfld_->setElemSzPol( uiObject::Small );
    distfld_->attach( alignedBelow, mlcbox );
    distfld_->setValue( SI().inlDistance() );

    uiString ztxt = tr("and vertical distance %1 of")
						  .arg(SI().getUiZUnitString());
    zgatefld_ = new uiGenInput( this, ztxt, FloatInpSpec() );
    zgatefld_->setElemSzPol( uiObject::Small );
    zgatefld_->attach( rightTo, distfld_ );
    zgatefld_->setValue( SI().zStep()*SI().zDomain().userFactor() );

    BufferStringSet replaceopts;
    BufferString opt1( "Keep '" ); opt1 += mdps_->name(); opt1 += "'";
    replaceopts.add( opt1 );

    BufferString opt2( "Overwrite with '" );opt2 += sdps_->name();opt2 += "'";
    replaceopts.add( opt2 );
    replaceopts.add( "Take the average of both" );

    uiLabeledComboBox* rlcbox =
	new uiLabeledComboBox( this, replaceopts,
			       tr("Replace policy for matching positions") );
    rlcbox->attach( alignedBelow, distfld_ );
    replacepolfld_ = rlcbox->box();
    replacepolfld_->setHSzPol( uiObject::MedVar );

    overwritefld_ =
	new uiGenInput( this, uiStrings::sUndefVal(),
			BoolInpSpec(true,tr("Replace if possible"),tr("Keep")));
    overwritefld_->attach( alignedBelow, rlcbox );

    IOObjContext ctxt = mIOObjContext( PosVecDataSet );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrOutput(uiStrings::sCrossPlot()) );
    outfld_->attach( alignedBelow, overwritefld_ );

    matchPolChangedCB( 0 );
    attribChangedCB( 0 );
}


uiDataPointSetMerger::~uiDataPointSetMerger()
{
}


void uiDataPointSetMerger::matchPolChangedCB( CallBacker* )
{
    replacepolfld_->setSensitive( matchpolfld_->currentItem()!=2 );
    overwritefld_->display( matchpolfld_->currentItem()!=2 );
    distfld_->display( matchpolfld_->currentItem()==1 );
    zgatefld_->display( matchpolfld_->currentItem()==1 );
}


void uiDataPointSetMerger::setTable()
{
    tbl_->setColumnLabel( 0, toUiString(sdps_->name()) );
    BufferStringSet colnames;
    colnames.add( "None" );
    for ( int colnr=0; colnr<sdps_->nrCols(); colnr++ )
	colnames.add( sdps_->colName(colnr) );

    const int nrcols = mdps_->nrCols();
    tbl_->setNrRows( nrcols );

    for ( int rowidx=0; rowidx<nrcols; rowidx++ )
    {
	BufferString colnm( mdps_->colName(rowidx) );
	uiString celltxt = tr("Couple '%1' to").arg(colnm);
        tbl_->setRowLabel( rowidx, celltxt );
	tbl_->setRowLabel( rowidx, celltxt );
	uiComboBox* cb = new uiComboBox( 0, colnames, "Attributes" );
	cb->selectionChanged.notify(
		mCB(this,uiDataPointSetMerger,attribChangedCB) );
	const int nearmatchidx = colnames.nearestMatch( colnm );
	if ( nearmatchidx>= 0 )
	    cb->setCurrentItem( nearmatchidx );

	tbl_->setCellObject( RowCol(rowidx,0), cb );
    }
}



void uiDataPointSetMerger::attribChangedCB( CallBacker* )
{
    TypeSet<int> scolids;
    for ( int rowidx=0; rowidx<tbl_->nrRows(); rowidx++ )
    {
	uiObject* cbobj = tbl_->getCellObject( RowCol(rowidx,0) );
	mDynamicCastGet(uiComboBox*,combobox,cbobj);
	if ( !combobox )
	    continue;
	if ( combobox->currentItem() )
	    scolids.addIfNew( combobox->currentItem()-1 );
    }

    addcoloptfld_->setSensitive( scolids.size()<sdps_->nrCols() );
}


BufferStringSet uiDataPointSetMerger::checkForNewColumns() const
{
    BufferStringSet newcolnames;
    if ( !addcoloptfld_->getBoolValue() )
	return newcolnames;

    TypeSet<int> scolids;
    for ( int col=0; col<sdps_->nrCols(); col++ )
	scolids.addIfNew( col );

    for ( int rowidx=0; rowidx<tbl_->nrRows(); rowidx++ )
    {
	uiObject* cbobj = tbl_->getCellObject( RowCol(rowidx,0) );
	mDynamicCastGet(uiComboBox*,combobox,cbobj);
	if ( !combobox )
	    continue;
	if ( combobox->currentItem() && (sdps_->indexOf(combobox->text())>=0) )
	    scolids -= sdps_->indexOf( combobox->text() );
    }

    for ( int colidx=0; colidx<scolids.size(); colidx++ )
	newcolnames.add( sdps_->colName(scolids[colidx]) );

    return newcolnames;
}


void uiDataPointSetMerger::checkForSameColNms( BufferStringSet& colnms ) const
{
    BufferStringSet oldcolnames;
    for ( int col=0; col<mdps_->nrCols(); col++ )
	oldcolnames.add( mdps_->colName(col) );
    for ( int colidx=0; colidx<colnms.size(); colidx++ )
    {
	if ( oldcolnames.isPresent(colnms[colidx]->buf()) )
	    colnms.get(colidx) += "(2)";
    }
}


bool uiDataPointSetMerger::acceptOK( CallBacker* )
{
   if ( !outfld_->ioobj() )
       return false;

    if ( matchpolfld_->currentItem()==1 )
    {
	if ( mIsUdf(distfld_->getFValue()) || distfld_->getFValue()<0 )
	{
	    uiMSG().error(tr("Choose a proper horizontal search radius"));
	    return false;
	}
	else if ( mIsUdf(zgatefld_->getFValue()) || zgatefld_->getFValue()<0 )
	{
	    uiMSG().error(tr("Choose a proper vertical search radius"));
	    return false;
	}
    }

    PtrMan<IOObj> dpsobj = outfld_->getIOObj();
    DPSMergerProp dpsmrfprop( dpsobj->key(), mdps_->id(), sdps_->id());
    dpsmrfprop.setMatchPolicy(
	    (DPSMergerProp::MatchPolicy)matchpolfld_->currentItem() );
    dpsmrfprop.setReplacePolicy(
	    (DPSMergerProp::ReplacePolicy)replacepolfld_->currentItem() );
    BufferStringSet newcolnms = checkForNewColumns();
    const int nrrows = tbl_->nrRows();
    for ( int rowidx=0; rowidx<nrrows+newcolnms.size(); rowidx++ )
    {
	const bool isnew = rowidx > nrrows-1;
	if ( !isnew )
	{
	    uiObject* cbobj = tbl_->getCellObject( RowCol(rowidx,0) );
	    mDynamicCastGet(uiComboBox*,combobox,cbobj);
	    if ( !combobox )
		continue;

	    if ( combobox->currentItem() )
		dpsmrfprop.setColid( rowidx, combobox->currentItem()-1 );
	}
	else
	    dpsmrfprop.setColid( rowidx, sdps_->indexOf(
					    newcolnms.get(rowidx-nrrows)) );
    }

    checkForSameColNms( newcolnms );
    uiTaskRunner taskrunner( this );
    dpsmrfprop.setOverWriteUndef( overwritefld_->getBoolValue() );
    dpsmrfprop.setMaxAllowedHorDist( distfld_->getFValue() );
    dpsmrfprop.setMaxAllowedZDist(
	    zgatefld_->getFValue()/SI().zDomain().userFactor() );

    DPSMerger merger( dpsmrfprop );
    merger.addNewCols( newcolnms );
    TaskRunner::execute( &taskrunner, merger );

    BufferString errmsg;
    const bool ret =
	merger.getNewDPS()->dataSet().putTo( dpsobj->fullUserExpr(false),
					     errmsg, false );
    if ( !ret )
	uiMSG().error( mToUiStringTodo(errmsg) );

    return true;
}
