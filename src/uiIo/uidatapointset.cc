/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uidatapointset.cc,v 1.99 2012-07-10 08:05:35 cvskris Exp $";

#include "uidatapointset.h"
#include "uidatapointsetman.h"
#include "uistatsdisplaywin.h"
#include "uidatapointsetcrossplotwin.h"
#include "uimsg.h"

#include "datapointset.h"
#include "dpsdispmgr.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "randcolor.h"
#include "datacoldef.h"
#include "ctxtioobj.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "unitofmeasure.h"
#include "keystrs.h"
#include "oddirs.h"
#include "unitofmeasure.h"
#include "mousecursor.h"
#include "mathexpression.h"
#include "settings.h"
#include "timer.h"
#include "variogramcomputers.h"

#include "uibutton.h"
#include "uitable.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolortable.h"
#include "uidpsaddcolumndlg.h"
#include "uidpsselectednessdlg.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uifileinput.h"
#include "uistatusbar.h"
#include "uiobjdisposer.h"
#include "uimsg.h"
#include "uivariogram.h"

static const int cNrPosCols = 3;
static const int cMinPtsForDensity = 20000;
static const char* sKeyGroups = "Groups";

mDefineInstanceCreatedNotifierAccess(uiDataPointSet)

mClass uiDPSDispPropDlg : public uiDialog
{
public:
uiDPSDispPropDlg( uiParent* p, const uiDataPointSetCrossPlotter& plotter,
		  const DataPointSetDisplayProp* prevdispprop )
    : uiDialog(this,uiDialog::Setup("Display Properties","","").modal(false))
    , plotter_(plotter)
{
    BoolInpSpec binp( prevdispprop ? prevdispprop->showSelected() : false,
	    	      "Selected Points","All points with attribute" );
    typefld_ = new uiGenInput( this, "Display",binp );
    typefld_->valuechanged.notify( mCB(this,uiDPSDispPropDlg,typeChangedCB) );

    BufferStringSet colnms;
    const DataPointSet& dps = plotter.dps();
    for ( int colidx=0; colidx<dps.nrCols(); colidx++ )
	colnms.add( dps.colName(colidx) );

    uiLabeledComboBox* llb = new uiLabeledComboBox(
	    			this, colnms, "Attribute to display" );
    llb->attach( alignedBelow, typefld_ );
    selfld_ = llb->box();
    if ( prevdispprop && !prevdispprop->showSelected() )
    {
	const char* attrnm = dps.colName( prevdispprop->dpsColID() );
	selfld_->setCurrentItem( attrnm );
    }

    selfld_->selectionChanged.notify(mCB(this,uiDPSDispPropDlg,attribChanged));

    coltabfld_ =  new uiColorTable( this, ColTab::Sequence("Rainbow"), false );
    coltabfld_->attach( leftAlignedBelow, llb );
    if ( prevdispprop && !prevdispprop->showSelected() )
    {
	coltabfld_->setSequence( &prevdispprop->colSequence(), true );
	coltabfld_->setMapperSetup( &prevdispprop->colMapperSetUp() );
    }

    attribChanged( 0 );
    typeChangedCB( 0 );
}

void attribChanged( CallBacker* )
{
    const DataPointSet& dps = plotter_.dps();
    const int bivsidx = dps.bivSetIdx( dps.indexOf(selfld_->text()) );
    Interval<float> valrange = dps.bivSet().valRange( bivsidx );
    coltabfld_->setInterval( valrange );
}


void typeChangedCB( CallBacker* )
{
    selfld_->display( !typefld_->getBoolValue(), true );
    coltabfld_->display( !typefld_->getBoolValue(), true );
}

bool type() const
{ return typefld_->getBoolValue(); }

const char* colName() const
{ return selfld_->text(); }

const ColTab::Sequence& ctSeq() const
{ return coltabfld_->colTabSeq(); }

const ColTab::MapperSetup& ctMapperSetup() const
{ return coltabfld_->colTabMapperSetup(); }

    uiGenInput*				typefld_;
    uiComboBox*				selfld_;
    uiColorTable*			coltabfld_;
    const uiDataPointSetCrossPlotter&	plotter_;
};



uiDataPointSet::Setup::Setup( const char* wintitl, bool ismodal )
    : uiDialog::Setup(wintitl?wintitl:"Extracted data",mNoDlgTitle,"111.0.0")
    , isconst_(false)
    , canaddrow_(false)
    , directremove_(true)
    , allowretrieve_(true)
    , initialmaxnrlines_(4000)
{
    modal_ = ismodal;
    nrstatusflds_ = 1;
}


#define mDPM DPM(DataPackMgr::PointID())

uiDataPointSet::uiDataPointSet( uiParent* p, const DataPointSet& dps,
				const uiDataPointSet::Setup& su,
       				DataPointSetDisplayMgr* dpsmgr )
	: uiDialog(p,su)
	, dps_(*const_cast<DataPointSet*>(&dps))
    	, setup_(su)
    	, zfac_(SI().zDomain().userFactor())
    	, zunitnm_(SI().getZUnitString(false))
	, tbl_(0)
    	, unsavedchgs_(false)
    	, fillingtable_(true)
    	, valueChanged(this)
    	, selPtsToBeShown(this)
    	, rowAdded(this)
    	, rowToBeRemoved(this)
    	, rowRemoved(this)
	, xplotwin_(0)
	, dpsdisppropdlg_(0)
	, statswin_(0)
	, dpsdispmgr_(dpsmgr)
	, iotb_(0)
	, disptb_(0)
	, maniptb_(0)
	, percfld_(0)
	, timer_(new Timer())
{
    windowClosed.notify( mCB(this,uiDataPointSet,closeNotify) );

    if ( mDPM.haveID(dps_.id()) )
	mDPM.obtain( dps_.id() );
    setCtrlStyle( LeaveOnly );
    runcalcs_.allowNull( true );

    const int nrcols = initVars();
    mkToolBars();

    const char* dpsnm = dps.name();
    uiLabel* titllbl = 0;
    if ( *dpsnm != '<' )
    {
	titllbl = new uiLabel( this, dpsnm );
	titllbl->attach( hCentered );
    }

    tbl_ = new uiTable( this, uiTable::Setup(size() ? size() : 10,nrcols)
			  .rowgrow( setup_.canaddrow_ ).removerowallowed(false)
			  .selmode( uiTable::Multi )
			  .manualresize( true ), "Data Table" );
    if ( titllbl )
	tbl_->attach( ensureBelow, titllbl );
    tbl_->valueChanged.notify( mCB(this,uiDataPointSet,valChg) );
    tbl_->rowClicked.notify( mCB(this,uiDataPointSet,rowSel) );
    tbl_->rowInserted.notify( mCB(this,uiDataPointSet,rowAddedCB) );
    tbl_->selectionChanged.notify( mCB(this,uiDataPointSet,selChg) );
    tbl_->setTableReadOnly( setup_.isconst_ );
    tbl_->setLabelAlignment( Alignment::Left, true );
    dps_.dataSet().pars().get( sKeyGroups, grpnames_ );

    selPtsToBeShown.notify( mCB(this,uiDataPointSet,showSelPts) );
    setPrefWidth( 800 ); setPrefHeight( 600 );

    postFinalise().notify( mCB(this,uiDataPointSet,initWin) );
    mTriggerInstanceCreatedNotifier();
}


#define mCleanRunCalcs \
    deepErase( runcalcs_ ); \
    const int nrcols = dps_.nrCols() + cNrPosCols; \
    for ( int idx=0; idx<nrcols; idx++ ) \
	runcalcs_ += 0

int uiDataPointSet::initVars()
{
    sortcol_ = statscol_ = xcol_ = ycol_ = y2col_ = -1;
    delete xplotwin_; xplotwin_ = 0;
    delete statswin_; statswin_ = 0;

    mCleanRunCalcs;

    eachrow_ = dps_.nrActive() / setup_.initialmaxnrlines_;
    if ( eachrow_ < 1.0 ) eachrow_ = 1.0;
    percentage_ = (float)100/eachrow_;

    calcIdxs();
    if ( tbl_ )
	disptb_->setSensitive( xplottbid_, true );
    return nrcols;
}


uiDataPointSet::~uiDataPointSet()
{
    deepErase( variodlgs_ );
    removeSelPts( 0 );
    if ( dpsdisppropdlg_ )
	dpsdisppropdlg_->windowClosed.notify(
		mCB(this,uiDataPointSet,showPtsInWorkSpace) );
}


void uiDataPointSet::closeNotify( CallBacker* )
{ plotpercentage_ = mUdf(float); }


void uiDataPointSet::mkToolBars()
{
    if ( iotb_) iotb_->clear();
    if ( disptb_ ) disptb_->clear();
    if ( maniptb_ ) maniptb_->clear();

#define mAddButton(fnm,func,tip) \
    iotb_->addButton( fnm, tip, mCB(this,uiDataPointSet,func) )
    if ( !setup_.isconst_ )
    {
	if ( !iotb_ )
	    iotb_ = new uiToolBar( this, "I/O Tool bar" );
	mAddButton( "saveset.png", save, "Save data" );
	if ( setup_.allowretrieve_ )
	    mAddButton( "openset", retrieve, "Retrieve stored data" );
    }
#undef mAddButton

    if ( !maniptb_ )
	maniptb_ = new uiToolBar( this, "Manip Tool bar" );
#define mAddButton(fnm,func,tip) \
    maniptb_->addButton( fnm, tip, mCB(this,uiDataPointSet,func) )
    mAddButton( "axis-x", selXCol, "Set data for X" );
    mAddButton( "axis-add-y", selYCol, "Select as Y data" );
    mAddButton( "axis-rm-y", unSelCol, "UnSelect as Y data" );
    mAddButton( "delselrows", delSelRows, "Remove selected rows" );
    mAddButton( "axis-prev", colStepL, "Set Y one column left" );
    mAddButton( "axis-next", colStepR, "Set Y one column right" );
    mAddButton( "sortcol", setSortCol, "Set sorted column to current" );
    mAddButton( "plus", addColumn, "Add column.." );
    mAddButton( "minus", removeColumn, "Remove column" );
#undef mAddButton

    if ( !disptb_ )
	disptb_ = new uiToolBar( this, "Display Tool bar" );

    uiGroup* grp = new uiGroup( disptb_, "Each grp" );
    percfld_ = new uiSpinBox( grp, 1, "Each" );
    percfld_->setSuffix( "%" );
    percfld_->setValue( percentage_ );
    percfld_->setInterval( (float)0.1, mUdf(float),(float)0.1 );
    percfld_->valueChanged.notify( mCB(this,uiDataPointSet,eachChg) );
    new uiLabel( grp, "Show", percfld_ );
    disptb_->addObject( grp->attachObj() );

#define mAddButton(fnm,func,tip,istogg) \
    disptb_->addButton( fnm, tip, mCB(this,uiDataPointSet,func), istogg )
    dispxytbid_ = mAddButton( "toggxy", toggleXYZ,
			      "Toggle show X and Y columns", true );
    dispztbid_ = mAddButton( "toggz", toggleXYZ,
			     "Toggle show Z column", true );
    mAddButton( "statsinfo", showStatsWin,
			     "Show histogram and stats for column", false );
    if ( dps_.group(0) < mUdf(od_uint16) && SI().zIsTime() )
	mAddButton( "variogram", compVertVariogram,
		    "Compute variogram for column", false );
    xplottbid_ = mAddButton( "xplot", showCrossPlot,
	    		     "Show crossplot", false );

    disptb_->turnOn( dispxytbid_, true ); disptb_->turnOn( dispztbid_, true );
}


void uiDataPointSet::updColNames()
{
    const int nrcols = dps_.nrCols() + 3;
    const TColID zcid = 2;
    for ( TColID tid=0; tid<nrcols; tid++ )
    {
	BufferString axnm;
	if ( tid == xcol_ )
	    axnm += "X";
	else if ( tid == ycol_ )
	    axnm += "Y";
	else if ( tid == y2col_ )
	    axnm += "Y2";

	BufferString colnm( tid == sortcol_ ? "*" : "" );;
	if ( !axnm.isEmpty() )
	    { colnm += "["; colnm += axnm; colnm += "]"; }

	if ( tid == zcid )
	    colnm += BufferString("Z (",zunitnm_,")");
	else
	    colnm += userName( dColID(tid) );
	
	if ( tid > tbl_->nrCols()-1 )
	    tbl_->insertColumns( tid, 1 );

	tbl_->setColumnLabel( tid, colnm );
    }
}


void uiDataPointSet::calcIdxs()
{
    const int orgtblsz = drowids_.size();
    drowids_.erase(); trowids_.erase(); sortidxs_.erase();

    const int dpssz = dps_.size();
    if ( dpssz<0 )
    {
	uiMSG().message( "DataPointSet too large, choose a subselection" );
	return;
    }

    trowids_.setSize( dpssz, -1 );
    int calcidx = 0;
    int dcountidx = 0;
    for ( int did=0; did<dpssz; did++ )
    {
	const int eachcount = 0;
	const bool inact = dps_.isInactive(did);
	if ( inact || (dcountidx < mNINT32(calcidx * eachrow_)) )
	{
	    if ( !inact )
		dcountidx++;
	    continue;
	}
	else
	{
	    const TRowID tid = drowids_.size();
	    sortidxs_ += tid;
	    trowids_[did] = tid;
	    drowids_ += did;
	}
	calcidx++;
	dcountidx++;
    }

    revsortidxs_ = sortidxs_;
    if ( sortcol_ >= 0 )
	calcSortIdxs();

    const int newtblsz = drowids_.size();
    if ( tbl_ && (newtblsz != orgtblsz || newtblsz != tbl_->nrRows()) )
	tbl_->setNrRows( newtblsz );
}


void uiDataPointSet::calcSortIdxs()
{
    const DColID dcid = dColID( sortcol_ );
    TypeSet<float> vals;
    for ( int disprid=0; disprid<sortidxs_.size(); disprid++ )
    {
	const DRowID drid = drowids_[ disprid ];
	float val = getVal( dcid, drid, false );
	if ( mIsUdf(val) ) val = MAXFLOAT;
	vals += val;
    }

    sort_coupled( vals.arr(), sortidxs_.arr(), sortidxs_.size() );
    for ( int idx=0; idx<sortidxs_.size(); idx++ )
	revsortidxs_[ sortidxs_[idx] ] = idx;
}


uiDataPointSet::DRowID uiDataPointSet::dRowID( TRowID tid ) const
{
    if ( tid < -1 ) tid = tbl_->currentRow();
    if ( !sortidxs_.validIdx(tid) ) return -1;
    return drowids_[ sortidxs_[tid] ];
}


uiDataPointSet::TRowID uiDataPointSet::tRowID( DRowID did ) const
{
    if ( did < -1 ) return tbl_->currentRow();
    else if ( did < 0 ) return -1;
    const TRowID itabrow = trowids_[did];
    if ( itabrow < 0 ) return -1;
    return revsortidxs_[ itabrow ];
}


uiDataPointSet::DColID uiDataPointSet::dColID( TColID tid ) const
{
    if ( tid < -1 ) tid = tbl_->currentCol();
    return tid - cNrPosCols;
}


uiDataPointSet::TColID uiDataPointSet::tColID( DColID did ) const
{
    if ( did < -1-cNrPosCols ) return tbl_->currentCol();

    int ret = did + cNrPosCols;
    if ( ret < 0 ) ret = -1;
    return ret;
}


void uiDataPointSet::fillPos( TRowID tid )
{
    fillingtable_ = true;
    const DataPointSet::Pos pos( dps_.pos(dRowID(tid)) );
    RowCol rc( tid, 0 );
    const Coord c( pos.coord() );
    tbl_->setValue( rc, c.x ); rc.col++;
    tbl_->setValue( rc, c.y ); rc.col++;
    if ( mIsUdf(pos.z_) )
	tbl_->setText( rc, "" );
    else
    {
	float fz = zfac_ * pos.z_ * 100;
	int iz = mNINT32(fz);
	tbl_->setValue( rc, iz * 0.01 );
    }
    BufferString rownm;
    if ( is2D() )
	rownm += pos.nr_;
    else
	{ rownm += pos.binid_.inl; rownm += "/"; rownm += pos.binid_.crl; }
    tbl_->setRowLabel( tid, rownm );

    fillingtable_ = false;
}


void uiDataPointSet::fillData( TRowID tid )
{
    RowCol rc( tid, cNrPosCols );
    const DRowID drid = dRowID(tid);
    fillingtable_ = true;
    for ( DColID dcid=0; dcid<dps_.nrCols(); dcid++ )
	{ tbl_->setValue( rc, getVal(dcid,drid,true) ); rc.col++; }
    fillingtable_ = false;
}


void uiDataPointSet::handleAxisColChg()
{
    updColNames();
    if ( xplotwin_ )
    {
	xplotwin_->plotter().setCols( dColID(xcol_), dColID(ycol_),
				      dColID(y2col_) );
	xplotwin_->setButtonStatus();
    }

    if ( ycol_ >= 0 && statswin_ )
	showStats( dColID(ycol_) );
    
}


void uiDataPointSet::initWin( CallBacker* c )
{
    setSortedCol( 3 );
    if ( dps_.isEmpty() && setup_.allowretrieve_ )
	timer_->start( 500, true );
}


void uiDataPointSet::selXCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;
    if ( xcol_ != tid )
    {
	xcol_ = tid;
	handleAxisColChg();
    }
}


void uiDataPointSet::selYCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;

    const TColID prevy = ycol_; const TColID prevy2 = y2col_;
    int minptsfordensity = cMinPtsForDensity;
    Settings& setts = Settings::common();
    setts.get( sKeyMinDPPts(), minptsfordensity );

    if ( ycol_ == -1 )
    {
	ycol_ = tid;
	plotpercentage_ =
	    (float)100 / (float)(1+dps_.nrActive()/minptsfordensity);
    }
    else
    {
	if ( dps_.nrActive()*2 > minptsfordensity )
	{
	    BufferString msg( "DataPoint set too large. Percentage of points "
		    	      "displayed should be modified to give better "
			      "performance. Do you want to change 'Plot each' "
		      	      "or do you want to continue with no Y2 ?" ); 

	    if ( uiMSG().askGoOn(msg,"Change % Data displayed",
				 "Continue with no Y2") )
	    {
		plotpercentage_ =
		    (float)100 / (float)(1+dps_.nrActive()*2/minptsfordensity);
		y2col_ = tid;
	    }
	    else
	    {
		plotpercentage_ =
		    (float)100 / (float)(1+dps_.nrActive()/minptsfordensity);
		y2col_ = -1;
	    }		
	}
	else
	    y2col_ = tid;
    }
    
    if ( prevy != ycol_ || prevy2 != y2col_ )
    {
	if ( xplotwin_ )
	    xplotwin_->setPercDisp( plotpercentage_ );
	handleAxisColChg();
    }
 
    if ( xplotwin_ && y2col_ == tid )
	xplotwin_->setSelComboSensitive( true );

    if ( xplotwin_ ) xplotwin_->setGrpColors();
}


void uiDataPointSet::unSelCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;

    if ( tid == ycol_ )
	{ ycol_ = y2col_; y2col_ = -1; }
    else if ( tid == y2col_ )
	y2col_ = -1;
    else
	return;

    handleAxisColChg();
    
    if ( xplotwin_ && y2col_==-1 )
	xplotwin_->setSelComboSensitive( false );
    
    if ( xplotwin_ ) xplotwin_->setGrpColors();
}


void uiDataPointSet::colStepL( CallBacker* )
{
    ycol_ = ycol_ < 1 ? tbl_->nrCols()-1 : ycol_ - 1;
    if ( ycol_ == 2 && !isDisp(false) )
	ycol_--;

    handleAxisColChg();
}


void uiDataPointSet::colStepR( CallBacker* )
{
    ycol_ = ycol_ >= tbl_->nrCols()-1 ? 0 : ycol_ + 1;
    if ( ycol_ == 2 && !isDisp(false) )
	ycol_++;
    if ( ycol_ >= tbl_->nrCols() )
	ycol_ = 0;

    handleAxisColChg();
}


void uiDataPointSet::rowSel( CallBacker* cb )
{
    mCBCapsuleUnpack(int,trid,cb);
    setStatsMarker( dRowID(trid) );
    handleGroupChg( dRowID(trid) );
}


class uiSelectPosDlg : public uiDialog
{
public:
uiSelectPosDlg( uiParent* p, const BufferStringSet& grpnames )
    : uiDialog( p, uiDialog::Setup("Select Position for new row","","") )
    , grpfld_(0)
{
    seltypefld_ = new uiGenInput( this, "Position type",
			BoolInpSpec(true,"X/Y","Inline/CrossLine") );
    seltypefld_->valuechanged.notify( mCB(this,uiSelectPosDlg,selTypeChanged) );

    posinpfld_ = new uiGenInput( this, "Input Position",
			PositionInpSpec( PositionInpSpec::Setup(true)) );
    posinpfld_->attach( leftAlignedBelow, seltypefld_ );

    BufferString zinptxt( "Z Value in " );
    SI().zIsTime() ? zinptxt += "sec" : zinptxt += "Metre/Feet";
    zinpfld_ = new uiGenInput( this, zinptxt, FloatInpSpec() );
    zinpfld_->attach( leftAlignedBelow, posinpfld_ );

    if ( grpnames.size()>1 )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Select group" );
	grpfld_ = lcb->box();
	grpfld_->addItems( grpnames );
	grpfld_->attach( alignedBelow, zinpfld_ );
    }
}


void selTypeChanged( CallBacker* )
{
    posinpfld_->newSpec( PositionInpSpec(PositionInpSpec::Setup(
		    			 seltypefld_->getBoolValue())), 0 );
}

bool acceptOK( CallBacker* )
{
    DataPointSet::Pos pos;
    const bool isreasonable =
	seltypefld_->getBoolValue() ? SI().isReasonable(posinpfld_->getCoord())
				    : SI().isReasonable(posinpfld_->getBinID());
    if (!isreasonable )
    {
	uiMSG().error( "Position entered is not valid." );
	return false;
    }
    if ( seltypefld_->getBoolValue() )
	pos.set( posinpfld_->getCoord() );
    else
	pos.set( posinpfld_->getBinID() );
    pos.z_ = zinpfld_->getfValue();
    
    datarow_ =
	DataPointSet::DataRow( pos, !grpfld_ ? 1 : grpfld_->currentItem() +1 );
    return true;
}

    DataPointSet::DataRow datarow_;
    uiGenInput*		seltypefld_;
    uiGenInput*		posinpfld_;
    uiGenInput*		zinpfld_;
    uiComboBox*		grpfld_;
};


void uiDataPointSet::rowAddedCB( CallBacker* cb )
{
    uiSelectPosDlg dlg( this, groupNames() );
    if ( dlg.go() )
    {
	addRow( dlg.datarow_ );
	Coord3 newcoord( dlg.datarow_.coord(), dlg.datarow_.pos_.z_ );
	rowAdded.trigger();
	for ( int rownr=0; rownr<tbl_->nrRows(); rownr++ )
	{
	    Coord3 coord(
	      tbl_->getdValue(RowCol(rownr,0)),
	      tbl_->getdValue(RowCol(rownr,1)),
	      tbl_->getdValue(RowCol(rownr,2))/SI().zDomain().userFactor() );
	    if ( mIsEqual(coord.x,newcoord.x,2) &&
	    	 mIsEqual(coord.y,newcoord.y,2) &&
	    	 mIsEqual(coord.z,newcoord.z,1e-4) )
	    {
		tbl_->ensureCellVisible( RowCol(rownr,0) );
		break;
	    }
	}
    }
    else
	tbl_->removeRow( tbl_->newCell().row );
}


void uiDataPointSet::selChg( CallBacker* )
{
    const ObjectSet<uiTable::SelectionRange>& selrgs = tbl_->selectedRanges();
    if ( selrgs.isEmpty() ) return;

    handleGroupChg( dRowID(selrgs[0]->firstrow_) );
}


void uiDataPointSet::handleGroupChg( uiDataPointSet::DRowID drid )
{
    if ( drid < 1 )
	return;
    const int grp = dps_.group( drid );
    if ( grp < 1 ) return;
    const char* grpnm = groupName( grp );
    if ( grpnm && *grpnm )
    {
	BufferString txt( grptype_ );
	if ( !txt.isEmpty() ) txt += ": ";
	txt += grpnm;
	statusBar()->message( txt, 0 );
    }
}


bool uiDataPointSet::isDisp( bool xy ) const
{
    return disptb_->isOn( xy ? dispxytbid_ : dispztbid_ );
}


void uiDataPointSet::toggleXYZ( CallBacker* )
{
    const bool havexy = disptb_->isOn( dispxytbid_ );
    const bool havez = disptb_->isOn( dispztbid_ );
    tbl_->hideColumn( 0, !havexy );
    tbl_->hideColumn( 1, !havexy );
    tbl_->hideColumn( 2, !havez );
    redoAll();
}


void uiDataPointSet::showCrossPlot( CallBacker* )
{
    if ( xplotwin_ )
	xplotwin_->plotter().dataChanged();
    else
    {
	xplotwin_ = new uiDataPointSetCrossPlotWin( *this );
	uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
	xpl.selectionChanged.notify( mCB(this,uiDataPointSet,xplotSelChg) );
	xpl.removeRequest.notify( mCB(this,uiDataPointSet,xplotRemReq) );
	xplotwin_->plotter().pointsSelected.notify(
		mCB(this,uiDataPointSet,showStatusMsg) );
	xplotwin_->windowClosed.notify( mCB(this,uiDataPointSet,xplotClose) );
    }

    disptb_->setSensitive( xplottbid_, false );
    xplotwin_->setPercDisp( plotpercentage_ );
    disptb_->setSensitive( xplottbid_, false );
    handleAxisColChg();
    xplotwin_->show();
}


void uiDataPointSet::showStatusMsg( CallBacker* )
{
    if ( !xplotwin_ || !&xplotwin_->plotter() )
	return;
    BufferString msg( "Y Selected: ", xplotwin_->plotter().nrYSels() );
    if ( xplotwin_->plotter().isY2Shown() )
    {
	msg += "; Y2 Selected: ";
	msg += xplotwin_->plotter().nrY2Sels();
    }
    xplotwin_->statusBar()->message( msg, 1 );
}


void uiDataPointSet::notifySelectedCell()
{
    TypeSet<RowCol> selectedrowcols( xplotwin_->plotter().getSelectedCells() );
    if ( selectedrowcols.isEmpty() )
    {
	tbl_->removeAllSelections();
	return;
    }

    tbl_->selectItems( selectedrowcols, true );
}


void uiDataPointSet::getXplotPos( uiDataPointSet::DColID& dcid,
				  uiDataPointSet::DRowID& drid ) const
{
    drid = -1; dcid = -cNrPosCols-1;
    if ( !xplotwin_ ) return;
    const uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
    drid = xpl.selRow();
    TColID tcid = xpl.selRowIsY2() ? y2col_ : ycol_;
    if ( tcid >= 0 )
	dcid = dColID( tcid );
}


void uiDataPointSet::setCurrent( uiDataPointSet::DColID dcid,
				 uiDataPointSet::DRowID drid )
{
    RowCol rc( tRowID(drid), tColID(dcid) );
    if ( rc.col >= 0 && rc.row >= 0 )
	tbl_->setCurrentCell( rc );
}


void uiDataPointSet::setCurrent( const DataPointSet::Pos& pos,
				 uiDataPointSet::DColID dcid )
{
    setCurrent( dps_.find(pos), dcid );
}


void uiDataPointSet::xplotSelChg( CallBacker* )
{
    int dcid, drid; getXplotPos( dcid, drid );
    if ( drid < 0 || dcid < -cNrPosCols ) return;

    setCurrent( dcid, drid );
    setStatsMarker( drid );
}


void uiDataPointSet::setStatsMarker( uiDataPointSet::DRowID drid )
{
    if ( !statswin_ || statscol_ < 0 ) return;

    const float val = getVal( dColID(statscol_), drid, false );
    statswin_->setMarkValue( val, true );
}


void uiDataPointSet::xplotRemReq( CallBacker* )
{
    int drid, dcid; getXplotPos( dcid, drid );
    if ( drid < 0 ) return;

    if ( setup_.directremove_ )
	dps_.setInactive( drid, true );
    else
    {
	rowToBeRemoved.trigger( drid );
	dps_.bivSet().remove( dps_.bvsPos(drid) );
	rowRemoved.trigger( drid );
    }

    if ( !setup_.directremove_ )
	dps_.dataChanged();

    const TRowID trid = tRowID( drid );
    if ( trid >= 0 )
	redoAll();

    delete runcalcs_.replace( dcid, 0 );
}


void uiDataPointSet::reDoTable()
{
    calcIdxs();
    updColNames();

    for ( DColID dcid=0; dcid<dps_.nrCols(); dcid++ )
    {
	const UnitOfMeasure* mu = dps_.colDef(dcid).unit_;
	if ( mu )
	    tbl_->setColumnToolTip( tColID(dcid), mu->name() );
    }

    const int nrrows = tbl_->nrRows();
    for ( TRowID tid=0; tid<nrrows; tid++ )
    {
	fillPos( tid );
	fillData( tid );
    }

    tbl_->resizeRowsToContents();
}


void uiDataPointSet::redoAll()
{
    reDoTable();

    if ( statswin_ )
	showStats( dColID(statscol_) );
    if ( xplotwin_ )
	showCrossPlot( 0 );
}


void uiDataPointSet::xplotClose( CallBacker* )
{
    uiOBJDISP()->go( xplotwin_ );
    disptb_->setSensitive( xplottbid_, true );
    xplotwin_ = 0;
}


void uiDataPointSet::statsClose( CallBacker* )
{
    uiOBJDISP()->go( statswin_ );
    statswin_ = 0;
}


const char* uiDataPointSet::userName( uiDataPointSet::DColID did ) const
{
    if ( did >= 0 )
	return dps_.colName( did );
    else if ( did == -1 )
	return "Z";
    else
	return did == -3 ? "X-Coord" : "Y-Coord";
}


#define mGetRCIdx(dcid) \
    int rcidx = dcid; \
    if ( rcidx < 0 ) rcidx = dps_.nrCols() - 1 - dcid


Stats::RunCalc<float>& uiDataPointSet::getRunCalc(
				uiDataPointSet::DColID dcid ) const
{
    static Stats::RunCalc<float> empty( Stats::CalcSetup(false) );
    if ( dcid < -cNrPosCols ) return empty;

    mGetRCIdx(dcid);
    Stats::RunCalc<float>* rc = runcalcs_[rcidx];
    if ( !rc )
    {
	Stats::CalcSetup su( false );
#	define mReq(typ) require(Stats::typ)
	su.mReq(Count).mReq(Average).mReq(Median).mReq(StdDev);
	rc = new Stats::RunCalc<float>( su.mReq(Min).mReq(Max).mReq(RMS) );
	for ( DRowID drid=0; drid<dps_.size(); drid++ )
	{
	    if ( !dps_.isInactive(drid) )
		rc->addValue( getVal( dcid, drid, true ) );
	}
	runcalcs_.replace( rcidx, rc );
    }

    return *rc;
}


void uiDataPointSet::showStatsWin( CallBacker* )
{
    const DColID dcid = dColID();
    showStats( dcid );
}


void uiDataPointSet::showStats( uiDataPointSet::DColID dcid )
{
    int newcol = tColID( dcid );
    if ( newcol < 0 ) return;
    statscol_ = newcol;

    BufferString txt( "Column: " );
    txt += userName( dcid );
    if ( statscol_ >= 0 )
    {
	const DataColDef& dcd = dps_.colDef( dcid );
	if ( dcd.unit_ )
	    { txt += " ("; txt += dcd.unit_->name(); txt += ")"; }
	if ( dcd.ref_ != dcd.name_ )
	    { txt += "\n"; txt += dcd.ref_; }
    }

    const Stats::RunCalc<float>& rc = getRunCalc( dcid );
    if ( !statswin_ )
    {
	statswin_ = new uiStatsDisplayWin( this, uiStatsDisplay::Setup(), 1, false );
	statswin_->windowClosed.notify( mCB(this,uiDataPointSet,statsClose) );
    }

    //statswin_->setData( rc );
    statswin_->setDataName( txt );
    statswin_->show();
}


float uiDataPointSet::getVal( DColID dcid, DRowID drid, bool foruser ) const
{
    if ( dcid >= 0 )
    {
	const float val = dps_.value( dcid, drid );
	if ( !foruser ) return val;
	const UnitOfMeasure* mu = dps_.colDef( dcid ).unit_;
	return mu ? mu->userValue(val) : val;
    }
    else if ( dcid == -1 )
    {
	const float val = dps_.z( drid );
	if ( !foruser ) return val;
	return val * zfac_;
    }

    return dcid == -3 ? dps_.coord(drid).x : dps_.coord(drid).y;
}


#define mRetErr fillPos( cell.row ); return

void uiDataPointSet::valChg( CallBacker* )
{
    if ( fillingtable_ ) return;

    const RowCol& cell = tbl_->notifiedCell();
    if ( cell.row < 0 || cell.col < 0 ) return;

    const DColID dcid( dColID(cell.col) );
    const DRowID drid( dRowID(cell.row) );

    afterchgdr_ = beforechgdr_ = dps_.dataRow( drid );
    bool poschgd = false;

    if ( dcid >= 0 )
    {
	float val = tbl_->getfValue( cell );
	const UnitOfMeasure* mu = dps_.colDef( dcid ).unit_;
	afterchgdr_.data_[dcid] = mu ? mu->internalValue(val) : val;
    }
    else
    {
	const char* txt = tbl_->text( cell );
	if ( !txt || !*txt )
	{
	    uiMSG().error( "Positioning values must be filled" );
	    mRetErr;
	}
	DataPointSet::Pos& pos( afterchgdr_.pos_ );

	if ( dcid == -1 )
	{
	    if ( !isDisp(false) ) { pErrMsg("Huh"); mRetErr; }
	    pos.z_ = tbl_->getfValue( cell ) / zfac_;
	    poschgd = !mIsZero(pos.z_-beforechgdr_.pos_.z_,1e-6);
	}
	else
	{
	    if ( !isDisp(true) ) { pErrMsg("Huh"); mRetErr; }
	    Coord crd( pos.coord() );
	    (dcid == -cNrPosCols ? crd.x : crd.y) = tbl_->getdValue( cell );
	    pos.set( crd );
	    poschgd = pos.binid_ != beforechgdr_.pos_.binid_;
	}
    }

    if ( poschgd )
    {
	rowToBeRemoved.trigger( drid );
	dps_.bivSet().remove( dps_.bvsPos(drid) );
	rowRemoved.trigger( drid );
    }

    bool setchg = dps_.setRow( afterchgdr_ ) || cell.col == sortcol_;
    if ( setchg )
    {
	dps_.dataChanged(); redoAll();
	setCurrent( afterchgdr_.pos_, dcid );
    }

    mGetRCIdx(dcid);
    delete runcalcs_.replace( rcidx, 0 );
    unsavedchgs_ = true;
    valueChanged.trigger();
}


void uiDataPointSet::eachChg( CallBacker* )
{
    if ( mIsUdf(plotpercentage_) ) return;

    float newpercentage = percfld_->getFValue();
    if ( newpercentage < 0 ) newpercentage = 0;
    if ( newpercentage == percentage_ )
	return;
    percentage_ = newpercentage;

    float neweachrow = (float)100/percentage_;
    if ( neweachrow < 1.0 ) neweachrow = 1.0;
    if ( !mIsEqual(neweachrow,eachrow_,mDefEps) )
    {
	eachrow_ = neweachrow;
	redoAll();
	if ( !dps_.isEmpty() )
	    setCurrent( 0, 0 );
    }
}


void uiDataPointSet::addRow( const DataPointSet::DataRow& datarow )
{
    dps_.addRow( datarow );
    dps_.dataChanged();
    unsavedchgs_ = true;
    reDoTable();
}


void uiDataPointSet::setSortCol( CallBacker* )
{
    setSortedCol( tColID() );
}


void uiDataPointSet::setSortedCol( TColID tid )
{
    if ( sortcol_ == tid ) return;

    sortcol_ = tid;
    DRowID drid = dRowID();
    redoAll();
    setCurrent( dColID(sortcol_), drid );
}


const char* uiDataPointSet::groupName( int idx ) const
{
    if ( idx < 1 || idx > grpnames_.size() )
	return "";
    return grpnames_.get( idx-1 );
}


bool uiDataPointSet::is2D() const
{
    return dps_.is2D();
}


bool uiDataPointSet::saveOK()
{
    if ( !unsavedchgs_ )
	return true;

    int res = uiMSG().askSave( "There are unsaved changes.\n"
			       "Do you want to save the data?" );
    if ( res == -1 )
	return false;
    else if ( res == 0 )
	return true;

    return doSave();
}


bool uiDataPointSet::rejectOK( CallBacker* )
{
    if ( !saveOK() )
	return false;
    return acceptOK( 0 );
}


bool uiDataPointSet::acceptOK( CallBacker* )
{
    removeSelPts( 0 );
    mDPM.release( dps_.id() );
    delete xplotwin_; delete statswin_;
    return true;
}


void uiDataPointSet::retrieve( CallBacker* )
{
    if ( !saveOK() ) return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg seldlg( this, ctio );
    seldlg.selGrp()->getManipGroup()->addButton( "manxplot",
	    	"Manage cross-plot data", mCB(this,uiDataPointSet,manage) );
    curseldlg_ = &seldlg;
    const bool selok = seldlg.go() && seldlg.ioObj();
    curseldlg_ = 0;
    if ( !selok ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    PosVecDataSet pvds;
    BufferString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    MouseCursorManager::restoreOverride();
    if ( !rv )
	{ uiMSG().error( errmsg ); return; }
    if ( pvds.data().isEmpty() )
	{ uiMSG().error("Selected data set is empty"); return; }
    DataPointSet* newdps = new DataPointSet( pvds, dps_.is2D(),
	    				     dps_.isMinimal() );
    if ( newdps->isEmpty() )
    { delete newdps; uiMSG().error("Data set is not suitable"); return; }

    setCaption( seldlg.ioObj()->name() );
    removeSelPts( 0 );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    tbl_->clearTable();
    dps_ = *newdps;
    grpnames_.erase();
    dps_.dataSet().pars().get( sKeyGroups, grpnames_ );

    const int nrcols = initVars();
    tbl_->setNrRows( size() );
    tbl_->setNrCols( nrcols );
    percfld_->setValue( percentage_ );

    redoAll();
    mkToolBars();
    MouseCursorManager::restoreOverride();
}


class uiDataPointSetSave : public uiDialog
{
public:

uiDataPointSetSave( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup("Create output","Specify output","111.0.1"))
    , ctio_(PosVecDataSetTranslatorGroup::ioContext())
    , type_(typ)
{
    ctio_.ctxt.forread = false;
    if ( !type_.isEmpty() )
	ctio_.ctxt.toselect.require_.set( sKey::Type(), typ );
    const CallBack tccb( mCB(this,uiDataPointSetSave,outTypChg) );

    tabfld_ = new uiGenInput( this, "Output to",
	    		BoolInpSpec(false,"Text file","OpendTect object") );
    tabfld_->valuechanged.notify( tccb );
    uiFileInput::Setup su;
    su.defseldir(GetDataDir()).forread(false).filter("*.txt");
    txtfld_ = new uiFileInput( this, "Output file", su );
    txtfld_->attach( alignedBelow, tabfld_ );
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, tabfld_ );

    postFinalise().notify( tccb );
}

~uiDataPointSetSave()
{
    delete ctio_.ioobj;
}

void outTypChg( CallBacker* )
{
    istab_ = tabfld_->getBoolValue();
    txtfld_->display( istab_ );
    selgrp_->display( !istab_ );
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool acceptOK( CallBacker* )
{
    istab_ = tabfld_->getBoolValue();
    if ( istab_ )
    {
	fname_ = txtfld_->fileName();
	if ( fname_.isEmpty() )
	    mErrRet("Please select the output file name")
    }
    else
    {
	if ( !selgrp_->processInput() )
	    mErrRet("Please enter a name for the output")
	ctio_.setObj( selgrp_->getCtxtIOObj().ioobj->clone() );
	if ( !type_.isEmpty() )
	{
	    ctio_.ioobj->pars().set( sKey::Type(), type_ );
	    IOM().commitChanges( *ctio_.ioobj );
	}
	fname_ = ctio_.ioobj->fullUserExpr(false);
    }

    return true;
}

    CtxtIOObj		ctio_;
    BufferString	fname_;
    BufferString	type_;
    uiGenInput*		tabfld_;
    uiFileInput*	txtfld_;
    uiIOObjSelGrp*	selgrp_;
    bool		istab_;
};


void uiDataPointSet::save( CallBacker* )
{
    doSave();
}


bool uiDataPointSet::doSave()
{
    if ( dps_.nrActive() < 1 ) return true;

    uiDataPointSetSave uidpss( this, storepars_.find(sKey::Type()) );
    if ( !uidpss.go() ) return false;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    DataPointSet savedps( dps_ );
    savedps.dataSet().pars() = storepars_;
    if ( !grpnames_.isEmpty() )
	savedps.dataSet().pars().set( sKeyGroups, grpnames_ );
    savedps.purgeInactive();
    BufferString errmsg;
    const bool ret = savedps.dataSet().
			putTo( uidpss.fname_, errmsg, uidpss.istab_ );
    MouseCursorManager::restoreOverride();
    if ( !ret )
	uiMSG().error( errmsg );
    else
	unsavedchgs_ = false;

    return ret;
}


void uiDataPointSet::manage( CallBacker* )
{
    uiDataPointSetMan dlg( this );
    dlg.go();
    MultiID mid;
    if ( curseldlg_ && curseldlg_->ioObj() )
	mid = curseldlg_->ioObj()->key();
    curseldlg_->selGrp()->fullUpdate( mid );
}


void uiDataPointSet::showSelPts( CallBacker* )
{
    if ( !dpsdisppropdlg_ )
    {
	dpsdisppropdlg_ =
	    new uiDPSDispPropDlg( xplotwin_, xplotwin_->plotter(),
				  dpsdispmgr_->dispProp() );
	dpsdisppropdlg_->windowClosed.notify(
		mCB(this,uiDataPointSet,showPtsInWorkSpace) );
    }

    dpsdisppropdlg_->go();
}


void uiDataPointSet::showPtsInWorkSpace( CallBacker* )
{
    if ( !dpsdispmgr_ || !dpsdisppropdlg_->uiResult() ) return;

    const uiDataPointSetCrossPlotter& plotter = xplotwin_->plotter();

    DataPointSetDisplayProp* dispprop;
    if ( dpsdisppropdlg_->type() )
    {
	ObjectSet<SelectionGrp> selgrps = plotter.selectionGrps();
	BufferStringSet selgrpnms;
	TypeSet<Color> selgrpcols;

	for ( int idx=0; idx<selgrps.size(); idx++ )
	{
	    selgrpnms.add( selgrps[idx]->name() );
	    selgrpcols += selgrps[idx]->col_;
	}

	dispprop = new DataPointSetDisplayProp( selgrpnms, selgrpcols );
    }
    else
	dispprop = new DataPointSetDisplayProp(
		dpsdisppropdlg_->ctSeq(), dpsdisppropdlg_->ctMapperSetup(),
		dps_.indexOf(dpsdisppropdlg_->colName()) );

    setDisp( dispprop );
}


void uiDataPointSet::setDisp( DataPointSetDisplayProp* dispprop )
{
    dpsdispmgr_->lock();
    dpsdispmgr_->clearDispProp();
    dpsdispmgr_->setDispProp( dispprop );
    
    int dpsid = dpsdispmgr_->getDisplayID(dps_);
    if ( dpsid < 0 )
	dpsid = dpsdispmgr_->addDisplay( dpsdispmgr_->availableViewers(), dps_);
    else
	dpsdispmgr_->updateDisplay( dpsdispmgr_->getDisplayID(dps_), dps_ );

    dpsdispmgr_->unLock();
}


void uiDataPointSet::removeSelPts( CallBacker* )
{
    if ( !dpsdispmgr_ ) return;

    const int dpsid = dpsdispmgr_->getDisplayID( dps_ );
    if ( dpsid < 0 ) return;

    dpsdispmgr_->removeDisplay( dpsid );
}


void uiDataPointSet::delSelRows( CallBacker* )
{
    if ( eachrow_ > 1 )
    {
	ObjectSet<uiTable::SelectionRange> tablerange =
	    tbl_->selectedRanges();
	if ( tablerange.size()>1 )
	{
	     if ( !uiMSG().askGoOn( "Only selected rows will be removed"
		     	      "\nThose rows falling in the same range"
			      "\nbut not displayed as only certain"
			      "\npercentage of data is displayed will not"
			      "\nbe removed."
			      "\nDo you want to go ahead with removal?" ) )
		 return;
	}
	else
	{
	    const int rep = uiMSG().askGoOnAfter("Do you want to remove rows"
				 "\nwhich fall in the same range but are not"
				 "\nselected or displayed as only certain"
				 "\npercentage of data is displayed or only the"
				 "\nselected & displayed ones ?", "Cancel",
				 "Delete all", "Delete only selected" );
	    if ( rep == 0 )
		removeHiddenRows();
	    if ( rep != 1 )
		return;
	}
    }

    int nrrem = 0;
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	if ( tbl_->isRowSelected(irow) )
	{
	    nrrem++;
	    if ( setup_.directremove_ )
		dps_.setInactive( dRowID(irow), true );
	    else
	    {
		rowToBeRemoved.trigger( dRowID(irow) );
		dps_.bivSet().remove( dps_.bvsPos(dRowID(irow)) );
		rowRemoved.trigger( dRowID(irow) );
	    }
	}
    }

    if ( !setup_.directremove_ )
	dps_.dataChanged();
    if ( nrrem < 1 )
    {
	uiMSG().message( "Please select the row(s) you want to remove."
			 "\nby clicking on the row label(s)."
			 "\nYou can select multiple rows by dragging,"
			 "\nor by holding down the shift key when clicking." );
	return;
    }

    mCleanRunCalcs;
    unsavedchgs_ = true;
    redoAll();
}


void uiDataPointSet::removeHiddenRows()
{
    const uiTable::SelectionRange* selrange =
	tbl_->selectedRanges()[0];
    Interval<float> valrange;
    valrange.start = tbl_->getfValue(
	    RowCol(selrange->firstrow_,sortcol_) );
    valrange.stop = tbl_->getfValue(
	    RowCol(selrange->lastrow_,sortcol_) );
    for ( int drowid=0; drowid<dps_.size(); drowid++ )
    {
	const Coord poscoord = dps_.coord( drowid );
	float val;
	
	if ( sortcol_ == 2 )
	    val = dps_.z( drowid );
	else if ( sortcol_ == 1 )
	    val = poscoord.y;
	else if ( sortcol_ == 0 )
	    val = poscoord.x;
	else
	    val = dps_.value( dColID(sortcol_), drowid );
	
	if ( valrange.includes(val,true) || (valrange.start==val)
					 || (valrange.stop==val) )
	    dps_.setInactive( drowid, true );
	continue;
	
    }

    mCleanRunCalcs;
    unsavedchgs_ = true;
    redoAll();
    return;
}


bool uiDataPointSet::isSelectionValid( DRowID rid ) const
{
    return xplotwin_ ? xplotwin_->plotter().isSelectionValid(rid) : false;
}


int uiDataPointSet::getSelectionGroupIdx( int selareaid ) const
{
    return xplotwin_ ? xplotwin_->plotter().getSelGrpIdx(selareaid) : -1;
}


void uiDataPointSet::calcSelectedness()
{
    uiDPSSelectednessDlg dlg( this, xplotwin_->plotter() );
    dlg.go();
}


void uiDataPointSet::addColumn( CallBacker* )
{
    uiDataPointSet::DColID dcid=-dps_.nrFixedCols()+1;
    BufferStringSet colnames;
    TypeSet<int> dcids;
    for ( ; dcid<dps_.nrCols(); dcid++ )
    {
	colnames.add( userName(dcid) );
	dcids += dcid;
    }

    uiDPSAddColumnDlg dlg( this, true );
    dlg.setColInfos( colnames, dcids );
    if ( dlg.go() )
    {
	dps_.dataSet().add(new DataColDef(dlg.newAttribName()));
	BinIDValueSet& bvs = dps_.bivSet();
	BinIDValueSet::Pos pos;
	while ( bvs.next(pos,false) )
	{
	    TypeSet<int> colids = dlg.usedColIDs();
	    MathExpression* mathobj = dlg.mathObject();
	    BinID curbid;
	    TypeSet<float> vals;
	    bvs.get( pos, curbid, vals );
	    DataPointSet::RowID rid = dps_.getRowID( pos );
	    
	    for ( int idx=0; idx<colids.size(); idx++ )
	    {
		const float yval = dps_.value( colids[idx], rid );
		mathobj->setVariableValue( idx, yval );
	    }

	    vals[ vals.size()-1 ] = mathobj->getValue();
	    bvs.set( pos, vals ); 
	}

	unsavedchgs_ = true;
	dps_.dataChanged();
	tbl_->setColumnLabel( tbl_->nrCols()-1, dlg.newAttribName() );
	reDoTable();
    }
}


void uiDataPointSet::removeColumn( CallBacker* )
{
    const DColID dcolid = dColID();
    if ( dcolid < 0 )
	return uiMSG().error( "Cannot remove this column" );

    unsavedchgs_ = true;
    tbl_->removeColumn( tbl_->currentCol() );
    dps_.dataSet().removeColumn( tColID(dcolid)+1 );
    dps_.dataChanged();
    reDoTable();
}


void uiDataPointSet::compVertVariogram( CallBacker* )
{
    const DColID dcid = dColID();
    if ( dcid<1 )
	return uiMSG().error( "Please select an attribute column" );

    dps_.dataSet().pars().set( sKeyGroups, grpnames_ );
    int nrgroups = 0;
    for ( DataPointSet::RowID irow=0; irow<dps_.size(); irow++ )
    {
	if ( dps_.group(irow) > nrgroups )
	    nrgroups = dps_.group(irow);
    }

    uiVariogramDlg varsettings( parent(), true );                              
    if ( !varsettings.go() )                                                    
	return;

    BufferString errmsg;
    bool msgiserror = true;
    VertVariogramComputer vvc( dps_, dcid, varsettings.getStep(),
			      varsettings.getMaxRg(), varsettings.getFold(),
	   		      nrgroups, errmsg, msgiserror );
    if ( !vvc.isOK() )
    {
	msgiserror ? uiMSG().error( errmsg.buf() )
	    	   : uiMSG().warning( errmsg.buf() );
	return;
    }

    uiVariogramDisplay* uivv = new uiVariogramDisplay( parent(), vvc.getData(),
	    					       vvc.getXaxes(),
	    					       vvc.getLabels(),
	   					       varsettings.getMaxRg(),
						       false );
    variodlgs_ += uivv;
    uivv->draw();
    uivv->go();
}
