/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uidatapointset.cc,v 1.1 2008-04-03 08:28:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidatapointset.h"
#include "uistatsdisplaywin.h"
#include "uidatapointsetcrossplotwin.h"

#include "datapointset.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "datacoldef.h"
#include "ctxtioobj.h"
#include "iopar.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "unitofmeasure.h"
#include "keystrs.h"

#include "uitable.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"


uiDataPointSet::Setup::Setup( const char* wintitl, bool ismodal )
    : uiDialog::Setup(wintitl?wintitl:"Extracted data","","0.0.0")
    , isconst_(true)
    , initialmaxnrlines_(4000)
{
    modal_ = ismodal;
}


static CallBackSet& creationCBS()
{
    static CallBackSet* cbs = 0;
    if ( !cbs ) cbs = new CallBackSet;
    return *cbs;
}


void uiDataPointSet::createNotify( const CallBack& cb )
{ creationCBS() += cb; }
void uiDataPointSet::stopCreateNotify( CallBacker* c )
{ creationCBS().removeWith( c ); }


#define mDPM DPM(DataPackMgr::PointID)

uiDataPointSet::uiDataPointSet( uiParent* p, const DataPointSet& dps,
				const uiDataPointSet::Setup& su )
	: uiDialog(p,su)
	, dps_(const_cast<DataPointSet&>(dps))
    	, setup_(su)
    	, sortcol_(-1)
    	, statscol_(-1)
    	, xcol_(-1), ycol_(-1), y2col_(-1)
    	, dispxy_(dps.is2D())
    	, zfac_(SI().zFactor())
    	, zunitnm_(SI().getZUnit(false))
	, tbl_(0)
	, xplotwin_(0)
	, statswin_(0)
{
    mDPM.obtain( dps_.id() );
    setCtrlStyle( LeaveOnly );
    runcalcs_.allowNull( true );
    const int nrcols = dps_.nrCols() + nrPosCols();
    for ( int idx=0; idx<nrcols+2; idx++ ) runcalcs_ += 0;
    eachrow_ = dps_.size() / setup_.initialmaxnrlines_;
    if ( eachrow_ < 1 ) eachrow_ = 1;

    calcIdxs();
    mkToolBar();

    uiLabel* titllbl = new uiLabel( this, dps.name() );
    titllbl->attach( hCentered );

    tbl_ = new uiTable( this, uiTable::Setup(size(),nrcols)
			  .rowgrow( false ).colgrow( false )
			  .selmode( uiTable::Multi )
			  .manualresize( true ) );
    tbl_->attach( ensureBelow, titllbl );

    setPrefWidth( 800 ); setPrefHeight( 600 );
    eachrow_ = -1; // force refill
    finaliseDone.notify( mCB(this,uiDataPointSet,eachChg) );
    creationCBS().doCall( this );
}


uiDataPointSet::~uiDataPointSet()
{
}


void uiDataPointSet::mkToolBar()
{
    maniptb_ = new uiToolBar( this, "Manip Tool bar" );
#define mAddButton(fnm,func,tip) \
    maniptb_->addButton( fnm, mCB(this,uiDataPointSet,func), tip )
    mAddButton( "axis-x.png", selXCol, "Set data for X" );
    mAddButton( "axis-add-y.png", selYCol, "Select as Y data" );
    mAddButton( "axis-rm-y.png", unSelCol, "UnSelect as Y data" );
    mAddButton( "axis-prev.png", colStepL, "Set Y one column left" );
    mAddButton( "axis-next.png", colStepR, "Set Y one column right" );
    mAddButton( "sortcol.png", setSortCol, "Set sorted column to current" );
#undef mAddButton

    disptb_ = new uiToolBar( this, "Display Tool bar" );

    uiGroup* grp = new uiGroup( disptb_, "Each grp" );
    eachfld_ = new uiSpinBox( grp, 0, "Each" );
    eachfld_->setValue( eachrow_ );
    eachfld_->setInterval( 1, mUdf(int), 1 );
    eachfld_->valueChanged.notify( mCB(this,uiDataPointSet,eachChg) );
    new uiLabel( grp, "Display each", eachfld_ );
    disptb_->addObject( grp->attachObj() );

#define mAddButton(fnm,func,tip,istogg) \
    disptb_->addButton( fnm, mCB(this,uiDataPointSet,func), tip, istogg )
    xyictbid_ = mAddButton( "toggxy.png", toggXYIC,
	    		    "Toggle (X,Y) vs I/X", true );
    xplottbid_ = mAddButton( "xplot.png", showCrossPlot,
	    		     "Show crossplot", false );
    statstbid_ = mAddButton( "statsinfo.png", showStatsWin,
			    "Provide statistical info on column", false );

    disptb_->turnOn( xyictbid_, is2D() );
}


void uiDataPointSet::updColNames()
{
    const int nrcols = tbl_->nrCols();
    const TColID zcid = nrPosCols()-1;
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
	tbl_->setColumnLabel( tid, colnm );
    }
}


void uiDataPointSet::calcIdxs()
{
    const int orgtblsz = drowids_.size();
    drowids_.erase(); trowids_.erase(); sortidxs_.erase();

    const int dpssz = dps_.size();
    for ( int did=0; did<dpssz; did++ )
    {
	const bool inact = dps_.isInactive(did);
	const bool hidden = did % eachrow_;
	if ( inact || hidden )
	    trowids_ += -1;
	else
	{
	    const TRowID tid = drowids_.size();
	    sortidxs_ += tid;
	    trowids_ += tid;
	    drowids_ += did;
	}
    }

    if ( sortcol_ >= 0 )
	calcSortIdxs();

    const int newtblsz = drowids_.size();
    if ( tbl_ && newtblsz != orgtblsz )
	tbl_->setNrRows( newtblsz );
}


void uiDataPointSet::calcSortIdxs()
{
    const DColID dcid = dColID( sortcol_ );
    TypeSet<float> vals;
    for ( TRowID trid=0; trid<sortidxs_.size(); trid++ )
    {
	DRowID drid = dRowID( trid );
	float val = getVal( dcid, drid );
	vals += val;
    }

    sort_coupled( vals.arr(), sortidxs_.arr(), sortidxs_.size() );
}


uiDataPointSet::DRowID uiDataPointSet::dRowID( TRowID tid ) const
{
    if ( tid < -1 ) tid = tbl_->currentRow();
    if ( tid < 0 ) return -1;
    return drowids_[ sortidxs_[tid] ];
}


uiDataPointSet::TRowID uiDataPointSet::tRowID( DRowID did ) const
{
    if ( did < -1 ) return tbl_->currentRow();
    else if ( did < 0 ) return -1;
    const TRowID itabrow = trowids_[did];
    if ( itabrow < 0 ) return -1;
    return sortidxs_[ itabrow ];
}


uiDataPointSet::DColID uiDataPointSet::dColID( TColID tid ) const
{
    return (tid < -1 ? tbl_->currentCol() : tid) - nrPosCols();
}


uiDataPointSet::TColID uiDataPointSet::tColID( DColID did ) const
{
    return did < -1-nrPosCols() ? tbl_->currentCol() : (did + nrPosCols());
}


void uiDataPointSet::fillPos( TRowID tid )
{
    const DataPointSet::Pos pos( dps_.pos(dRowID(tid)) );
    RowCol rc( tid, 0 );
    if ( is2D() )
    {
	tbl_->setValue( rc, pos.nr_ );
	rc.c()++;
    }
    if ( dispxy_ )
    {
	const Coord c( pos.coord() );
	tbl_->setValue( rc, c.x ); rc.c()++;
	tbl_->setValue( rc, c.y ); rc.c()++;
    }
    else
    {
	const BinID bid( pos.binID() );
	tbl_->setValue( rc, bid.inl ); rc.c()++;
	tbl_->setValue( rc, bid.crl ); rc.c()++;
    }
    float fz = zfac_ * pos.z_ * 100;
    int iz = mNINT(fz);
    tbl_->setValue( rc, iz * 0.01 );
}


void uiDataPointSet::fillData( TRowID tid )
{
    const DataPointSet::DataRow dr( dps_.dataRow(dRowID(tid)) );
    RowCol rc( tid, 3 );
    if ( is2D() ) rc.c()++;

    for ( int icol=0; icol<dr.data_.size(); icol++ )
	{ tbl_->setValue( rc, dr.data_[icol] ); rc.c()++; }
}


void uiDataPointSet::handleAxisColChg()
{
    updColNames();
    if ( xplotwin_ )
	xplotwin_->plotter().setCols( dColID(xcol_), dColID(ycol_),
					dColID(y2col_) );
    if ( ycol_ >= 0 && statswin_ )
	showStats( dColID(ycol_) );
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
    if ( ycol_ == -1 )
	ycol_ = tid;
    else
	y2col_ = tid;

    if ( prevy != ycol_ || prevy2 != y2col_ )
	handleAxisColChg();
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
}


void uiDataPointSet::colStepL( CallBacker* )
{
    if ( ycol_ < 1 ) return;
    ycol_--;
    handleAxisColChg();
}


void uiDataPointSet::colStepR( CallBacker* )
{
    if ( ycol_ < 0 || ycol_ >= tbl_->nrCols() - 1 )
	return;
    ycol_++;
    handleAxisColChg();
}


void uiDataPointSet::showCrossPlot( CallBacker* )
{
    if ( !xplotwin_ )
    {
	xplotwin_ = new uiDataPointSetCrossPlotWin( *this );
	uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
	xpl.selectionChanged.notify( mCB(this,uiDataPointSet,xplotSelChg) );
	xpl.removeRequest.notify( mCB(this,uiDataPointSet,xplotRemReq) );
	xplotwin_->windowClosed.notify( mCB(this,uiDataPointSet,xplotClose) );
    }

    disptb_->setSensitive( xplottbid_, false );
    handleAxisColChg();
    xplotwin_->show();
}


void uiDataPointSet::getXplotPos( uiDataPointSet::DRowID& drid,
				  uiDataPointSet::DColID& dcid ) const
{
    drid = -1; dcid = -nrPosCols()-1;
    if ( !xplotwin_ ) return;
    const uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
    drid = xpl.selRow();
    TColID tcid = xpl.isY2() ? y2col_ : ycol_;
    if ( tcid >= 0 )
	dcid = dColID( tcid );
}


void uiDataPointSet::xplotSelChg( CallBacker* )
{
    int drid, dcid; getXplotPos( drid, dcid );
    if ( drid < 0 || dcid < -nrPosCols() ) return;

    RowCol rc( tRowID(drid), tColID(dcid) );
    if ( rc.col >= 0 && rc.row >= 0 )
    {
	tbl_->setCurrentCell( rc );
	if ( statswin_ )
	{
	    const float val = getVal( dColID(statscol_), rc.row );
	    statswin_->setMarkValue( val, true );
	}
    }
}


void uiDataPointSet::xplotRemReq( CallBacker* )
{
    int drid, dcid; getXplotPos( drid, dcid );
    if ( drid < 0 ) return;
    dps_.setInactive( drid, true );
    const TRowID trid = tRowID( drid );
    if ( trid >= 0 )
	redoAll();
}


void uiDataPointSet::redoAll()
{
    calcIdxs();

    const int nrrows = tbl_->nrRows();
    for ( TRowID tid=0; tid<nrrows; tid++ )
    {
	fillPos( tid );
	fillData( tid );
    }

    updColNames();

    if ( statswin_ )
	showStats( dColID(statscol_) );
    if ( xplotwin_ )
	showCrossPlot( 0 );
}


void uiDataPointSet::xplotClose( CallBacker* )
{
    delete xplotwin_; xplotwin_ = 0;
    disptb_->setSensitive( xplottbid_, true );
}


void uiDataPointSet::statsClose( CallBacker* )
{
    delete statswin_; statswin_ = 0;
}


const char* uiDataPointSet::userName( uiDataPointSet::DColID did ) const
{
    if ( did >= 0 )
	return dps_.colName( did );
    else if ( did < -3 )
	return "Trace number";
    else if ( did == -1 )
	return "Z";
    else if ( dispxy_ )
	return did == -3 ? "X-Coord" : "Y-Coord";
    else
	return did == -3 ? "In-line" : "X-line";
}


Stats::RunCalc<float>& uiDataPointSet::getRunCalc(
				uiDataPointSet::DColID dcid ) const
{
    static Stats::RunCalc<float> empty( Stats::RunCalcSetup(false) );
    if ( dcid < -nrPosCols() ) return empty;

    int rcidx = dcid;
    if ( rcidx < 0 )
    {
	rcidx = dps_.nrCols() - 1 - dcid;
	if ( dispxy_ && (dcid == -2 || dcid == -3) )
	   rcidx = runcalcs_.size() - dcid - 4;
    }
    Stats::RunCalc<float>* rc = runcalcs_[rcidx];
    if ( !rc )
    {
	Stats::RunCalcSetup su( false );
#	define mReq(typ) require(Stats::typ)
	su.mReq(Count).mReq(Average).mReq(Median).mReq(StdDev);
	rc = new Stats::RunCalc<float>( su.mReq(Min).mReq(Max).mReq(RMS) );
	for ( DRowID drid=0; drid<dps_.size(); drid++ )
	    rc->addValue( getVal( dcid, drid ) );
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
    if ( dcid < -nrPosCols() ) return;
    statscol_ = tColID(dcid);

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
	statswin_ = new uiStatsDisplayWin( this, uiStatsDisplay::Setup() );
	statswin_->windowClosed.notify( mCB(this,uiDataPointSet,statsClose) );
    }
    statswin_->setData( rc );
    statswin_->setDataName( txt );
    statswin_->show();
}


float uiDataPointSet::getVal( DColID dcid, DRowID drid ) const
{
    if ( dcid >= 0 )
	return dps_.value( dcid, drid );
    else if ( dcid == -1 )
	return dps_.z( drid ) * zfac_;
    else if ( dcid < -3 )
	return dps_.trcNr( drid );

    if ( dispxy_ )
	return dcid == -3 ? dps_.coord(drid).x : dps_.coord(drid).y;

    return dcid == -3 ? dps_.binID(drid).inl : dps_.binID(drid).crl;
}


void uiDataPointSet::setVal( DColID dcid, DRowID drid, float val )
{
    if ( dcid < 0 ) return;
    dps_.getValues(drid)[dcid] = val;
}


void uiDataPointSet::eachChg( CallBacker* )
{
    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    if ( neweachrow != eachrow_ )
    {
	eachrow_ = neweachrow;
	redoAll();
    }
}


void uiDataPointSet::toggXYIC( CallBacker* )
{
    dispxy_ = disptb_->isOn( xyictbid_ );
    redoAll();
}


void uiDataPointSet::setSortCol( CallBacker* )
{
    TColID tid = tColID();
    if ( sortcol_ != tid )
    {
	sortcol_ = tColID();
	redoAll();
    }
}


bool uiDataPointSet::is2D() const
{
    return dps_.is2D();
}


bool uiDataPointSet::rejectOK( CallBacker* )
{
    mDPM.release( dps_.id() );
    delete xplotwin_;
    delete statswin_;
    return true;
}


bool uiDataPointSet::acceptOK( CallBacker* )
{
    mDPM.release( dps_.id() );
    return true;
}


void uiDataPointSet::save( CallBacker* )
{
    if ( dps_.nrActive() < 1 ) return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    ctio.ctxt.parconstraints.set( sKey::Type, "MVA Data" );
    ctio.ctxt.includeconstraints = true;
    uiIOObjSelDlg seldlg( this, ctio );
    uiGenInput* odstlfld = new uiGenInput( &seldlg, "Store style",
	    			BoolInpSpec(true,"OpendTect","Spreadsheet") );
    odstlfld->attach( alignedBelow, seldlg.selGrp() );
    if ( !seldlg.go() ) return;

    dps_.dataSet().pars() = storepars_;
    BufferString errmsg;
    if ( !dps_.dataSet().putTo(ctio.ioobj->fullUserExpr(false),errmsg,
    				!odstlfld->getBoolValue()) )
	uiMSG().error( errmsg );
}
