/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisbrowser.h"

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
#include "uiseissel.h"
#include "uiseistrcbufviewer.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"

#include "cbvsreadmgr.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "executor.h"
#include "arrayndimpl.h"
#include "filepath.h"
#include "ioman.h"
#include "ranges.h"
#include "safefileio.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisinfo.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "zdomain.h"
#include "od_helpids.h"


//Writer class
class uiSeisBrowseWriter : public Executor
{ mODTextTranslationClass(uiSeisBrowseWriter);
public:

    uiSeisBrowseWriter( const uiSeisBrowser::Setup& setup,
					const SeisTrcBuf& tbuf, bool is2d )
	: Executor( "Writing Back Changed Traces" )
	, is2d_(is2d)
	, tbufchgdtrcs_(tbuf)
	, trc_(*new SeisTrc())
	, msg_(tr("Initialising"))
    {
	PtrMan<IOObj> outioobj = IOM().get( setup.outmid_ );
	PtrMan<IOObj> inpioobj = IOM().get( setup.inpmid_ );
	const FilePath fp( outioobj->fullUserExpr(true) );
	safeio_ = new SafeFileIO( fp.fullPath() );

	tro_ = CBVSSeisTrcTranslator::getInstance();
	tro_->set2D( Seis::is2D(setup.geom_) );

	uiString errmsg;
	tri_ = CBVSSeisTrcTranslator::make( inpioobj->fullUserExpr(true),
				    false, Seis::is2D(setup.geom_), &errmsg );

	SeisIOObjInfo seisinfo( inpioobj.ptr() );
	TrcKeyZSampling cs;
	seisinfo.getRanges( cs );
	totalnr_ = cs.nrInl() * cs.nrCrl();
    }


    ~uiSeisBrowseWriter()
    {
	delete tri_;
	delete tro_; // tro_ may depend on a 'living' safeio_
	delete safeio_;
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
	{
	    uiMSG().error( tr("Input cube is empty") );
	    return false;
	}

	msg_ = tr("Writing");
	return true;
    }

    od_int64		totalNr() const override	{ return totalnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
    { return tr("Traces done"); }

protected:

    int nextStep() override
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

	deleteAndNullPtr( tri_ );
	deleteAndNullPtr( tro_ );
	return safeio_->closeSuccess() ? Finished() : ErrorOccurred();
    }

    CBVSSeisTrcTranslator*  tri_		= nullptr;
    CBVSSeisTrcTranslator*  tro_		= nullptr;
    SafeFileIO*		    safeio_		= nullptr;

    int			totalnr_;
    int			nrdone_			= 0;
    const SeisTrcBuf&	tbufchgdtrcs_;
    SeisTrc&		trc_;
    bool		is2d_;
    uiString		msg_;

};


//Viewer Class
class uiSeisBrowserInfoVwr : public uiAmplSpectrum
{ mODTextTranslationClass(uiSeisBrowserInfoVwr);
public :

			uiSeisBrowserInfoVwr(uiParent*,const SeisTrc&,bool,
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


uiSeisBrowser::Setup::Setup( const MultiID& ky, Seis::GeomType gt )
    : uiDialog::Setup(uiString::emptyString(),mNoDlgTitle,
		      mODHelpKey(mSeisBrowserHelpID) )
    , inpmid_(ky)
    , outmid_(ky)
    , geom_(gt)
    , startpos_(mUdf(int),mUdf(int))
    , startz_(mUdf(float))
{
    wintitle_ = uiString(tr("Browse %1 '%2'")).arg( Seis::nameOf( gt ) )
					      .arg(IOM().nameOf( ky ));
}


uiSeisBrowser::Setup::~Setup()
{}


uiSeisBrowser::uiSeisBrowser( uiParent* p, const uiSeisBrowser::Setup& su,
			      bool is2d )
    : uiDialog(p,su)
    , is2d_(is2d)
    , setup_(su)
    , tbufbefore_(*new SeisTrcBuf(true))
    , tbufafter_(*new SeisTrcBuf(true))
    , tbuf_(*new SeisTrcBuf(false))
    , tbufchgdtrcs_(*new SeisTrcBuf(false))
    , ctrc_(*new SeisTrc)
    , zdomdef_(&ZDomain::SI())
{
    setCtrlStyle( CloseOnly );
    if ( !openData(su) )
    {
	setTitleText( tr("Error") );
	uiString lbltxt = uiStrings::phrCannotOpen(uiStrings::phrInput(
			  uiStrings::phrData(tr("('%1')\n%2")
			  .arg(Seis::nameOf(su.geom_))
			  .arg(IOM().nameOf( su.inpmid_ )))));

	if ( !su.linekey_.isEmpty() )
	    { lbltxt = toUiString("%1 - %2").arg(lbltxt).arg(su.linekey_); }
	new uiLabel( this, lbltxt );
	return;
    }

    createMenuAndToolBar();
    createTable();
    setPos( su.startpos_, true );
    setZ( su.startz_ );
    mAttachCB( tbl_->selectionChanged, uiSeisBrowser::trcselectionChanged );
}


uiSeisBrowser::~uiSeisBrowser()
{
    detachAllNotifiers();
    delete tr_;
    delete &tbuf_;
    delete &tbufbefore_;
    delete &tbufafter_;
    delete &ctrc_;
}


const BinID& uiSeisBrowser::curBinID() const
{
    return ctrc_.info().trcKey().position();
}


float uiSeisBrowser::curZ() const
{
    return sd_.start + tbl_->currentRow() * sd_.step;
}


void uiSeisBrowser::setZ( float z )
{
    if ( mIsUdf(z) )
	return;

    int newrow = (int)((int)z - (int)sd_.start) / (int)sd_.step;
    tbl_->setCurrentCell( RowCol(tbl_->currentCol(),newrow) );
}


bool uiSeisBrowser::openData( const uiSeisBrowser::Setup& su )
{
    uiString emsg;
    PtrMan<IOObj> ioobj = IOM().get( su.inpmid_ );
    if ( !ioobj )
	return false;

    SeisIOObjInfo ioinf( *ioobj );
    zdomdef_ = &SeisIOObjInfo(*ioobj).zDomainDef();

    if ( is2d_ )
    {
	Seis2DDataSet ds( *ioobj );
	const int index = ds.indexOf( su.linekey_ );
	if ( index < 0 )
	    return false;

	const OD::String& fnm = SeisCBVS2DLineIOProvider::getFileName( *ioobj,
							ds.geomID(index) );
	tr_ = CBVSSeisTrcTranslator::make( fnm, false,
					   Seis::is2D(su.geom_), &emsg );

	const StringView datatype = ds.dataType();
	if ( datatype == sKey::Steering() )
	    compnr_ = 1;
    }
    else
    {
	tr_ = CBVSSeisTrcTranslator::make( ioobj->fullUserExpr(true), false,
				    Seis::is2D(su.geom_), &emsg );
    }

    if ( !tr_ )
    {
	uiMSG().error( emsg );
	return false;
    }

    if ( !tr_->readInfo(ctrc_.info()) )
    {
	uiMSG().error( tr("Input cube is empty") );
	return false;
    }

    nrcomps_ = tr_->componentInfo().size();
    nrsamples_ = tr_->inpNrSamples();
    sd_ = tr_->inpSD();
    return true;
}


#define mAddButton(fnm,func,tip,toggle) \
    uitb_->addButton( fnm, tip, mCB(this,uiSeisBrowser,func), toggle )

void uiSeisBrowser::createMenuAndToolBar()
{
    uitb_ = new uiToolBar( this, tr("Tool Bar") );
    if ( !setup_.locked_ )
    {
	editbutidx_ = mAddButton( "edit", editCB, uiStrings::sEdit(), true );
	savebutidx_ = mAddButton( "save", saveChangesCB, uiStrings::sSave(),
								    false );
	saveasbutidx_ = mAddButton( "saveas", saveAsChangesCB,
						uiStrings::sSaveAs(), false );
	updateSaveButtonState( false );
    }

    mAddButton( "gotopos",goToPush,tr("Goto position"),false );
    mAddButton( "info",infoPush,tr("Information"),false );
    if ( !is2d_ )
	crlwisebutidx_ = mAddButton( "crlwise",switchViewTypePush,
				     tr("Switch to Cross-line"),true );
    mAddButton( "leftarrow",leftArrowPush,tr("Move left"),false );
    mAddButton( "rightarrow",rightArrowPush,tr("Move right"),false );
    showwgglbutidx_ = mAddButton( "wva",dispTracesPush,
				  tr("Display current traces"),false );
    tr_->getComponentNames( compnms_ );
    if ( compnms_.size()>1 )
    {
	selcompnmfld_ = new uiComboBox( uitb_, compnms_, "Component name" );
	uitb_->addObject( selcompnmfld_ );
	selcompnmfld_->setCurrentItem( compnr_ );
	mAttachCB( selcompnmfld_->selectionChanged,
						uiSeisBrowser::chgCompNrCB );
    }

    uiLabel* lbl = new uiLabel( uitb_, tr("Nr traces") );
    uitb_->addObject( lbl );
    nrtrcsfld_ = new uiSpinBox( uitb_ );
    nrtrcsfld_->setInterval( StepInterval<int>(3,99999,2) );
    nrtrcsfld_->doSnap( true );
    nrtrcsfld_->setValue( 2*stepout_+1 );
    mAttachCB( nrtrcsfld_->valueChanged, uiSeisBrowser::nrTracesChgCB );
    uitb_->addObject( nrtrcsfld_ );
}


void uiSeisBrowser::createTable()
{
    const int nrrows = tr_->readMgr()->info().nrsamples_;
    const int nrcols = 2*stepout_ + 1;
    tbl_ = new uiTable( this, uiTable::Setup(nrrows,nrcols)
			     .selmode(uiTable::Multi).manualresize(true),
			     "Seismic data" );
    mAttachCB( tbl_->valueChanged, uiSeisBrowser::valChgReDraw );
    tbl_->setStretch( 1, 1 );
    tbl_->setPrefHeight( 400 );
    tbl_->setPrefWidth( 600 );
    tbl_->setTableReadOnly( true );
}


BinID uiSeisBrowser::getNextBid( const BinID& cur, int idx,
				   bool before ) const
{
    const BinID& step = tr_->readMgr()->info().geom_.step;
    return crlwise_
	? BinID( cur.inl() + (before?-1:1)*step.inl()*idx, cur.crl() )
	: BinID( cur.inl(), cur.crl() + (before?-1:1)*step.crl()*idx );
}


void uiSeisBrowser::addTrc( SeisTrcBuf& tbuf, const BinID& bid )
{
    auto* newtrc = new SeisTrc;
    const int chgbufidx = tbufchgdtrcs_.find( bid, false );
    if ( chgbufidx >= 0 )
	*newtrc = *tbufchgdtrcs_.get( chgbufidx );
    else if ( !tr_->goTo(bid) || !tr_->read(*newtrc) )
    {
	if ( is2D() )
	    newtrc->info().setGeomID( Pos::GeomID(bid.row()) )
			  .setTrcNr( bid.trcNr() );
	else
	    newtrc->info().setPos( bid );

	newtrc->info().calcCoord();
	fillUdf( *newtrc );
    }

    tbuf.add( newtrc );
}


void uiSeisBrowser::setPos( const BinID& bid, bool veryfirst )
{
    doSetPos( bid, false, veryfirst );
}


bool uiSeisBrowser::doSetPos( const BinID& bid, bool force, bool veryfirst )
{
    if ( !tbl_ )
	return false;

    if ( !force && bid == ctrc_.info().binID() )
	return true;

    NotifyStopper notifstop( tbl_->valueChanged );

    commitChanges( false );
    BinID binid( bid );
    const bool inlok = is2D() || !mIsUdf(bid.inl());
    const bool crlok = !mIsUdf(bid.crl());
    if ( !inlok || !crlok )
    {
	tr_->toStart();
	binid = tr_->readMgr()->binID();
	const BinID step = tr_->readMgr()->info().geom_.step;
	if ( crlwise_ )
	    binid.inl() += stepout_ * step.inl();
	else
	    binid.crl() += stepout_ * step.crl();
    }

    tbuf_.erase();
    tbufbefore_.deepErase();
    tbufafter_.deepErase();

    const bool havetrc = tr_->goTo( binid );
    const bool canread = havetrc && tr_->read( ctrc_ );
    if ( !canread && !veryfirst )
	uiMSG().error( tr("Cannot read data at specified location") );
    if ( !havetrc || !canread )
    {
	binid = ctrc_.info().binID();
	if ( !tr_->goTo(binid) || !tr_->read(ctrc_) )
	    fillUdf( ctrc_ );
    }

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


bool uiSeisBrowser::is2D() const
{
    return is2d_;
}


void uiSeisBrowser::setStepout( int nr )
{
    stepout_ = nr;
    nrtrcsfld_->setValue( nr*2+1 );
}


void uiSeisBrowser::fillUdf( SeisTrc& trc )
{
    while ( trc.nrComponents() > nrcomps_ )
	trc.data().delComponent(0);

    while ( trc.nrComponents() < nrcomps_ )
	trc.data().addComponent( nrsamples_, DataCharacteristics() );

    trc.reSize( nrsamples_, false );

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	for ( int isamp=0; isamp<nrsamples_; isamp++ )
	    trc.set( isamp, mUdf(float), icomp );
    }
}


void uiSeisBrowser::fillTable()
{
    NotifyStopper notifstop( tbl_->valueChanged );

    const CBVSInfo& info = tr_->readMgr()->info();
    const int zfac = zdomdef_->userFactor();
    const char* zunstr = zdomdef_->unitStr(false);
    const int nrdec = Math::NrSignificantDecimals( info.sd_.step*zfac );

    BufferString zvalstr;
    for ( int idx=0; idx<info.nrsamples_; idx++ )
    {
	zvalstr.set( info.sd_.atIndex(idx)*zfac, nrdec );
	tbl_->setRowLabel( idx, toUiString(zvalstr) );
	uiString tt;
        tt = toUiString("%1%2 sample at %3").arg(idx+1)
                    .arg(getRankPostFix(idx+1)).arg(zvalstr).withUnit(zunstr);
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

    const int middlecol = tbuf_.size()/2;
    tbl_->selectColumn( middlecol );
    tbl_->ensureCellVisible( RowCol(0,middlecol) );
    tbl_->resizeColumnsToContents();
}


void uiSeisBrowser::fillTableColumn( const SeisTrc& trc, int colidx )
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


class uiSeisBrowserGoToDlg : public uiDialog
{ mODTextTranslationClass(uiSeisBrowserGoToDlg);
public:

uiSeisBrowserGoToDlg( uiParent* p, BinID cur, bool is2d, bool isps=false )
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

bool acceptOK( CallBacker* ) override
{
    pos_ = posfld_->getBinID();
    if ( !SI().isReasonable(pos_) )
    {
	uiMSG().error( tr("Please specify a valid position") );
	return false;
    }

    return true;
}

    BinID	pos_;
    uiGenInput*	posfld_;

};


bool uiSeisBrowser::goTo( const BinID& bid )
{
    return doSetPos( bid, true );
}


void uiSeisBrowser::updateSaveButtonState( bool setactive )
{
    saveenabled_ = setactive;
    uitb_->setSensitive( savebutidx_, setactive );
    uitb_->setSensitive( saveasbutidx_, setactive );
}


bool uiSeisBrowser::storeChgdData( const bool isnew )
{
    commitChanges( isnew );
    deleteAndNullPtr( tr_ );
    if ( tbufchgdtrcs_.isEmpty() )
	return false;

    Setup su( setup_ );
    if ( isnew )
    {
	IOObjContext ctxt( uiSeisSel::ioContext(is2D()?Seis::Line:Seis::Vol,
								    false) );
	ctxt.toselect_.require_.set( sKey::Type(), sKey::Attribute() );
	uiSeisSelDlg seiseldlg( this, ctxt, uiSeisSel::Setup(is2D(),false) );
	if ( !seiseldlg.go() )
	    return false;

	const IOObj* ioobj = seiseldlg.ioObj();
	if ( !ioobj )
	{
	    uiMSG().error( tr("Output object not defined") );
	    return false;
	}

	su.outmid_ = ioobj->key();
    }

    PtrMan<uiSeisBrowseWriter> wrtr =
	new uiSeisBrowseWriter( su, tbufchgdtrcs_, is2D() );
    uiTaskRunner dlg( this );
    if ( !TaskRunner::execute(&dlg,*wrtr) )
    {
	openData( setup_ );
	uiMSG().error( tr("Unable to write data") );
	return false;
    }

    updateSaveButtonState( false );
    uiMSG().message( tr("Data Successfully written") );
    return true;
}


void uiSeisBrowser::editCB( CallBacker* )
{
    const bool editenabled = uitb_->isOn( editbutidx_ );
    tbl_->setTableReadOnly( !editenabled );
    updateSaveButtonState( editenabled );
}


void uiSeisBrowser::saveChangesCB( CallBacker* )
{
    storeChgdData( false );
}


void uiSeisBrowser::saveAsChangesCB( CallBacker* )
{
    storeChgdData( true );
}


void uiSeisBrowser::infoPush( CallBacker* )
{
    const SeisTrc& trc = tbl_->currentCol()<0 ? ctrc_
					      : *tbuf_.get(tbl_->currentCol());
    if ( !infovwr_ )
    {
	infovwr_ = new uiSeisBrowserInfoVwr( this, trc, is2d_, *zdomdef_ );
	mAttachCB( infovwr_->windowClosed, uiSeisBrowser::infoClose );
    }

    infovwr_->setTrace( trc );
    infovwr_->show();
}


void uiSeisBrowser::infoClose( CallBacker* )
{
    infovwr_ = nullptr;
}


void uiSeisBrowser::trcselectionChanged( CallBacker* )
{
    if ( infovwr_ )
    {
	const SeisTrc& trc = tbl_->currentCol()<0 ? ctrc_
				      : *tbuf_.get(tbl_->currentCol());
	infovwr_->setTrace( trc );
    }
}


void uiSeisBrowser::goToPush( CallBacker* cb )
{
    uiSeisBrowserGoToDlg dlg( this, curBinID(),is2D() );
    if ( dlg.go() )
    {
	if ( doSetPos( dlg.pos_, false ) )
	    trcselectionChanged( cb );
    }

    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisBrowser::rightArrowPush( CallBacker* cb )
{
    if ( !goTo( getNextBid(curBinID(),stepout_,false) ) )
	return;

    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();

    trcselectionChanged( cb );
}


void uiSeisBrowser::leftArrowPush( CallBacker* cb )
{
    if ( !goTo( getNextBid(curBinID(),stepout_,true) ) )
	return;

    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();

    trcselectionChanged( cb );
}


void uiSeisBrowser::switchViewTypePush( CallBacker* )
{
    crlwise_ = uitb_->isOn( crlwisebutidx_ );
    doSetPos( curBinID(), true );
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisBrowser::commitChanges( bool isnew )
{
    if ( tbuf_.size() < 1 )
	return;

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
	if ( !changed[idx] )
	    continue;

	const SeisTrc& buftrc = *tbuf_.get(idx);
	const int chidx = tbufchgdtrcs_.find(buftrc.info().binID(),is2D());
	if ( chidx < 0 || isnew )
	    tbufchgdtrcs_.add( new SeisTrc( buftrc ) );
	else
	{
	    SeisTrc& chbuftrc = *tbufchgdtrcs_.get( chidx );
	    chbuftrc = buftrc;
	}
    }
}


void uiSeisBrowser::doBrowse( uiParent* p, const IOObj& ioobj, bool is2d,
			      const LineKey* lk )
{
    uiSeisBrowser::Setup setup( ioobj.key(), is2d ? Seis::Line : Seis::Vol );
    setup.locked( ioobj.implReadOnly() );
    if ( lk )
	setup.linekey( *lk );

    uiSeisBrowser dlg( p, setup, is2d );
    dlg.go();
}


void uiSeisBrowser::dispTracesPush( CallBacker* )
{
    if ( trcbufvwr_ )
	trcbufvwr_->start();
    else
    {
	uiSeisTrcBufViewer::Setup stbvsetup( uiString::emptyString() );
	trcbufvwr_ = new uiSeisTrcBufViewer( this, stbvsetup );
	trcbufvwr_->selectDispTypes( true, false );
	mAttachCB( trcbufvwr_->windowClosed,
					uiSeisBrowser::trcbufViewerClosed );
	trcbufvwr_->setTrcBuf( &tbuf_, setup_.geom_, "Browsed seismic data",
				    IOM().nameOf(setup_.inpmid_), compnr_ );
	trcbufvwr_->start(); trcbufvwr_->handleBufChange();

	if ( (tbuf_.isEmpty()) )
	    uiMSG().error( tr("No data at the specified position ") );
    }

    updateWiggleButtonStatus();
    setTrcBufViewTitle();
}


void uiSeisBrowser::updateWiggleButtonStatus()
{
    const bool turnon = !trcbufvwr_ || trcbufvwr_->isHidden();
    uitb_->turnOn( showwgglbutidx_, !turnon );
}


void uiSeisBrowser::trcbufViewerClosed( CallBacker* )
{
    trcbufvwr_ = nullptr;
    updateWiggleButtonStatus();
}


void uiSeisBrowser::valChgReDraw( CallBacker* )
{
    commitChanges( false );
    const RowCol rc = tbl_->currentCell();
    if ( rc.row()<0 || rc.col()<0 )
	return;

    SeisTrc* trace = tbuf_.get( rc.col() );
    const float chgdval = tbl_->getFValue( rc );
    trace->set( rc.row(), chgdval, compnr_ );
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();

    if ( !saveenabled_ )
	updateSaveButtonState( true );
}


void uiSeisBrowser::setTrcBufViewTitle()
{
    if ( trcbufvwr_ )
	trcbufvwr_->setWinTitle( tr( "Central Trace: %1")
			       .arg(curBinID()
			  .toString(Seis::is2D(setup_.geom_)) ) );

}


void uiSeisBrowser::chgCompNrCB( CallBacker* )
{
    NotifyStopper notifstop( tbl_->valueChanged );
    commitChanges( false );
    compnr_ = selcompnmfld_ ? selcompnmfld_->currentItem() : 0;
    fillTable();
}


void uiSeisBrowser::nrTracesChgCB( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->getIntValue();
    stepout_ = nrtrcs / 2;
    tbl_->clear();
    tbl_->setNrCols( nrtrcs );
    doSetPos( curBinID(), true );
    if ( trcbufvwr_ )
    {
	trcbufvwr_->setTrcBuf( &tbuf_, setup_.geom_, "Browsed seismic data",
				    IOM().nameOf(setup_.inpmid_), compnr_ );
	trcbufvwr_->handleBufChange();
    }
}


// uiSeisBrowserInfoVwr
uiSeisBrowserInfoVwr::uiSeisBrowserInfoVwr( uiParent* p, const SeisTrc& trc,
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
    DataInpSpec* pdis = &iis;
    if ( is2d_ )
	pdis = &fis;

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
    uiLabel* lbl = new uiLabel( valgrp, zdomdef_.uiUnitStr(true) );
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
    lbl = new uiLabel( valgrp, zdomdef_.uiUnitStr(true) );
    lbl->attach( rightOf, maxamplatfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, exportfld_ );
    valgrp->attach( centeredBelow, sep );
    valgrp->attach( ensureBelow, sep );

    setPrefHeightInChar( 20 );
    setPrefWidthInChar( 50 );

    setTrace( trc );
}


void uiSeisBrowserInfoVwr::setTrace( const SeisTrc& trc )
{
    coordfld_->setValue( trc.info().coord );
    if ( is2d_ )
    {
	trcnrbinidfld_->setValue( trc.info().trcNr(), 0 );
	trcnrbinidfld_->setValue( trc.info().refnr, 1 );
    }
    else
	trcnrbinidfld_->setValue( trc.info().binID() );

    if ( trc.size() < 1 )
	return;

    float v0 = mUdf(float);
    float z0 = mUdf(float);
    int isamp;
    for ( isamp=0; isamp<trc.size(); isamp++ )
    {
	const float v = trc.get( isamp, 0 );
	if ( !mIsUdf(v) )
	{
	    v0 = v;
	    z0 = trc.info().samplePos( isamp );	
	    break;
	}
    }

    if ( mIsUdf(z0) )
	return;

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
	{
	    amplrg.start = v;
	    peakzs.start = trc.info().samplePos( isamp );
	}

	if ( v > amplrg.stop )
	{
	    amplrg.stop = v;
	    peakzs.stop = trc.info().samplePos( isamp );
	}
    }

    minamplfld_->setValue( amplrg.start );
    maxamplfld_->setValue( amplrg.stop );

    const int zfac = zdomdef_.userFactor();
    const int nrdec =
		Math::NrSignificantDecimals( trc.info().sampling.step*zfac );
    BufferString zvalstr;
    zvalstr.set( peakzs.start*zfac, nrdec );
    minamplatfld_->setText( zvalstr );
    zvalstr.set( peakzs.stop*zfac, nrdec );
    maxamplatfld_->setText( zvalstr );

    setup_.nyqvistspspace_ = trc.info().sampling.step;
    Array1DImpl<float> a1d( vals.size() );
    for ( int idx=0; idx<vals.size(); idx++ )
	a1d.set( idx, vals[idx] );

    setData( a1d );
}
