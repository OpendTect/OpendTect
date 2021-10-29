/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2008
________________________________________________________________________

-*/

#include "uidatapointset.h"

#include "ctxtioobj.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "dpsdispmgr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mathexpression.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "randcolor.h"
#include "settings.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "variogramcomputers.h"

#include "uibutton.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uidatapointsetman.h"
#include "uidatapointsetcrossplotwin.h"
#include "uidpsaddcolumndlg.h"
#include "uidpsselectednessdlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uistatsdisplaywin.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbar.h"
#include "uivariogram.h"
#include "od_helpids.h"

static const int cMinPtsForDensity = 20000;
static const char* sKeyGroups = "Groups";

mDefineInstanceCreatedNotifierAccess(uiDataPointSet)

class uiDPSDispPropDlg : public uiDialog
{ mODTextTranslationClass(uiDPSDispPropDlg)
public:
uiDPSDispPropDlg( uiParent* p, const uiDataPointSetCrossPlotter& plotter,
		  const DataPointSetDisplayProp* prevdispprop )
    : uiDialog(this,uiDialog::Setup(mJoinUiStrs(sDisplay(),sProperties()),
				    uiStrings::sEmptyString(),
				    mNoHelpKey).modal(false))
    , plotter_(plotter)
{
    BoolInpSpec binp( prevdispprop ? prevdispprop->showSelected() : false,
		      tr("Selected Points"),tr("All points with attribute") );
    typefld_ = new uiGenInput( this, uiStrings::sDisplay(),binp );
    typefld_->valuechanged.notify( mCB(this,uiDPSDispPropDlg,typeChangedCB) );

    BufferStringSet colnms;
    const DataPointSet& dps = plotter.dps();
    for ( int colidx=0; colidx<dps.nrCols(); colidx++ )
	colnms.add( dps.colName(colidx) );

    selfld_ = new uiLabeledComboBox(this, colnms, uiStrings::phrJoinStrings(
				    uiStrings::sAttribute(), tr("to display")));
    selfld_->attach( alignedBelow, typefld_ );
    if ( prevdispprop && !prevdispprop->showSelected() )
    {
	const char* attrnm = dps.colName( prevdispprop->dpsColID() );
	selfld_->box()->setCurrentItem( attrnm );
    }

    selfld_->box()->selectionChanged.notify(
			mCB(this,uiDPSDispPropDlg,attribChanged));

    coltabfld_ = new uiColorTableGroup( this, ColTab::Sequence("Rainbow"),
					OD::Horizontal, false );
    coltabfld_->attach( alignedBelow, selfld_ );
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
    const int bivsidx = dps.bivSetIdx( dps.indexOf(selfld_->box()->text()) );
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
{ return selfld_->box()->text(); }

const ColTab::Sequence& ctSeq() const
{ return coltabfld_->colTabSeq(); }

const ColTab::MapperSetup& ctMapperSetup() const
{ return coltabfld_->colTabMapperSetup(); }

    uiGenInput*				typefld_;
    uiLabeledComboBox*			selfld_;
    uiColorTableGroup*			coltabfld_;
    const uiDataPointSetCrossPlotter&	plotter_;
};



uiDataPointSet::Setup::Setup( const uiString& wintitl, bool ismodal )
    : uiDialog::Setup( wintitl.isSet() ? wintitl : tr("Extracted data"),
		       mNoDlgTitle, mODHelpKey(mDataPointSetHelpID) )
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
	, zfac_(mCast(float,SI().zDomain().userFactor()))
	, zunitnm_(SI().getUiZUnitString(false))
	, tbl_(0)
	, unsavedchgs_(false)
	, fillingtable_(true)
	, showbids_(false)
	, posdisptypechgd_(false)
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
	, showbidsfld_(0)
{
    windowClosed.notify( mCB(this,uiDataPointSet,closeNotify) );
    mAttachCB(IOM().applicationClosing,uiDataPointSet::applClosingCB);

    if ( mDPM.haveID(dps_.id()) )
	mDPM.obtain( dps_.id() );

    setCtrlStyle( CloseOnly );
    runcalcs_.allowNull( true );

    const int nrcols = initVars();
    mkToolBars();

    tbl_ = new uiTable( this, uiTable::Setup(size() ? size() : 10,nrcols)
			  .rowgrow( setup_.canaddrow_ ).removerowallowed(false)
			  .selmode( uiTable::Multi )
			  .manualresize( true ), "Data Table" );
    tbl_->valueChanged.notify( mCB(this,uiDataPointSet,valChg) );
    tbl_->rowClicked.notify( mCB(this,uiDataPointSet,rowClicked) );
    tbl_->rowInserted.notify( mCB(this,uiDataPointSet,rowAddedCB) );
    tbl_->selectionChanged.notify( mCB(this,uiDataPointSet,selChg) );
    tbl_->setTableReadOnly( setup_.isconst_ );
    tbl_->setLabelAlignment( Alignment::Left, true );
    dps_.dataSet().pars().get( sKeyGroups, grpnames_ );

    selPtsToBeShown.notify( mCB(this,uiDataPointSet,showSelPts) );
    setPrefWidth( 800 ); setPrefHeight( 600 );

    auto* xplotbut = new uiPushButton( topGroup(), tr("Show Cross-plot"),
		mCB(this,uiDataPointSet,showCrossPlot), true );
    xplotbut->setIcon( "xplot" );
    xplotbut->attach( centeredBelow, getDlgGroup() );
    xplotbut->attach( bottomBorder );

    postFinalise().notify( mCB(this,uiDataPointSet,initWin) );
    mTriggerInstanceCreatedNotifier();
}

int uiDataPointSet::nrPosCols() const
{ return dps_.nrFixedCols()-1; }

#define mCleanRunCalcs \
    deepErase( runcalcs_ ); \
    const int nrcols = dps_.nrCols() + nrPosCols(); \
    for ( int idx=0; idx<nrcols; idx++ ) \
	runcalcs_ += 0

#define mGetHPosVal( dcid, drid ) ( dcid == -nrPosCols() ) ? \
	( showbids_ ? dps_.binID(drid).inl() : dps_.coord(drid).x ) : \
	( showbids_ ? dps_.binID(drid).crl() : dps_.coord(drid).y )

int uiDataPointSet::initVars()
{
    sortcol_ = statscol_ = xcol_ = ycol_ = y2col_ = -1;
    if ( xplotwin_ )
    {
	xplotwin_->close();
	xplotwin_ = nullptr;
    }
    if ( statswin_ )
    {
	statswin_->close();
	statswin_ = nullptr;
    }

    mCleanRunCalcs;

    eachrow_ = mCast( float, dps_.nrActive() / setup_.initialmaxnrlines_ );
    if ( eachrow_ < 1.0 ) eachrow_ = 1.0;
    percentage_ = (float)100/eachrow_;

    calcIdxs();
    if ( tbl_ )
	disptb_->setSensitive( xplottbid_, true );
    return nrcols;
}


uiDataPointSet::~uiDataPointSet()
{
    detachAllNotifiers();
    deepErase( variodlgs_ );
    removeSelPts( 0 );
    if ( xplotwin_ )
	xplotwin_->close();
    if ( statswin_ )
	statswin_->close();
}


void uiDataPointSet::applClosingCB( CallBacker* )
{
    dpsdispmgr_ = 0;
}


void uiDataPointSet::chgPosDispType( CallBacker* )
{
    const bool showbids = !is2D() && showbidsfld_->isChecked();
    if ( showbids_ == showbids ) return;
    showbids_ = showbids;
    posdisptypechgd_ = true;
    mCleanRunCalcs;
    handleAxisColChg();
    reDoTable();
    posdisptypechgd_ = false;
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
	    iotb_ = new uiToolBar( this, toUiString("I/O Tool bar") );
	mAddButton( "save", save, uiStrings::phrSave(uiStrings::sData()) );
	if ( setup_.allowretrieve_ )
	    mAddButton( "open", retrieve, tr("Retrieve stored data") );
    }
#undef mAddButton

    if ( !maniptb_ )
	maniptb_ = new uiToolBar( this, toUiString("Manip Toolbar") );
#define mAddButton(fnm,func,tip) \
    maniptb_->addButton( fnm, tip, mCB(this,uiDataPointSet,func) )
    mAddButton( "axis-x", selXCol, tr("Set data for X") );
    mAddButton( "axis-add-y", selYCol, uiStrings::phrSelect(tr("as Y data")) );
    mAddButton( "axis-rm-y", unSelYCol, tr("UnSelect as Y data") );
    mAddButton( "delselrows", delSelRows, uiStrings::phrRemove
							(tr("selected rows")) );
    mAddButton( "axis-prev", colStepL, tr("Set Y one column left") );
    mAddButton( "axis-next", colStepR, tr("Set Y one column right") );
    mAddButton( "sortcol", setSortCol, tr("Set sorted column to current") );
    mAddButton( "plus", addColumn, m3Dots(uiStrings::phrAdd(tr("column"))) );
    mAddButton( "minus", removeColumn, uiStrings::phrRemove(tr("column")) );
#undef mAddButton

    if ( !disptb_ )
	disptb_ = new uiToolBar( this, mJoinUiStrs(sDisplay(), sToolbar()) );

    uiLabel* showlbl = new uiLabel( disptb_,uiStrings::sShow() );
    disptb_->addObject( showlbl );
    percfld_ = new uiSpinBox( disptb_, 1, "Each" );
    percfld_->setSuffix( toUiString("%") );
    percfld_->setInterval( (float)0.1, (float)100, (float)0.1 );
    percfld_->setValue( percentage_ );
    percfld_->setStretch( 0, 0 );
    percfld_->valueChanged.notify( mCB(this,uiDataPointSet,eachChg) );
    disptb_->addObject( percfld_ );

#define mAddButton(fnm,func,tip,istogg) \
    disptb_->addButton( fnm, tip, mCB(this,uiDataPointSet,func), istogg )
    dispxytbid_ = mAddButton( "toggxy", toggleXYZ,
			      tr("Toggle show X and Y columns"), true );
    dispztbid_ = mAddButton( "toggz", toggleXYZ,
			     tr("Toggle show Z column"), true );

    if ( !is2D() )
    {
	showbidsfld_ = new uiCheckBox( disptb_, tr("Show Inl/Crl") );
	showbidsfld_->activated.notify(mCB(this,uiDataPointSet,chgPosDispType));
	showbidsfld_->setChecked( showbids_ );
	disptb_->addObject( showbidsfld_ );
    }

    mAddButton( "statsinfo", showStatsWin,
			     tr("Show histogram and stats for column"), false );
    if ( dps_.group(0) < mUdf(od_uint16) && SI().zIsTime() )
	mAddButton( "variogram", compVertVariogram,
		    tr("Compute variogram for column"), false );
    xplottbid_ = mAddButton( "xplot", showCrossPlot,
			     tr("Show Cross-plot"), false );

    disptb_->turnOn( dispxytbid_, true ); disptb_->turnOn( dispztbid_, true );
}


void uiDataPointSet::showXY( bool yn )
{
    disptb_->turnOn( dispxytbid_, yn );
    toggleXYZ( 0 );
}

void uiDataPointSet::showZ( bool yn )
{
    disptb_->turnOn( dispztbid_, yn );
    toggleXYZ( 0 );
}

void uiDataPointSet::updColNames()
{
    const int nrcols = dps_.nrCols() + nrPosCols();
    const TColID zcid = 2;
    for ( TColID tid=0; tid<nrcols; tid++ )
    {
	uiString axnm;
	if ( tid == xcol_ )
	    axnm = toUiString("[%1]").arg(uiStrings::sX());
	if ( tid == ycol_ )
	    axnm = toUiString("[%1]").arg(uiStrings::sY());
	if ( tid == y2col_ )
	    axnm = toUiString("[%1]").arg(uiStrings::sY2());

	uiString colnm =
		tid == sortcol_ ? toUiString("*") : uiStrings::sEmptyString();
	if ( !axnm.isEmpty() )
	    colnm = axnm;

	if ( tid == zcid )
	    colnm = tr("%1 Z (%2)").arg(colnm).arg(zunitnm_);
	else
	    colnm = toUiString("%1 %2").arg(colnm).arg(userName(dColID(tid)));

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
	uiMSG().message( tr("DataPointSet too large, choose a subselection") );
	return;
    }

    trowids_.setSize( dpssz, -1 );
    int calcidx = 0;
    int dcountidx = 0;
    for ( int did=0; did<dpssz; did++ )
    {
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
    return tid - nrPosCols();
}


uiDataPointSet::TColID uiDataPointSet::tColID( DColID did ) const
{
    if ( did < -1-nrPosCols() ) return tbl_->currentCol();

    int ret = did + nrPosCols();
    if ( ret < 0 ) ret = -1;
    return ret;
}


void uiDataPointSet::fillPos( TRowID tid )
{
    fillingtable_ = true;
    const DRowID drid = dRowID(tid);
    const DataPointSet::Pos pos( dps_.pos(drid) );
    RowCol rc( tid, 0 );
    tbl_->setValue( rc, mGetHPosVal(-nrPosCols(),drid), 2 ); rc.col()++;
    tbl_->setValue( rc, mGetHPosVal(-nrPosCols()+1,drid), 2 ); rc.col()++;
    if ( mIsUdf(pos.z_) )
	tbl_->setText( rc, "" );
    else
    {
	float fz = zfac_ * pos.z_ * 100;
	int iz = mNINT32(fz);
	tbl_->setValue( rc, iz * 0.01 );
    }

    if ( dps_.is2D() )
    {
	rc.col()++;
	tbl_->setValue( rc, pos.nr_ );
    }

    BufferString rownm = groupName( dps_.group(dRowID(tid)) );
    if ( rownm.isEmpty() )
    {
	if ( is2D() )
	    rownm += pos.nr_;
	else
	    rownm += pos.binid_.toString();
    }
    tbl_->setRowLabel( tid, toUiString(rownm) );
    fillingtable_ = false;
}


void uiDataPointSet::fillData( TRowID tid )
{
    RowCol rc( tid, nrPosCols() );
    const DRowID drid = dRowID(tid);
    fillingtable_ = true;
    for ( DColID dcid=0; dcid<dps_.nrCols(); dcid++ )
	{ tbl_->setValue( rc, getVal(dcid,drid,true) ); rc.col()++; }
    fillingtable_ = false;
}


void uiDataPointSet::handleAxisColChg()
{
    updColNames();
    disptb_->setSensitive( xplottbid_, !xplotwin_ && (xcol_>=0 && (ycol_>=0 ||
								   y2col_>=0)));
    if ( xplotwin_ )
	xplotwin_->handleAxisChg( xcol_, ycol_, y2col_ );

}


void uiDataPointSet::initWin( CallBacker* c )
{
    setSortedCol( nrPosCols() );
    disptb_->setSensitive( xplottbid_, !xplotwin_ && (xcol_>=0 && (ycol_>=0 ||
								   y2col_>=0)));
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


void uiDataPointSet::unSelXCol()
{
    const TColID tid = tColID(); if ( tid < 0 ) return;
    xcol_ = -1;
    handleAxisColChg();
}


void uiDataPointSet::selYCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;

    const TColID prevy = ycol_; const TColID prevy2 = y2col_;
    int minptsfordensity = cMinPtsForDensity;
    Settings& setts = Settings::common();
    setts.get( sKeyMinDPPts(), minptsfordensity );

    if ( minptsfordensity <= 0 || mIsUdf(minptsfordensity) )
    {
	setts.set( sKeyMinDPPts(), cMinPtsForDensity ); setts.write();
	minptsfordensity = cMinPtsForDensity;
    }

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
	    uiString msg(tr("DataPoint set too large. The percentage of points "
			    "displayed should be modified for acceptable "
			    "performance.\n\nDo you want to change 'Plot each' "
			    "or do you want to continue with no Y2 ?"));

	    if (uiMSG().askGoOn(msg, tr("Change % Data displayed"),
				tr("Continue with no Y2")))
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

    if ( xplotwin_ )
    {
	if ( y2col_ == tid )
	    xplotwin_->setSelComboSensitive( true );

	xplotwin_->setGrpColors();
    }
}


void uiDataPointSet::unSelYCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;

    if ( tid == ycol_ )
	{ ycol_ = y2col_; y2col_ = -1; }
    else if ( tid == y2col_ )
	y2col_ = -1;
    else
	return;

    handleAxisColChg();

    if ( xplotwin_ )
    {
	if ( y2col_==-1 )
	    xplotwin_->setSelComboSensitive( false );

	xplotwin_->setGrpColors();
    }
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


void uiDataPointSet::rowClicked( CallBacker* cb )
{
    mCBCapsuleUnpack(int,trid,cb);
    setStatsMarker( dRowID(trid) );
    handleGroupChg( dRowID(trid) );
}


class uiSelectPosDlg : public uiDialog
{ mODTextTranslationClass(uiSelectPosDlg);
public:
uiSelectPosDlg( uiParent* p, const BufferStringSet& grpnames )
    : uiDialog( p, uiDialog::Setup(uiStrings::phrSelectPos(tr("for new row")),
				   mNoDlgTitle,mNoHelpKey) )
    , grpfld_(0)
{
    seltypefld_ = new uiGenInput( this, mJoinUiStrs(sPosition(), sType()),
			BoolInpSpec(true,tr("X/Y"),toUiString("%1/%2").arg(
			uiStrings::sInline()).arg(uiStrings::sCrossline())) );
    seltypefld_->valuechanged.notify( mCB(this,uiSelectPosDlg,selTypeChanged) );

    posinpfld_ = new uiGenInput( this,
				uiStrings::phrInput(uiStrings::sPosition()),
				PositionInpSpec(PositionInpSpec::Setup(true)) );
    posinpfld_->attach( leftAlignedBelow, seltypefld_ );

    uiString zinptxt = tr("%1 in %2").arg(uiStrings::sZValue()).arg(
		SI().zIsTime() ? uiStrings::sSec() : toUiString("%3/%4")
			.arg(uiStrings::sMeter()).arg(uiStrings::sFeet()));
    zinpfld_ = new uiGenInput( this, zinptxt, FloatInpSpec() );
    zinpfld_->attach( leftAlignedBelow, posinpfld_ );

    if ( grpnames.size()>1 )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
					  uiStrings::phrSelect(tr("group" )) );
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
	uiMSG().error(tr("Position entered is not valid."));
	return false;
    }
    if ( seltypefld_->getBoolValue() )
	pos.set( posinpfld_->getCoord() );
    else
	pos.set( posinpfld_->getBinID() );
    pos.z_ = zinpfld_->getFValue();

    datarow_ =
	DataPointSet::DataRow( pos, mCast( unsigned short,
				 !grpfld_ ? 1 : grpfld_->currentItem() +1 ) );
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
	const float newzval = dlg.datarow_.pos_.z_;
	const Coord3 newcoord( dlg.datarow_.coord(), newzval );
	const BinID newbid( dlg.datarow_.binID() );
	rowAdded.trigger();
	for ( int rownr=0; rownr<tbl_->nrRows(); rownr++ )
	{
	    const double xval = tbl_->getDValue(RowCol(rownr,0));
	    const double yval = tbl_->getDValue(RowCol(rownr,1));
	    const double zval = tbl_->getDValue(RowCol(rownr,2));
	    if ( showbids_ )
	    {
		const BinID bid( mCast(int,xval), mCast(int,yval) );
		if ( ( bid == newbid ) && mIsEqual(zval,newcoord.z,1e-4) )
		{
		    tbl_->ensureCellVisible( RowCol(rownr,0) );
		    break;
		}

	    }
	    else
	    {
		const Coord3 coord(xval,yval,zval/SI().zDomain().userFactor());
		if ( mIsEqual(coord.x,newcoord.x,2) &&
		     mIsEqual(coord.y,newcoord.y,2) &&
		     mIsEqual(coord.z,newcoord.z,1e-4) )
		{
		    tbl_->ensureCellVisible( RowCol(rownr,0) );
		    break;
		}
	    }
	}
    }
    else
	tbl_->removeRow( tbl_->newCell().row() );
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
	statusBar()->message( toUiString(txt), 0 );
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
    if ( xcol_<0 || ycol_<0 )
    {
	uiMSG().error( tr("Please select a column for %1")
		.arg(xcol_<0 ? "X" : "Y") );
	return;
    }

    if ( xplotwin_ )
	xplotwin_->plotter().dataChanged();
    else
    {
	xplotwin_ = new uiDataPointSetCrossPlotWin( *this );
	xplotwin_->setDeleteOnClose( true );
	uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
	xpl.selectionChanged.notify( mCB(this,uiDataPointSet,xplotSelChg) );
	xpl.removeRequest.notify( mCB(this,uiDataPointSet,xplotRemReq) );
	xplotwin_->plotter().pointsSelected.notify(
		mCB(this,uiDataPointSet,showStatusMsg) );
	xplotwin_->windowClosed.notify( mCB(this,uiDataPointSet,xplotClose) );
    }

    xplotwin_->setPercDisp( plotpercentage_ );
    handleAxisColChg();
    xplotwin_->show();
}


void uiDataPointSet::showStatusMsg( CallBacker* )
{
    if ( !xplotwin_ )
	return;

    uiString msg = tr("Y Selected: %1%2")
		 .arg(xplotwin_->plotter().nrYSels())
		 .arg(xplotwin_->plotter().isY2Shown()
		 ? tr("; Y2 Selected: %1").arg(xplotwin_->plotter().nrY2Sels())
		 : uiString::emptyString());
    xplotwin_->statusBar()->message( msg, 1 );
}


void uiDataPointSet::notifySelectedCell()
{
    if ( !xplotwin_ ) return;
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
    drid = -1; dcid = -nrPosCols()-1;
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
    if ( rc.col() >= 0 && rc.row() >= 0 )
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
    if ( drid < 0 || dcid < -nrPosCols() ) return;

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
    MouseCursorChanger mc( MouseCursor::Wait );
    reDoTable();

    if ( statswin_ )
	showStats( dColID(statscol_) );
    if ( xplotwin_ )
	showCrossPlot( 0 );
}


void uiDataPointSet::xplotClose( CallBacker* )
{
    xplotwin_ = nullptr;
    disptb_->setSensitive( xplottbid_, true );
}


void uiDataPointSet::statsClose( CallBacker* )
{
    statswin_ = nullptr;
}


#define mGetHPosName( dcid ) ( did == -nrPosCols() ) ? \
	( showbids_ ? sKey::Inline() : sKey::XCoord() ) : \
	( showbids_ ? sKey::Crossline() : sKey::YCoord() )

#define mIsZ( dcid ) dps_.is2D() ? dcid == -2 : dcid == -1

#define mIsTrcNr( dcid ) dps_.is2D() && dcid == -1


const char* uiDataPointSet::userName( uiDataPointSet::DColID did ) const
{
    if ( did >= 0 )
    {
	static BufferString colnm;
	colnm = dps_.colName( did );
	const UnitOfMeasure* uom = dps_.unit( did );
	const BufferString symbolstr = uom ? uom->symbol() : "";
	if ( !symbolstr.isEmpty() )
	    colnm.addSpace().add( "(" ).add( symbolstr.buf() ).add( ")" );

	return colnm.buf();
    }

    if ( mIsTrcNr(did) )
	return "Trace Nr";
    else if ( mIsZ(did) )
	return "Z";
    else
	return mGetHPosName( did );
}


#define mGetRCIdx(dcid) \
    int rcidx = dcid; \
    if ( rcidx < 0 ) rcidx = dps_.nrCols() - 1 - dcid

Stats::RunCalc<float>& uiDataPointSet::getRunCalc(
				uiDataPointSet::DColID dcid ) const
{
    static Stats::RunCalc<float> empty( Stats::CalcSetup(false) );
    if ( dcid < -nrPosCols() )
	return empty;

    const int tcid = tColID( dcid );
    if ( tcid<0 )
	return empty;

    while ( !runcalcs_.validIdx(tcid) )
	runcalcs_ += 0;

    Stats::RunCalc<float>* rc = runcalcs_[tcid];
    if ( !rc )
    {
	Stats::CalcSetup su( false );
#	define mReq(typ) require(Stats::typ)
	su.mReq(Count).mReq(Average).mReq(Median).mReq(StdDev);
	su.setNeedSorted();
	rc = new Stats::RunCalc<float>( su.mReq(Min).mReq(Max).mReq(RMS) );
	for ( DRowID drid=0; drid<dps_.size(); drid++ )
	{
	    if ( !dps_.isInactive(drid) )
		rc->addValue( getVal( dcid, drid, true ) );
	}
	runcalcs_.replace( tcid, rc );
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
	statswin_ =
	    new uiStatsDisplayWin( this, uiStatsDisplay::Setup(), 1, false );
	statswin_->windowClosed.notify( mCB(this,uiDataPointSet,statsClose) );
    }

    statswin_->setData( rc.medValsArr(), rc.count() );
    statswin_->setDataName( txt );
    statswin_->show();
}


float uiDataPointSet::getVal( DColID dcid, DRowID drid, bool foruser ) const
{
    if ( dcid >= 0 )
	return dps_.value( dcid, drid );

    if ( mIsTrcNr(dcid) )
	return mCast(float,dps_.pos( drid ).nr_);
    else if ( mIsZ(dcid) )
    {
	const float val = dps_.z( drid );
	if ( !foruser ) return val;
	return val * zfac_;
    }
    return mCast(float,mGetHPosVal(dcid,drid));
}


#define mRetErr fillPos( cell.row() ); return

void uiDataPointSet::valChg( CallBacker* )
{
    if ( fillingtable_ ) return;

    const RowCol& cell = tbl_->notifiedCell();
    if ( cell.row() < 0 || cell.col() < 0 ) return;

    const DColID dcid( dColID(cell.col()) );
    const DRowID drid( dRowID(cell.row()) );

    afterchgdr_ = beforechgdr_ = dps_.dataRow( drid );
    bool poschgd = false;

    if ( dcid >= 0 )
	afterchgdr_.data_[dcid] = tbl_->getFValue( cell );
    else
    {
	const char* txt = tbl_->text( cell );
	if ( !txt || !*txt )
	{
	    uiMSG().error( tr("Positioning values must be filled") );
	    mRetErr;
	}
	DataPointSet::Pos& pos( afterchgdr_.pos_ );

	if ( dcid == -1 )
	{
	    if ( !isDisp(false) ) { pErrMsg("Huh"); mRetErr; }
	    pos.z_ = tbl_->getFValue( cell ) / zfac_;
	    poschgd = !mIsZero(pos.z_-beforechgdr_.pos_.z_,1e-6);
	}
	else
	{
	    if ( !isDisp(true) ) { pErrMsg("Huh"); mRetErr; }
	    BinID bid( pos.binID() ); Coord crd( pos.coord() );
	    if ( showbids_ )
	    {
		int& posval = ( dcid == -nrPosCols() ) ? bid.inl() : bid.crl();
		posval = mCast(int,tbl_->getDValue(cell));
	    }
	    else
	    {
		double& posval = ( dcid == -nrPosCols() ) ? crd.x : crd.y;
		posval = tbl_->getDValue(cell);
	    }
	    showbids_ ? pos.set( bid ) : pos.set( crd );
	    poschgd = pos.binid_ != beforechgdr_.pos_.binid_;
	}
    }

    if ( poschgd )
    {
	rowToBeRemoved.trigger( drid );
	dps_.bivSet().remove( dps_.bvsPos(drid) );
	rowRemoved.trigger( drid );
    }

    bool setchg = dps_.setRow( afterchgdr_ ) || cell.col() == sortcol_;
    if ( setchg )
    {
	dps_.dataChanged(); redoAll();
	setCurrent( afterchgdr_.pos_, dcid );
    }

    const int tcid = tColID( dcid );
    delete runcalcs_.replace( tcid, 0 );
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

    int res = uiMSG().askSave( tr("There are unsaved changes."
			       "\n\nDo you want to save the data?") );
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

    return acceptOK( nullptr );
}


bool uiDataPointSet::acceptOK( CallBacker* )
{
    removeSelPts( nullptr );
    mDPM.release( dps_.id() );

    closeAndZeroPtr( xplotwin_ );
    closeAndZeroPtr( statswin_ );
    return true;
}


void uiDataPointSet::retrieve( CallBacker* )
{
    if ( !saveOK() ) return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    ctio.ctxt_.setName( "Cross-plot Data" );
    uiIOObjSelDlg seldlg( this, ctio );
    curseldlg_ = &seldlg;
    const bool selok = seldlg.go() && seldlg.ioObj();
    curseldlg_ = 0;
    if ( !selok )
	return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    PosVecDataSet pvds;
    BufferString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    MouseCursorManager::restoreOverride();
    if ( !rv )
    {
	uiMSG().error( mToUiStringTodo(errmsg) );
	return;
    }

    if ( pvds.data().isEmpty() )
    {
	uiMSG().error(uiDataPointSetMan::sSelDataSetEmpty());
	return;
    }

    MouseCursorManager::setOverride( MouseCursor::Wait );
    DataPointSet* newdps = new DataPointSet( pvds, dps_.is2D(),
					     dps_.isMinimal() );
    if ( newdps->isEmpty() )
    {
	delete newdps;
	uiMSG().error( tr("Data set is not suitable") );
	return;
    }

    setCaption( tr("Cross-plot Data: %1").arg(seldlg.ioObj()->uiName()) );
    removeSelPts( 0 );
    tbl_->clearTable();
    dps_ = *newdps;
    delete newdps;
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
{ mODTextTranslationClass(uiDataPointSetSave);
public:

uiDataPointSetSave( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup(tr("Save Cross-plot Data"),
				 mNoDlgTitle,
				 mODHelpKey(mdataPointSetSaveHelpID) ))
    , ctio_(PosVecDataSetTranslatorGroup::ioContext())
    , type_(typ)
{
    ctio_.ctxt_.forread_ = false;
    if ( !type_.isEmpty() )
	ctio_.ctxt_.toselect_.require_.set( sKey::Type(), typ );
    const CallBack tccb( mCB(this,uiDataPointSetSave,outTypChg) );

    tabfld_ = new uiGenInput( this, tr("Output to"),
		BoolInpSpec(false,tr("Text file"),
			    tr("OpendTect Cross-plot Data")) );
    tabfld_->valuechanged.notify( tccb );

    txtfld_ = new uiASCIIFileInput( this, false );
    txtfld_->attach( alignedBelow, tabfld_ );

    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, tabfld_ );

    postFinalise().notify( tccb );
}

~uiDataPointSetSave()
{
    delete ctio_.ioobj_;
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
	    mErrRet(tr("Please select the output file name"))
    }
    else
    {
	if ( !selgrp_->updateCtxtIOObj() )
	    mErrRet(tr("Please enter a name for the output"))
	ctio_.setObj( selgrp_->getCtxtIOObj().ioobj_->clone() );
	if ( !type_.isEmpty() )
	{
	    ctio_.ioobj_->pars().set( sKey::Type(), type_ );
	    IOM().commitChanges( *ctio_.ioobj_ );
	}
	fname_ = ctio_.ioobj_->fullUserExpr(false);
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
    const bool ret =
	savedps.dataSet().putTo( uidpss.fname_, errmsg, uidpss.istab_ );
    MouseCursorManager::restoreOverride();
    uiMainWin* mw = uiMSG().setMainWin( this );
    if ( !ret )
	uiMSG().error( mToUiStringTodo(errmsg) );
    else
    {
	unsavedchgs_ = false;
	if ( uidpss.ctio_.ioobj_ && !uidpss.istab_ )
	    setCaption( uidpss.ctio_.ioobj_->uiName() );

	uiMSG().message( tr("Cross-plot Data successfully saved") );
    }
    uiMSG().setMainWin( mw );

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
	mAttachCB(dpsdisppropdlg_->windowClosed,
		  uiDataPointSet::showPtsInWorkSpace);
    }

    dpsdisppropdlg_->go();
}


void uiDataPointSet::showPtsInWorkSpace( CallBacker* )
{
    if ( !dpsdispmgr_ || !dpsdisppropdlg_->uiResult() || !xplotwin_ ) return;

    const uiDataPointSetCrossPlotter& plotter = xplotwin_->plotter();

    DataPointSetDisplayProp* dispprop;
    if ( dpsdisppropdlg_->type() )
    {
	ObjectSet<SelectionGrp> selgrps = plotter.selectionGrps();
	BufferStringSet selgrpnms;
	TypeSet<OD::Color> selgrpcols;

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
	    if (!uiMSG().askGoOn(
			tr( "Only selected rows will be removed."
			    "\nThe rows in that range that "
			    "are not displayed will not be removed."
			    "\n\nDo you want to go ahead with removal?")))
		 return;
	}
	else
	{
	    const int rep = uiMSG().askGoOnAfter(
		    tr("Do you want to remove rows which fall in the same "
			"range but are not selected or displayed?\n(only a "
			"certain percentage of data is displayed!)"),
				uiStrings::sCancel(), tr("Delete all"),
				tr("Delete only selected") );
	    if ( rep == 1 )
		removeHiddenRows();
	    if ( rep != 0 )
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
	uiMSG().message(tr("Please select the row(s) you want to remove."
			"\nby clicking on the row label(s)."
			"\nYou can select multiple rows by dragging,"
			"\nor by holding down the shift key when clicking."));
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
    valrange.start = tbl_->getFValue(
	    RowCol(selrange->firstrow_,sortcol_) );
    valrange.stop = tbl_->getFValue(
	    RowCol(selrange->lastrow_,sortcol_) );
    for ( int drowid=0; drowid<dps_.size(); drowid++ )
    {
	float val;
	if ( sortcol_ == 2 )
	    val = dps_.z( drowid );
	else if ( sortcol_ == 1 )
	    val = mCast(float,mGetHPosVal(-nrPosCols()+1,drowid));
	else if ( sortcol_ == 0 )
	    val = mCast(float,mGetHPosVal(-nrPosCols(),drowid));
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
    if ( !xplotwin_ ) return;
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
	BinIDValueSet::SPos pos;
	TypeSet<int> colids = dlg.usedColIDs();
	Math::Expression* mathobj = dlg.mathObject();
	if ( !mathobj ) return;
	while ( bvs.next(pos,false) )
	{
	    BinID curbid;
	    TypeSet<float> vals;
	    bvs.get( pos, curbid, vals );
	    DataPointSet::RowID rid = dps_.getRowID( pos );

	    for ( int idx=0; idx<colids.size(); idx++ )
	    {
		float yval = mUdf(float);
		if ( colids[idx] >= 0 )
		    yval = dps_.value( colids[idx], rid );
		else if ( colids[idx] == -1 )
		{
		    yval = dps_.z( rid );
		    if ( !mIsUdf(yval) )
			yval *= zfac_;
		}
		else
		    yval = mCast(float,mGetHPosVal(colids[idx],rid));

		mathobj->setVariableValue( idx, yval );
	    }

	    vals[ vals.size()-1 ] = mCast(float,mathobj->getValue());
	    bvs.set( pos, vals );
	}

	unsavedchgs_ = true;
	dps_.dataChanged();
	tbl_->setColumnLabel(
		tbl_->nrCols()-1, toUiString(dlg.newAttribName()) );
	reDoTable();
    }
}


void uiDataPointSet::removeColumn( CallBacker* )
{
    const DColID dcolid = dColID();
    const TColID tcolid = tColID( dcolid );
    if ( dcolid < 0 )
	return uiMSG().error(tr("Cannot remove this column"));
    if ( tcolid == xcol_ || tcolid == ycol_ || tcolid == y2col_ )
    {
	uiStringSet options;
	if (tcolid == xcol_) options += uiStrings::sX();
	if (tcolid == ycol_) options += uiStrings::sY();
	if (tcolid == y2col_) options += uiStrings::sY2();

	uiString msg = tr("This column is selected as data for %1 "
			  "axis for the cross-plot. Removing the column "
			  "will unselect it. Do you really "
			  "want to remove this column?")
		     .arg(options.createOptionString(true));

	if ( !uiMSG().askGoOn(msg) )
	    return;
	else
	{
	    if ( tcolid == y2col_ ) unSelYCol( 0 );
	    if ( tcolid == ycol_ ) unSelYCol( 0 );
	    if ( tcolid == xcol_ ) unSelXCol();
	}
    }

    unsavedchgs_ = true;
    tbl_->removeColumn( tbl_->currentCol() );
    dps_.dataSet().removeColumn( tcolid+1 );
    if ( xcol_>tcolid ) xcol_--;
    if ( ycol_>tcolid ) ycol_--;
    if ( y2col_>tcolid ) y2col_--;
    mCleanRunCalcs;
    dps_.dataChanged();
    reDoTable();
}


void uiDataPointSet::compVertVariogram( CallBacker* )
{
    const DColID dcid = dColID();
    if ( dcid<1 )
	return uiMSG().error( tr("Please select an attribute column") );

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
	msgiserror ? uiMSG().error( mToUiStringTodo(errmsg) )
		   : uiMSG().warning( mToUiStringTodo(errmsg) );
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
