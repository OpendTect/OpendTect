/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$";

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
}


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
    TypeSet<int> mastercolids = prop_.masterColIDs();
    TypeSet<int> slavecolids = prop_.slaveColIDs();
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
    for ( int scolidx=0; scolidx<slavecolids.size(); scolidx++ )
    {
	if ( mastercolids.validIdx(scolidx) &&
	     mastercolids[scolidx]>mdr.data().size()-1 )
	    rowdata += sdr.data()[slavecolids[scolidx]];
    }

    mdr.pos_ = sdr.pos_;
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
	int mastercolid = mastercolids[col];
	if ( mastercolid>mdr.data().size()-1 )
	    mdr.data_ += sdr.data()[slavecolids[col]];
	
	const float masterval = mdr.data_[mastercolid];
	const float slaveval = sdr.data()[slavecolids[col]];
	if ( prop_.overWriteUndef() && (mIsUdf(masterval) || mIsUdf(slaveval)))
	{
	    if ( mIsUdf(masterval) )
		mdr.data_[mastercolid] = slaveval;
	}
	else if ( prop_.replacePolicy()!=DPSMergerProp::No )
	{
	    mdr.data_[mastercolid] =
		prop_.replacePolicy()==DPSMergerProp::Average
		    ? (slaveval+ masterval)/(float)2 : slaveval;
	}
    }

    return mdr;
}


void DPSMergerProp::setColid( int mastercolid, int slavecolid )
{
    if ( !mastercolids_.isPresent(mastercolid) )
    {
	mastercolids_ += mastercolid;
	slavecolids_ += slavecolid;
    }
    else 
    {
	const int idx = mastercolids_.indexOf( mastercolid );
	slavecolids_[idx] = slavecolid;
    }
}


uiDataPointSetMerger::uiDataPointSetMerger( uiParent* p, DataPointSet* mdps,
					    DataPointSet* sdps )
    : uiDialog(p,uiDialog::Setup("Crossplot data merging","","103.1.18") )
    , mdps_(mdps)
    , sdps_(sdps)
    , ctio_(PosVecDataSetTranslatorGroup::ioContext())
{
    setPrefHeight( 500 );
    DPM( DataPackMgr::PointID() ).addAndObtain( mdps_ );
    DPM( DataPackMgr::PointID() ).addAndObtain( sdps_ );
 
    BufferString capt( "Merge '" );
    capt += mdps->name(); 
    capt += "' with '";
    capt += sdps_->name(); capt += "'";
    setTitleText( capt );

    tbl_ = new uiTable( this,uiTable::Setup(mdps_->nrCols(),1)
						.insertrowallowed(false)
						.removerowallowed(false), "" );
    setTable();
    uiLabel* tbllbl = new uiLabel( this, "Column matching" );
    tbllbl->attach( leftOf, tbl_ );

    BufferString addtxt( "Add new column to '" );
    addtxt += mdps_->name(); addtxt += "'";

    BufferString addcolmsg( "Policy for unused columns of '");
    addcolmsg += sdps_->name(); addcolmsg += "'";
    addcoloptfld_ =
	new uiGenInput( this, addcolmsg, BoolInpSpec(true,"add all",
		    				     "ignore all") );
    addcoloptfld_->attach( leftAlignedBelow, tbllbl );
    addcoloptfld_->attach( ensureBelow, tbl_ );
    
    BufferStringSet matchopts;
    matchopts.add( "Exact match" );
    matchopts.add( "Nearby match" );
    matchopts.add( "Never match, add all new" );
    uiLabeledComboBox* mlcbox =
	new uiLabeledComboBox( this, matchopts,
			       "How do you want to match positions?" );
    mlcbox->attach( alignedBelow, addcoloptfld_ );
    matchpolfld_ = mlcbox->box();
    matchpolfld_->setHSzPol( uiObject::MedVar );
    matchpolfld_->setCurrentItem( 1 );
    matchpolfld_->selectionChanged.notify(
	    mCB(this,uiDataPointSetMerger,matchPolChangedCB) );

    BufferString maxtxt( "Search within a horizontal radius" );
    maxtxt += SI().getXYUnitString();
    maxtxt += " of";
    distfld_ = new uiGenInput( this, maxtxt, FloatInpSpec() );
    distfld_->setElemSzPol( uiObject::Small );
    distfld_->attach( alignedBelow, mlcbox );
    distfld_->setValue( SI().inlDistance() );
    
    BufferString ztxt( "and vertical distance" );
    ztxt += SI().getZUnitString();
    ztxt += " of";
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
			       "Replace policy for matching positions" );
    rlcbox->attach( alignedBelow, distfld_ );
    replacepolfld_ = rlcbox->box();
    replacepolfld_->setHSzPol( uiObject::MedVar );

    overwritefld_ =
	new uiGenInput( this, "Undefined values",
			BoolInpSpec(true,"Replace if possible","Keep") );
    overwritefld_->attach( alignedBelow, rlcbox );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Crossplot" );
    outfld_->attach( alignedBelow, overwritefld_ ); 

    matchPolChangedCB( 0 );
    attribChangedCB( 0 );
}


uiDataPointSetMerger::~uiDataPointSetMerger()
{
    DPM( DataPackMgr::PointID() ).release( mdps_->id() );
    DPM( DataPackMgr::PointID() ).release( sdps_->id() );
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
    tbl_->setColumnLabel( 0, sdps_->name() );
    BufferStringSet colnames;
    colnames.add( "None" );
    for ( int colnr=0; colnr<sdps_->nrCols(); colnr++ )
	colnames.add( sdps_->colName(colnr) );

    const int nrcols = mdps_->nrCols();
    tbl_->setNrRows( nrcols );
    
    for ( int rowidx=0; rowidx<nrcols; rowidx++ )
    {
	BufferString colnm( mdps_->colName(rowidx) );
	BufferString celltxt( "Couple '");
	celltxt += colnm; celltxt += "' to";
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
	if ( mIsUdf(distfld_->getfValue()) || distfld_->getfValue()<0 )
	{
	    uiMSG().error( "Choose a proper horizontal search radius" );
	    return false;
	}
	else if ( mIsUdf(zgatefld_->getfValue()) || zgatefld_->getfValue()<0 )
	{
	    uiMSG().error( "Choose a proper vertical search radius" );
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
    uiTaskRunner tr( this );
    dpsmrfprop.setOverWriteUndef( overwritefld_->getBoolValue() );
    dpsmrfprop.setMaxAllowedHorDist( distfld_->getfValue() );
    dpsmrfprop.setMaxAllowedZDist(
	    zgatefld_->getfValue()/SI().zDomain().userFactor() );
    
    DPSMerger merger( dpsmrfprop ); 
    merger.addNewCols( newcolnms );
    TaskRunner::execute( &tr, merger );

    BufferString errmsg;
    const bool ret =
	merger.getNewDPS()->dataSet().putTo( dpsobj->fullUserExpr(false),
					     errmsg, false );
    if ( !ret )
	uiMSG().error( errmsg );

    return true;
}
