/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
________________________________________________________________________

-*/

#include "uiseissampleeditor.h"

#include "uiamplspectrum.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseistrcbufviewer.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"

#include "trckeyzsampling.h"
#include "datapack.h"
#include "executor.h"
#include "arrayndimpl.h"
#include "filepath.h"
#include "dbman.h"
#include "ranges.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "seisioobjinfo.h"
#include "zdomain.h"
#include "od_helpids.h"


class uiSeisSampleEditorInfoVwr : public uiAmplSpectrum
{ mODTextTranslationClass(uiSeisSampleEditorInfoVwr);
public :

			uiSeisSampleEditorInfoVwr(uiParent*,const SeisTrc&,bool,
				const ZDomain::Def&);

    void		setTrace(const SeisTrc&);

protected:

    bool		is2d_;
    const ZDomain::Def&	zdomdef_;

    uiGenInput*		coordfld_;
    uiGenInput*		trcnrbinidfld_;
    uiGenInput*		minamplfld_;
    uiGenInput*		maxamplfld_;
    uiGenInput*		minamplatfld_;
    uiGenInput*		maxamplatfld_;

};


uiSeisSampleEditor::Setup::Setup( const DBKey& ky, Seis::GeomType gt )
    : uiDialog::Setup(uiString::emptyString(),mNoDlgTitle,
                      mODHelpKey(mSeisBrowserHelpID) )
    , id_(ky)
    , startpos_(mUdf(int),mUdf(int))
    , startz_(mUdf(float))
    , readonly_(false)
{
    wintitle_ = tr( "Browse '%1'" ).arg( DBM().nameOf( id_ ) );
}


#define mRetNoGo(msg) \
{ \
    new uiLabel( this, msg ); \
    setCtrlStyle( CloseOnly ); \
    delete prov_; \
    return; \
}


uiSeisSampleEditor::uiSeisSampleEditor( uiParent* p, const Setup& su )
    : uiDialog(p,su)
    , prov_(0)
    , tbl_(0)
    , toolbar_(0)
    , tbuf_(*new SeisTrcBuf(false))
    , changedtraces_(*new Pos::IdxPairDataSet(sizeof(SeisTrc*),false,false))
    , cubedata_(*new PosInfo::CubeData)
    , linedata_(*new PosInfo::Line2DData)
    , crlwise_(false)
    , stepout_(25)
    , compnr_(0)
    , sampling_(0.f,1.f)
    , infovwr_(0)
    , trcbufvwr_(0)
    , setup_(su)
    , zdomdef_(&ZDomain::SI())
    , toinlwisett_(tr("Switch to Inline-wise display"))
    , tocrlwisett_(tr("Switch to Crossline-wise display"))
{
    uiRetval uirv;
    prov_ = Seis::Provider::create( su.id_, &uirv );
    if ( !prov_ )
	mRetNoGo( uirv )

    uirv = prov_->getComponentInfo( compnms_, datatype_ );
    if ( compnms_.isEmpty() )
    {
	if ( uirv.isOK() )
	    compnms_.add( "Component 1" );
	else
	    mRetNoGo( uirv )
    }

    is2d_ = prov_->is2D();
    ZSampling zrg = prov_->getZRange();
    sampling_.start = zrg.start;
    sampling_.step = zrg.step;
    nrsamples_ = zrg.nrSteps() + 1;

    if ( !is2d_ )
	prov3D().getGeometryInfo( cubedata_ );
    else
    {
	int linenr = prov2D().lineNr( setup_.geomid_ );
	if ( linenr < 0 )
	    mRetNoGo( tr("Cannot find line (ID=%1)").arg(setup_.geomid_) )
	prov_->getGeometryInfo( linenr, linedata_ );
    }

    if ( (is2d_ ? linedata_.isEmpty() : cubedata_.isEmpty()) )
	mRetNoGo( tr("'%1' is empty").arg(prov_->name()) )


    // OK ... I guess we can do our thing

    if ( is2d_ )
	stepbid_ = BinID( 1, linedata_.minStep() );
    else
	stepbid_ = cubedata_.minStep();

    createMenuAndToolBar();
    createTable();

    doSetPos( su.startpos_, true );
    setZ( su.startz_ );
    tbl_->selectionChanged.notify(
	    mCB(this,uiSeisSampleEditor,trcselectionChanged) );
}


uiSeisSampleEditor::~uiSeisSampleEditor()
{
    clearEditedTraces();
    deepErase( tbuf_ );
    delete prov_;
    delete &tbuf_;
    delete &cubedata_;
    delete &linedata_;
}


void uiSeisSampleEditor::clearEditedTraces()
{
    Pos::IdxPairDataSet::SPos spos;
    while ( changedtraces_.next(spos) )
	delete (SeisTrc*)changedtraces_.getObj( spos );
    changedtraces_.setEmpty();
}


Seis::Provider2D& uiSeisSampleEditor::prov2D()
{
    return *static_cast<Seis::Provider2D*>( prov_ );
}


Seis::Provider3D& uiSeisSampleEditor::prov3D()
{
    return *static_cast<Seis::Provider3D*>( prov_ );
}


const BinID& uiSeisSampleEditor::curBinID() const
{
    return ctrc_.info().binID();
}


float uiSeisSampleEditor::curZ() const
{
    return sampling_.start + tbl_->currentRow() * sampling_.step;
}


void uiSeisSampleEditor::setZ( float z )
{
    if ( mIsUdf(z) ) return;

    int newrow = (int)((int)z - (int)sampling_.start) / (int)sampling_.step;
    tbl_->setCurrentCell( RowCol(tbl_->currentCol(),newrow) );
}


#define mAddButton(fnm,func,tip,toggle) \
    toolbar_->addButton( fnm, tip, mCB(this,uiSeisSampleEditor,func), toggle )

void uiSeisSampleEditor::createMenuAndToolBar()
{
    toolbar_ = new uiToolBar( this, tr("Sample Editor Tool Bar") );
    mAddButton( "gotopos",goToPush,tr("Goto position"),false );
    mAddButton( "info",infoPush,tr("Information"),false );
    if ( !is2d_ )
	crlwisebutidx_ = mAddButton( "crlwise", switchViewTypePush,
				     tocrlwisett_, true );
    mAddButton( "leftarrow",leftArrowPush,tr("Move left"),false );
    mAddButton( "rightarrow",rightArrowPush,tr("Move right"),false );
    showwgglbutidx_ = mAddButton( "wva",dispTracesPush,
				  tr("Display current traces"),false );

    if ( compnms_.size() > 1 )
    {
	selcompnmfld_ = new uiComboBox( toolbar_, compnms_.getUiStringSet(),
							    "Component name" );
	toolbar_->addObject( selcompnmfld_ );
	selcompnmfld_->setCurrentItem( compnr_ );
	selcompnmfld_->selectionChanged.notify(
				    mCB(this,uiSeisSampleEditor,chgCompNrCB) );
    }

    uiLabel* lbl = new uiLabel( toolbar_, tr("Nr traces") );
    toolbar_->addObject( lbl );
    nrtrcsfld_ = new uiSpinBox( toolbar_ );
    nrtrcsfld_->setInterval( StepInterval<int>(3,99999,2) );
    nrtrcsfld_->doSnap( true );
    nrtrcsfld_->setValue( 2*stepout_+1 );
    nrtrcsfld_->valueChanged.notify(
			    mCB(this,uiSeisSampleEditor,nrTracesChgCB) );
    toolbar_->addObject( nrtrcsfld_ );
}


void uiSeisSampleEditor::createTable()
{
    const int nrrows = nrsamples_;
    const int nrcols = 2*stepout_ + 1;
    tbl_ = new uiTable( this, uiTable::Setup( nrrows, nrcols )
			     .selmode(uiTable::Multi)
			     .manualresize( true ), "Seismic data" );

    tbl_->valueChanged.notify( mCB(this,uiSeisSampleEditor,tblValChgCB) );
    tbl_->setStretch( 1, 1 );
    tbl_->setPrefHeight( 400 );
    tbl_->setPrefWidth( 600 );
    tbl_->setTableReadOnly( setup_.readonly_ );
}


BinID uiSeisSampleEditor::getNextBid( const BinID& cur, int idx,
				   bool before ) const
{
    return crlwise_
	? BinID( cur.inl() + (before?-1:1)*stepbid_.inl()*idx, cur.crl() )
	: BinID( cur.inl(), cur.crl() + (before?-1:1)*stepbid_.crl()*idx );
}


void uiSeisSampleEditor::addTrc( SeisTrcBuf& tbuf, const BinID& bid )
{
    SeisTrc* newtrc = new SeisTrc;
    const int chgbufidx = tbufchgdtrcs_.find( bid, false );
    if ( chgbufidx >= 0 )
	*newtrc = *tbufchgdtrcs_.get( chgbufidx );
    else if ( !tr_->goTo(bid) || !tr_->read(*newtrc) )
    {
	newtrc->info().setBinID( bid );
	newtrc->info().coord_ = SI().transform( bid );
	fillUdf( *newtrc );
    }
    tbuf.add( newtrc );
}


void uiSeisSampleEditor::setPos( const BinID& bid )
{
    doSetPos( bid, false );
}


void uiSeisSampleEditor::doSetPos( const BinID& bid, bool force )
{
    if ( !tbl_ )
	return false;
    if ( !force && bid == ctrc_.info().binID() )
	return true;

    NotifyStopper notifstop( tbl_->valueChanged );

    commitChanges();
    BinID binid( bid );
    const bool inlok = is2d_ || !mIsUdf(bid.inl());
    const bool crlok = !mIsUdf(bid.crl());
    if ( inlok && crlok )
    {
	if ( is2d_ )
	    binid.crl() = linedata_.nearestNumber( bid.crl() );
	else
	    binid = cubedata_.nearestBinID( bid );
    }
    else
    {
	if ( is2d_ )
	    binid.crl() = linedata_.centerNumber();
	else
	    binid = posinfo_.centerPos();
    }

    tbuf_.deepErase();



    const bool havetrc = tr_->goTo( binid );
    const bool canread = havetrc && tr_->read( ctrc_ );
    if ( !canread )
    {
	if ( !veryfirst )
	    uiMSG().error( tr("Cannot read data at specified location") );
	else
	    fillUdf( ctrc_ );
    }
    if ( !havetrc || !canread )
	binid = ctrc_.info().binID();

    for ( int idx=1; idx<stepout_+1; idx++ )
    {
	addTrc( tbufbefore_, getNextBid(binid,idx,true) );
	addTrc( tbufafter_, getNextBid(binid,idx,false) );
    }

    for ( int idx=0; idx<stepout_; idx++ )
	tbuf_.add( tbufbefore_.get(stepout_-idx-1) );
    tbuf_.add( &ctrc_ );
    for ( int idx=0; idx<stepout_; idx++ )
	tbuf_.add( tbufafter_.get(idx) );

    fillTable();
    return true;
}


bool uiSeisSampleEditor::is2D() const
{
    return tr_->is2D();
}


void uiSeisSampleEditor::setStepout( int nr )
{
    stepout_ = nr;
    nrtrcsfld_->setValue( nr*2+1 );
}


void uiSeisSampleEditor::fillUdf( SeisTrc& trc )
{
    while ( trc.nrComponents() > nrComponents() )
	trc.data().delComponent(0);
    while ( trc.nrComponents() < nrComponents() )
	trc.data().addComponent( nrsamples_, DataCharacteristics() );

    trc.reSize( nrsamples_, false );

    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<nrsamples_; isamp++ )
	    trc.set( isamp, mUdf(float), icomp );
    }
}


static BufferString getZValStr( float z, int zfac )
{
    BufferString txt;
    float dispz = zfac * z * 10;
    int idispz = mNINT32( dispz );
    dispz = idispz * 0.1f;
    txt = dispz;
    return txt;
}


void uiSeisSampleEditor::fillTable()
{
    const CBVSInfo& info = tr_->readMgr()->info();
    const int zfac = zdomdef_->userFactor();
    const BufferString zunstr = zdomdef_->unitStr(false).getFullString();
    for ( int idx=0; idx<info.nrsamples_; idx++ )
    {
	const BufferString zvalstr( getZValStr(info.sd_.atIndex(idx),zfac) );
	tbl_->setRowLabel( idx, toUiString(zvalstr) );
	uiString tt;
	tt = toUiString("%1 %2 sample at %3 %4").arg(idx+1)
			.arg(getRankPostFix(idx+1)).arg(zvalstr).arg(zunstr);
	tbl_->setRowToolTip( idx, tt );
    }

    for ( int idx=0; idx<tbuf_.size(); idx++ )
    {
	const SeisTrc& buftrc = *tbuf_.get(idx);
	const int chidx = tbufchgdtrcs_.find(buftrc.info().binID(),is2D());
	if ( chidx < 0 )
	    fillTableColumn( buftrc, idx );
	else
	    fillTableColumn( *(tbufchgdtrcs_.get(chidx)), idx );
    }

    tbl_->resizeRowsToContents();
    const int middlecol = tbuf_.size()/2;
    tbl_->selectColumn( middlecol );
    tbl_->ensureCellVisible( RowCol(0,middlecol) );
}


void uiSeisSampleEditor::fillTableColumn( const SeisTrc& trc, int colidx )
{
    tbl_->setColumnLabel( colidx,
			  toUiString(trc.info().binID().toString(is2D())) );

    RowCol rc; rc.col() = colidx;
    for ( rc.row()=0; rc.row()<nrsamples_; rc.row()++ )
    {
	const float val = trc.get( rc.row(), compnr_ );
	tbl_->setValue( rc, val );
    }
}


class uiSeisSampleEditorGoToDlg : public uiDialog
{ mODTextTranslationClass(uiSeisSampleEditorGoToDlg);
public:

uiSeisSampleEditorGoToDlg( uiParent* p, BinID cur, bool is2d, bool isps=false )
    : uiDialog( p, uiDialog::Setup(tr("Reposition"),
				   tr("Specify a new position"),
				   mNoHelpKey) )
{
    PositionInpSpec inpspec(
	    PositionInpSpec::Setup(false,is2d,isps).binid(cur) );
    posfld_ = new uiGenInput( this, tr("New Position"),
			      inpspec.setName("Inline",0)
				     .setName("Crossline",1) );
}

bool acceptOK()
{
    pos_ = posfld_->getBinID();
    if ( !SI().isReasonable(pos_) )
    {
	uiMSG().error(uiStrings::phrSpecify(tr("a valid position")));
	return false;
    }

    return true;
}

    BinID	pos_;
    uiGenInput*	posfld_;

};


bool uiSeisSampleEditor::goTo( const BinID& bid )
{
    return doSetPos( bid, true );
}


void uiSeisSampleEditor::infoPush( CallBacker* )
{
    const SeisTrc& trc = tbl_->currentCol()<0 ? ctrc_
					      : *tbuf_.get(tbl_->currentCol());
    if ( !infovwr_ )
    {
	infovwr_ = new uiSeisSampleEditorInfoVwr( this, trc, is2d_, *zdomdef_ );
	infovwr_->windowClosed.notify( mCB(this,uiSeisSampleEditor,infoClose) );
    }
    infovwr_->setTrace( trc );
    infovwr_->show();
}


void uiSeisSampleEditor::infoClose( CallBacker* )
{
    infovwr_ = 0;
}


void uiSeisSampleEditor::trcselectionChanged( CallBacker* )
{
    if ( infovwr_ )
    {
	const SeisTrc& trc = tbl_->currentCol()<0 ? ctrc_
				      : *tbuf_.get(tbl_->currentCol());
	infovwr_->setTrace( trc );
    }
}


void uiSeisSampleEditor::goToPush( CallBacker* cb )
{
    uiSeisSampleEditorGoToDlg dlg( this, curBinID(),is2D() );
    if ( dlg.go() )
    {
	if ( doSetPos( dlg.pos_, false ) )
	    trcselectionChanged( cb );
    }
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
        trcbufvwr_->handleBufChange();
}


void uiSeisSampleEditor::rightArrowPush( CallBacker* cb )
{
    if ( !goTo( getNextBid(curBinID(),stepout_,false) ) )
	return;
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
    trcselectionChanged( cb );
}


void uiSeisSampleEditor::leftArrowPush( CallBacker* cb )
{
    if ( !goTo( getNextBid(curBinID(),stepout_,true) ) )
	return;
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
    trcselectionChanged( cb );
}


void uiSeisSampleEditor::switchViewTypePush( CallBacker* )
{
    crlwise_ = toolbar_->isOn( crlwisebutidx_ );
    toolbar_->setToolTip( crlwisebutidx_,
			  crlwise_ ? toinlwisett_ : tocrlwisett_ );
    doSetPos( curBinID(), true );
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisSampleEditor::commitChanges()
{
    if ( tbuf_.size() < 1 ) return;

    BoolTypeSet changed( tbuf_.size(), false );
    for ( RowCol pos(0,0); pos.col()<tbuf_.size(); pos.col()++)
    {
        SeisTrc& trc = *tbuf_.get( pos.col() );
	for ( pos.row()=0; pos.row()<nrsamples_; pos.row()++)
	{
	    const float tableval = tbl_->getFValue( pos );
	    const float trcval = trc.get( pos.row(), compnr_ );
	    const float diff = tableval - trcval;
	    if ( !mIsZero(diff,1e-6) )
	    {
		trc.set( pos.row(), tableval, compnr_ );
		changed[pos.col()] = true;
	    }
	}
    }

    for ( int idx=0; idx<changed.size(); idx++ )
    {
	if ( !changed[idx] ) continue;

	const SeisTrc& buftrc = *tbuf_.get(idx);
	const int chidx = tbufchgdtrcs_.find( buftrc.info().binID(),is2D() );
	if ( chidx < 0 )
	    tbufchgdtrcs_.add( new SeisTrc( buftrc ) );
	else
	{
	    SeisTrc& chbuftrc = *tbufchgdtrcs_.get( chidx );
	    chbuftrc = buftrc;
	}
    }
}


void uiSeisSampleEditor::doBrowse( uiParent* p, const DBKey& dbky,
				   Pos::GeomID geomid )
{
    uiSeisSampleEditor::Setup setup( dbky );
    setup.geomid_ = geomid;
    uiSeisSampleEditor dlg( p, setup );
    dlg.go();
}


bool uiSeisSampleEditor::acceptOK()
{
    if ( !prov_ )
	return true;

    commitChanges();
    if ( tbufchgdtrcs_.isEmpty() )
	return true;

    const int res =
	uiMSG().askSave(tr("Do you want to save the changes permanently?"),
			true);
    if ( res == 1 )
	return storeChgdData();

    return res == 0;
}


class uiSeisSampleEditorWriter : public Executor
{ mODTextTranslationClass(uiSeisSampleEditorWriter);
public:

uiSeisSampleEditorWriter( const uiSeisSampleEditor::Setup& setup, const SeisTrcBuf& tbuf,
		    bool is2d )
    : Executor( "Writing Back Changed Traces" )
    , nrdone_(0)
    , is2d_(is2d)
    , tbufchgdtrcs_(tbuf)
    , trc_(*new SeisTrc())
    , msg_(tr("Initializing"))
{
    prov_ = Seis::Provider::create( setup.id_, uirv_ );
    if ( !prov_ )
    {
	new uiLabel( this, uirv_ );
	return;
    }

    totalnr_ = prov_->totalNr();
}


~uiSeisSampleEditorWriter()
{
}


bool init()
{
    if ( !tri_ ||  !tro_ )
    {
	uiMSG().error( uiString::emptyString() );
	return false;
    }
    if ( !safeio_->open(false) )
    {
	uiMSG().error( tr("Unable to open the file") );
	return false;
    }
    StreamConn* conno = new StreamConn( safeio_->ostrm() );
    if ( !tro_->initWrite( conno, *tbufchgdtrcs_.first()) )
    {
	uiMSG().error( tr("Unable to write") );
	return false;
    }
    if ( !tri_->readInfo(trc_.info()) || !tri_->read(trc_) )
	{ uiMSG().error( tr("Input cube is empty") ); return false; }
    msg_ = tr("Writing");
    return true;
}

    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const          { return nrdone_; }
    uiString		message() const	{ return msg_; }
    uiString		nrDoneText() const	{ return tr("Traces done"); }

protected:

int nextStep()
{
    if ( nrdone_ == 0 && !init() )
	return ErrorOccurred();

    if ( tri_->read(trc_) )
    {
	const int chgidx = tbufchgdtrcs_.find( trc_.info().binID(), is2d_ );
	const bool res = chgidx<0 ? tro_->write( trc_ )
				  : tro_->write( *tbufchgdtrcs_.get(chgidx) );
	if ( !res )
	{
	    msg_ = tro_->errMsg();
	    safeio_->closeFail();
	    return ErrorOccurred();
	}

	nrdone_++;
	return MoreToDo();
    }

    safeio_->closeSuccess();
    return Finished();
}

    CBVSSeisTrcTranslator* tri_;
    CBVSSeisTrcTranslator* tro_;
    SafeFileIO*		safeio_;

    int			totalnr_;
    int                 nrdone_;
    const SeisTrcBuf&	tbufchgdtrcs_;
    SeisTrc&		trc_;
    bool                is2d_;
    uiString		msg_;

};


bool uiSeisSampleEditor::storeChgdData()
{
    PtrMan<uiSeisSampleEditorWriter> wrtr =
	new uiSeisSampleEditorWriter( *prov_, changedtraces_ );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, *wrtr );
}


void uiSeisSampleEditor::dispTracesPush( CallBacker* )
{
    if ( trcbufvwr_ )
	trcbufvwr_->start();
    else
    {
	uiSeisTrcBufViewer::Setup stbvsetup( uiString::emptyString() );
	trcbufvwr_ = new uiSeisTrcBufViewer( this, stbvsetup );
	trcbufvwr_->selectDispTypes( true, false );
	trcbufvwr_->windowClosed.notify(
			 mCB(this,uiSeisSampleEditor,trcbufViewerClosed) );

	trcbufvwr_->setTrcBuf( &tbuf_, setup_.geom_, "Browsed seismic data",
				    DBM().nameOf(setup_.id_), compnr_ );
	trcbufvwr_->start(); trcbufvwr_->handleBufChange();

	if ( (tbuf_.isEmpty()) )
	    uiMSG().error( tr("No data at the specified position ") );
    }

    updateWiggleButtonStatus();
    setTrcBufViewTitle();
}


void uiSeisSampleEditor::updateWiggleButtonStatus()
{
    const bool turnon = !trcbufvwr_ || trcbufvwr_->isHidden();
    toolbar_->turnOn( showwgglbutidx_, !turnon );
}


void uiSeisSampleEditor::trcbufViewerClosed( CallBacker* )
{
    trcbufvwr_ = 0;
    updateWiggleButtonStatus();
}


void uiSeisSampleEditor::tblValChgCB( CallBacker* )
{
    commitChanges();
    const RowCol rc = tbl_->currentCell();
    if ( rc.row()<0 || rc.col()<0 ) return;

    SeisTrc* trace = tbuf_.get( rc.col() );
    const float chgdval = tbl_->getFValue( rc );
    trace->set( rc.row(), chgdval, compnr_ );
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisSampleEditor::setTrcBufViewTitle()
{
    if ( trcbufvwr_ )
	trcbufvwr_->setWinTitle( tr( "Central Trace: %1")
			       .arg(curBinID()
			  .toString(Seis::is2D(setup_.geom_)) ) );

}


void uiSeisSampleEditor::chgCompNrCB( CallBacker* )
{
    NotifyStopper notifstop( tbl_->valueChanged );
    commitChanges();
    compnr_ = selcompnmfld_ ? selcompnmfld_->currentItem() : 0;
    fillTable();
}


void uiSeisSampleEditor::nrTracesChgCB( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->getIntValue();
    stepout_ = nrtrcs / 2;
    tbl_->clear();
    tbl_->setNrCols( nrtrcs );
    doSetPos( curBinID(), true );
    if ( trcbufvwr_ )
    {
	trcbufvwr_->setTrcBuf( &tbuf_, setup_.geom_, "Browsed seismic data",
				    DBM().nameOf(setup_.id_), compnr_ );
	trcbufvwr_->handleBufChange();
    }
}


// uiSeisSampleEditorInfoVwr
uiSeisSampleEditorInfoVwr::uiSeisSampleEditorInfoVwr( uiParent* p, const SeisTrc& trc,
					    bool is2d, const ZDomain::Def& zd )
    : uiAmplSpectrum(p)
    , is2d_(is2d)
    , zdomdef_(zd)
{
    setDeleteOnClose( true );
    setCaption( tr("Trace information") );

    uiGroup* valgrp = new uiGroup( this, "Values group" );

    PositionInpSpec coordinpspec( PositionInpSpec::Setup(true,is2d_,false) );
    coordfld_ = new uiGenInput( valgrp, uiStrings::sCoordinate(),
				coordinpspec.setName("X",0).setName("Y",0) );
    coordfld_->setReadOnly();

    uiString label( is2d_ ? tr("Trace/Ref number") : uiStrings::sPosition() );
    IntInpSpec iis; FloatInpSpec fis;
    DataInpSpec* pdis = &iis; if ( is2d_ ) pdis = &fis;
    trcnrbinidfld_ = new uiGenInput( valgrp, label, iis, *pdis );
    trcnrbinidfld_->attach( alignedBelow, coordfld_ );
    trcnrbinidfld_->setReadOnly();

    minamplfld_ = new uiGenInput( valgrp, tr("Minimum amplitude"),
				  FloatInpSpec() );
    minamplfld_->attach( alignedBelow, trcnrbinidfld_ );
    minamplfld_->setElemSzPol( uiObject::Small );
    minamplfld_->setReadOnly();
    minamplatfld_ = new uiGenInput( valgrp, tr("at"), FloatInpSpec() );
    minamplatfld_->attach( rightOf, minamplfld_ );
    minamplatfld_->setElemSzPol( uiObject::Small );
    minamplatfld_->setReadOnly();
    uiLabel* lbl = new uiLabel( valgrp, zdomdef_.unitStr(true) );
    lbl->attach( rightOf, minamplatfld_ );

    maxamplfld_ = new uiGenInput( valgrp, tr("Maximum amplitude"),
				  FloatInpSpec() );
    maxamplfld_->attach( alignedBelow, minamplfld_ );
    maxamplfld_->setElemSzPol( uiObject::Small );
    maxamplfld_->setReadOnly();
    maxamplatfld_ = new uiGenInput( valgrp, tr("at"), FloatInpSpec() );
    maxamplatfld_->attach( rightOf, maxamplfld_ );
    maxamplatfld_->setElemSzPol( uiObject::Small );
    maxamplatfld_->setReadOnly();
    lbl = new uiLabel( valgrp, zdomdef_.unitStr(true) );
    lbl->attach( rightOf, maxamplatfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, exportfld_ );
    valgrp->attach( centeredBelow, sep );
    valgrp->attach( ensureBelow, sep );

    setPrefHeightInChar( 20 );
    setPrefWidthInChar( 50 );

    setTrace( trc );
}


void uiSeisSampleEditorInfoVwr::setTrace( const SeisTrc& trc )
{
    coordfld_->setValue( trc.info().coord_ );
    if ( !is2d_ )
	trcnrbinidfld_->setValue( trc.info().binID() );
    else
    {
	trcnrbinidfld_->setValue( trc.info().binID().crl(), 0 );
	trcnrbinidfld_->setValue( trc.info().refnr_, 1 );
    }

    if ( trc.size() < 1 ) return;

    float v0 = mUdf(float), z0 = mUdf(float); int isamp;
    for ( isamp=0; isamp<trc.size(); isamp++ )
    {
	const float v = trc.get( isamp, 0 );
	if ( !mIsUdf(v) )
	    { v0 = v; z0 = trc.info().samplePos( isamp ); break; }
    }
    if ( mIsUdf(z0) ) return;

    Interval<float> amplrg( v0, v0 );
    Interval<float> peakzs( z0, z0 );
    TypeSet<float> vals;
    for ( ; isamp<trc.size(); isamp++ )
    {
	const float v = trc.get( isamp, 0 );
	if ( mIsUdf(v) || mIsUdf(-v) || !Math::IsNormalNumber(v) )
	    continue;

	vals += v;
	if ( v < amplrg.start )
	    { amplrg.start = v; peakzs.start = trc.info().samplePos(isamp); }
	if ( v > amplrg.stop )
	    { amplrg.stop = v; peakzs.stop = trc.info().samplePos(isamp); }
    }

    const int zfac = zdomdef_.userFactor();
    minamplfld_->setValue( amplrg.start );
    minamplatfld_->setText( getZValStr(peakzs.start,zfac) );
    maxamplfld_->setValue( amplrg.stop );
    maxamplatfld_->setText( getZValStr(peakzs.stop,zfac) );

    setup_.nyqvistspspace_ = trc.info().sampling_.step;
    Array1DImpl<float> a1d( vals.size() );
    for ( int idx=0; idx<vals.size(); idx++ )
	a1d.set( idx, vals[idx] );
    setData( a1d );
}
