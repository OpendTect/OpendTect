/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiseisbrowser.cc,v 1.70 2012-08-07 04:03:34 cvsmahant Exp $";

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
#include "uiseistrcbufviewer.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbar.h"

#include "cbvsreadmgr.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "datapack.h"
#include "executor.h"
#include "arrayndimpl.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "oddirs.h"
#include "ptrman.h"
#include "ranges.h"
#include "safefileio.h"
#include "samplingdata.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seisinfo.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "keystrs.h"
#include "zdomain.h"
#include <iostream>



class uiSeisBrowserInfoVwr : public uiAmplSpectrum
{
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
    : uiDialog::Setup("",mNoDlgTitle,"103.1.5")
    , id_(ky)
    , geom_(gt)
    , startpos_(mUdf(int),mUdf(int))
    , startz_(mUdf(float))
    , readonly_(true)
{
    wintitle_ = "Browse ";
    wintitle_ += Seis::nameOf( gt );
    wintitle_ += " '"; wintitle_ += IOM().nameOf( ky ); wintitle_ += "'";
}



uiSeisBrowser::uiSeisBrowser( uiParent* p, const uiSeisBrowser::Setup& su,
			      bool is2d )
    : uiDialog(p,su)
    , is2d_(is2d)
    , tr_(0)
    , tro_(0)
    , tbl_(0)
    , uitb_(0)
    , tbufbefore_(*new SeisTrcBuf(true))
    , tbufafter_(*new SeisTrcBuf(true))
    , tbuf_(*new SeisTrcBuf(false))
    , tbufchgdtrcs_(*new SeisTrcBuf(false))
    , ctrc_(*new SeisTrc)
    , crlwise_(false)
    , stepout_(25)
    , compnr_(0)
    , nrcomps_(1)
    , sd_(0)
    , infovwr_(0)
    , trcbufvwr_(0)
    , setup_(su)
    , zdomdef_(&ZDomain::SI())
{
    if ( !openData(su) )
    {
	setTitleText( "Error" );
	BufferString lbltxt( "Cannot open input data (" );
	lbltxt += Seis::nameOf(su.geom_); lbltxt += ")\n";
	lbltxt += IOM().nameOf( su.id_ );
	if ( !su.linekey_.isEmpty() )
	    { lbltxt += " - "; lbltxt += su.linekey_; }
	new uiLabel( this, lbltxt );
	setCtrlStyle( LeaveOnly );
	return;
    }

    createMenuAndToolBar();
    createTable();

    setPos( su.startpos_ );
    setZ( su.startz_ );
    tbl_->selectionChanged.notify( mCB(this,uiSeisBrowser,trcselectionChanged));
}


uiSeisBrowser::~uiSeisBrowser()
{
    delete tr_;
    delete &tbuf_;
    delete &tbufbefore_;
    delete &tbufafter_;
    delete &ctrc_;
}


const BinID& uiSeisBrowser::curBinID() const
{
    return ctrc_.info().binid;
}


float uiSeisBrowser::curZ() const
{
    return sd_.start + tbl_->currentRow() * sd_.step;
}


void uiSeisBrowser::setZ( float z ) 
{
    if ( mIsUdf(z) ) return;

    int newrow = (int)((int)z - (int)sd_.start) / (int)sd_.step;
    tbl_->setCurrentCell( RowCol(tbl_->currentCol(),newrow) );
}


bool uiSeisBrowser::openData( const uiSeisBrowser::Setup& su )
{
    BufferString emsg;
    PtrMan<IOObj> ioobj = IOM().get( su.id_ );
    if ( !ioobj ) return false;

    SeisIOObjInfo ioinf( *ioobj );
    zdomdef_ = &SeisIOObjInfo(*ioobj).zDomainDef();

    if ( is2d_ )
    {
	Seis2DLineSet seislineset( ioobj->fullUserExpr(true) );
	const int index = seislineset.indexOf( su.linekey_ );
	IOPar par( seislineset.getInfo(index) );
	FixedString fname = par.find( sKey::FileName() );
	FilePath fp( fname );
	if ( !fp.isAbsolute() )
	    fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
	tr_ = CBVSSeisTrcTranslator::make( fp.fullPath(), false,
					   Seis::is2D(su.geom_), &emsg );
	if ( su.linekey_.attrName() == sKey::Steering() )
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
	uiMSG().error( "Input cube is empty" );
	return false;
    }

    nrcomps_ = tr_->componentInfo().size();
    nrsamples_ = tr_->outNrSamples();
    sd_ = tr_->outSD();
    return true;
}


#define mAddButton(fnm,func,tip,toggle) \
    uitb_->addButton( fnm, tip, mCB(this,uiSeisBrowser,func), toggle )

void uiSeisBrowser::createMenuAndToolBar()
{
    uitb_ = new uiToolBar( this, "Tool Bar" );
    mAddButton( "gotopos",goToPush,"Goto position",false );
    mAddButton( "info",infoPush,"Information",false );
    if ( !is2d_ )
	crlwisebutidx_ = mAddButton( "crlwise",switchViewTypePush,
				     "Switch to Crossline",true );
    mAddButton( "leftarrow",leftArrowPush,"Move left",false );
    mAddButton( "rightarrow",rightArrowPush,"Move right",false );
    showwgglbutidx_ = mAddButton( "vd",dispTracesPush,
	    			  "Display current traces",false );
    tr_->getComponentNames( compnms_ );
    if ( compnms_.size()>1 )
    {
	selcompnmfld_ = new uiComboBox( uitb_, compnms_, "Component name" );
	uitb_->addObject( selcompnmfld_ );
	selcompnmfld_->setCurrentItem( compnr_ );
	selcompnmfld_->selectionChanged.notify( 
					mCB(this,uiSeisBrowser,chgCompNrCB) );
    }
}


void uiSeisBrowser::createTable()
{
    const int nrrows = tr_->readMgr()->info().nrsamples_;
    const int nrcols = 2*stepout_ + 1;
    tbl_ = new uiTable( this, uiTable::Setup( nrrows, nrcols )
			     .selmode(uiTable::Multi)
    			     .manualresize( true ), "Seismic data" );
    
    tbl_->valueChanged.notify( mCB(this,uiSeisBrowser,valChgReDraw) );
    tbl_->setStretch( 1, 1 );
    tbl_->setPrefHeight( 400 );
    tbl_->setPrefWidth( 600 );
    tbl_->setTableReadOnly( setup_.readonly_ );
}


BinID uiSeisBrowser::getNextBid( const BinID& cur, int idx,
				   bool before ) const
{
    const BinID& step = tr_->readMgr()->info().geom_.step;
    return crlwise_ ? BinID( cur.inl + (before?-1:1) * step.inl * idx, cur.crl)
		    : BinID( cur.inl, cur.crl + (before?-1:1) * step.crl * idx);
}


void uiSeisBrowser::addTrc( SeisTrcBuf& tbuf, const BinID& bid )
{
    SeisTrc* newtrc = new SeisTrc;
    newtrc->info().binid = bid;
    newtrc->info().coord.x = newtrc->info().coord.y = mUdf(double);
    const int chgbufidx = tbufchgdtrcs_.find( bid, false );
    if ( chgbufidx >= 0 )
	*newtrc = *tbufchgdtrcs_.get( chgbufidx );
    else if ( !tr_->goTo(bid) || !tr_->read(*newtrc) )
	{ fillUdf( *newtrc ); }
    tbuf.add( newtrc );
}


void uiSeisBrowser::setPos( const BinID& bid )
{
    doSetPos( bid, false );
}


bool uiSeisBrowser::doSetPos( const BinID& bid, bool force )
{
    NotifyStopper notifstop( tbl_->valueChanged );

    if ( !tbl_ )
	return false;
    if ( !force && bid == ctrc_.info().binid )
	return true;

    commitChanges();
    BinID binid( bid );
    const bool inlok = is2D() || !mIsUdf(bid.inl);
    const bool crlok = !mIsUdf(bid.crl);
    if ( !inlok || !crlok )
    {
	tr_->toStart();
	binid = tr_->readMgr()->binID();
    }

    tbuf_.erase();
    tbufbefore_.deepErase();
    tbufafter_.deepErase();

    const bool havetrc = tr_->goTo( binid );
    const bool canread = havetrc && tr_->read( ctrc_ );
    if ( !canread )
    {
	binid = ctrc_.info().binid;
	uiMSG().error( "Cannot read data at specified location" );
    }
    if ( !havetrc || !canread )
	binid = ctrc_.info().binid;

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

    for ( int idx=0; idx<tbuf_.size(); idx++ )
	tbuf_.get(idx)->info().nr = idx;

    fillTable();
    return true;
}


bool uiSeisBrowser::is2D() const
{
    return tr_->is2D();
}


void uiSeisBrowser::setStepout( int nr )
{
    stepout_ = nr;
    // TODO full redraw
    // TODO? store in user settings
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


static const char* getZValStr( float z, const float zfac )
{
    static BufferString txt;
    float dispz = zfac * z * 10;
    int idispz = mNINT32( dispz );
    dispz = idispz * 0.1;
    txt = dispz;
    return txt.buf();
}


void uiSeisBrowser::fillTable()
{
    const CBVSInfo& info = tr_->readMgr()->info();
    const float zfac = zdomdef_->userFactor();
    const char* zunstr = zdomdef_->unitStr(false);
    for ( int idx=0; idx<info.nrsamples_; idx++ )
    {
	const BufferString zvalstr( getZValStr(info.sd_.atIndex(idx),zfac) );
	tbl_->setRowLabel( idx, zvalstr );
	BufferString tt;
	tt.add( idx+1 ).add( getRankPostFix(idx+1) ).add( " sample at " )
	  .add( zvalstr ).add( zunstr );
	tbl_->setRowToolTip( idx, tt );
    }

    for ( int idx=0; idx<tbuf_.size(); idx++ )
    {
	const SeisTrc& buftrc = *tbuf_.get(idx);
	const int chidx = tbufchgdtrcs_.find(buftrc.info().binid,is2D());
	if ( chidx < 0 )
	    fillTableColumn( buftrc, idx );
	else
	    fillTableColumn( *(tbufchgdtrcs_.get(chidx)), idx );
    }

    tbl_->resizeRowsToContents();
}


void uiSeisBrowser::fillTableColumn( const SeisTrc& trc, int colidx )
{
    RowCol rc; rc.col = colidx;
    BufferString lbl;
    if ( is2D() )
	lbl = trc.info().binid.crl;
    else
	{ lbl = trc.info().binid.inl; lbl += "/"; lbl += trc.info().binid.crl; }
    tbl_->setColumnLabel( colidx, lbl );

    for ( rc.row=0; rc.row<nrsamples_; rc.row++ )
    {
	const float val = trc.get( rc.row, compnr_ );
	tbl_->setValue( rc, val );
    }
}


class uiSeisBrowserGoToDlg : public uiDialog
{
public:

uiSeisBrowserGoToDlg( uiParent* p, BinID cur, bool is2d, bool isps=false )
    : uiDialog( p, uiDialog::Setup("Reposition","Specify a new position",
				   mNoHelpID) )
{
    PositionInpSpec inpspec(
	    PositionInpSpec::Setup(false,is2d,isps).binid(cur) );
    posfld_ = new uiGenInput( this, "New Position", inpspec.setName("Inline",0)
	    					    .setName("Crossline",1) );
}

bool acceptOK( CallBacker* )
{
    pos_ = posfld_->getBinID();
    if ( !SI().isReasonable(pos_) )
    {
	uiMSG().error( "Please specify a valid position" );
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


void uiSeisBrowser::infoPush( CallBacker* )
{
    const SeisTrc& trc = tbl_->currentCol()<0 ? ctrc_ 
					      : *tbuf_.get(tbl_->currentCol());
    if ( !infovwr_ )
    {
	infovwr_ = new uiSeisBrowserInfoVwr( this, trc, is2d_, *zdomdef_ );
	infovwr_->windowClosed.notify( mCB(this,uiSeisBrowser,infoClose) );
    }
    infovwr_->setTrace( trc );
    infovwr_->show();
}


void uiSeisBrowser::infoClose( CallBacker* )
{
    infovwr_ = 0;
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
	/* user pressed OK AND input is OK */
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


void uiSeisBrowser::commitChanges()
{
    if ( tbuf_.size() < 1 ) return;

    BoolTypeSet changed( tbuf_.size(), false );
    for ( RowCol pos(0,0); pos.col<tbuf_.size(); pos.col++)
    {
        SeisTrc& trc = *tbuf_.get( pos.col );
	for ( pos.row=0; pos.row<nrsamples_; pos.row++) 
     	{
	    const float tableval = tbl_->getfValue( pos );
	    const float trcval = trc.get( pos.row, compnr_ );
	    const float diff = tableval - trcval;
   	    if ( !mIsZero(diff,1e-6) )
	    {
 	 	trc.set( pos.row, tableval, compnr_ );
		changed[pos.col] = true;
	    }
	}
    }

    for ( int idx=0; idx<changed.size(); idx++ )
    {
	if ( !changed[idx] ) continue;

	const SeisTrc& buftrc = *tbuf_.get(idx);
	const int chidx = tbufchgdtrcs_.find(buftrc.info().binid,is2D());
	if ( chidx < 0 )
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
    setup.readonly( ioobj.implReadOnly() );
    if ( lk ) setup.linekey( *lk );
    uiSeisBrowser dlg( p, setup, is2d );
    dlg.go();
}


bool uiSeisBrowser::acceptOK( CallBacker* )
{
    commitChanges();
    if ( tbufchgdtrcs_.isEmpty() )
	return true;

    const int res =
	uiMSG().askSave( "Do you want to save the changes permanently?", true );
    if ( res == 1 )
	return storeChgdData();

    return res == 0;
}


class uiSeisBrowseWriter : public Executor
{
public:

uiSeisBrowseWriter( const uiSeisBrowser::Setup& setup, const SeisTrcBuf& tbuf,
		    bool is2d )
    : Executor( "Writing Back Changed Traces" )
    , tro_(0)
    , tri_(0)
    , nrdone_(0)
    , is2d_(is2d)
    , tbufchgdtrcs_(tbuf)
    , trc_(*new SeisTrc())
{
    PtrMan<IOObj> ioobj = IOM().get( setup.id_ );
    const FilePath fp( ioobj->fullUserExpr(true) );
    safeio_ = new SafeFileIO( fp.fullPath() );

    tro_ = CBVSSeisTrcTranslator::getInstance();
    tro_->set2D( Seis::is2D(setup.geom_) );

    BufferString errmsg;
    tri_ = CBVSSeisTrcTranslator::make( ioobj->fullUserExpr(true), false,
	               		        Seis::is2D(setup.geom_), &errmsg );

    SeisIOObjInfo seisinfo( ioobj.ptr() );
    CubeSampling cs;
    seisinfo.getRanges( cs );
    totalnr_ = cs.nrInl() * cs.nrCrl();
}


~uiSeisBrowseWriter()
{
    delete safeio_;
    delete tri_;
    delete tro_;
}


bool init()
{
    if ( !tri_ ||  !tro_ )
    {
	uiMSG().error( "" );
	return false;
    }
    if ( !safeio_->open(false) )
    {
	uiMSG().error( "Unable to open the file" );
	return false;
    }
    StreamConn* conno = new StreamConn( safeio_->strmdata() );
    if ( !tro_->initWrite( conno, *tbufchgdtrcs_.first()) )
    {
	uiMSG().error( "Unable to write" );
	return false;
    }
    if ( !tri_->readInfo(trc_.info()) )
    {
	uiMSG().error( "Input cube is empty" );
	return false;
    }
    return true;
}


    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Traces done"; }

protected:
int nextStep()
{
    if ( nrdone_ == 0 && !init() )
	return ErrorOccurred();

    if ( tri_->read(trc_) ) 
    {
	const int chgidx = tbufchgdtrcs_.find( trc_.info().binid, is2d_ );
	const bool res = chgidx<0 ? tro_->write( trc_ ) 
				  : tro_->write( *tbufchgdtrcs_.get(chgidx) );
	if ( !res )
	{
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

};


bool uiSeisBrowser::storeChgdData()
{
    PtrMan<uiSeisBrowseWriter> wrtr =
	new uiSeisBrowseWriter( setup_, tbufchgdtrcs_, is2D() );
    uiTaskRunner dlg( this );
    return dlg.execute( *wrtr );
}


void uiSeisBrowser::dispTracesPush( CallBacker* )
{
    if ( trcbufvwr_ )
	trcbufvwr_->start();
    else
    {
	uiSeisTrcBufViewer::Setup stbvsetup( "" );
	stbvsetup.withhanddrag(true);
	trcbufvwr_ = new uiSeisTrcBufViewer( this, stbvsetup );
	trcbufvwr_->selectDispTypes( true, false );
	trcbufvwr_->windowClosed.notify(
			 mCB(this,uiSeisBrowser,trcbufViewerClosed) );

	trcbufvwr_->setTrcBuf( &tbuf_, setup_.geom_, "Browsed seismic data",
		    		    IOM().nameOf(setup_.id_), compnr_ );
	trcbufvwr_->start(); trcbufvwr_->handleBufChange();

	if ( (tbuf_.isEmpty()) )
	    uiMSG().error( "No data at the specified position " );
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
    trcbufvwr_ = 0;
    updateWiggleButtonStatus();
}


void uiSeisBrowser::valChgReDraw( CallBacker* )
{
    commitChanges();
    const RowCol rc = tbl_->currentCell();
    if ( rc.row<0 || rc.col<0 ) return;

    SeisTrc* trace = tbuf_.get( rc.col );
    const float chgdval = tbl_->getfValue( rc );
    trace->set( rc.row, chgdval, compnr_ );
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisBrowser::setTrcBufViewTitle()
{
    if ( !trcbufvwr_ ) return;

    BufferString title( "Central Trace: " );
    if ( !Seis::is2D(setup_.geom_) )
	{ title += curBinID().inl; title += "/"; }
    title += curBinID().crl;

    trcbufvwr_->setWinTitle( title );

}


void uiSeisBrowser::chgCompNrCB( CallBacker* )
{
    NotifyStopper notifstop( tbl_->valueChanged );
    commitChanges();
    compnr_ = selcompnmfld_ ? selcompnmfld_->currentItem() : 0;
    fillTable();
}



uiSeisBrowserInfoVwr::uiSeisBrowserInfoVwr( uiParent* p, const SeisTrc& trc,
					    bool is2d, const ZDomain::Def& zd )
    : uiAmplSpectrum(p)
    , is2d_(is2d)  
    , zdomdef_(zd)  
{
    setDeleteOnClose( true );
    setCaption( "Trace information" );

    uiGroup* valgrp = new uiGroup( this, "Values group" );

    PositionInpSpec coordinpspec( PositionInpSpec::Setup(true,is2d_,false) ); 
    coordfld_ = new uiGenInput( valgrp, "Coordinate",
	   			coordinpspec.setName("X",0).setName("Y",0) );
    coordfld_->setReadOnly();

    BufferString label( is2d_ ? "Trace/Ref number" : sKey::Position() );
    IntInpSpec iis; FloatInpSpec fis;
    DataInpSpec* pdis = &iis; if ( is2d_ ) pdis = &fis;
    trcnrbinidfld_ = new uiGenInput( valgrp, label.buf(), iis, *pdis );
    trcnrbinidfld_->attach( alignedBelow, coordfld_ );
    trcnrbinidfld_->setReadOnly();

    minamplfld_ = new uiGenInput( valgrp, "Minimum amplitude", FloatInpSpec() );
    minamplfld_->attach( alignedBelow, trcnrbinidfld_ );
    minamplfld_->setElemSzPol( uiObject::Small );
    minamplfld_->setReadOnly();
    minamplatfld_ = new uiGenInput( valgrp, "at", FloatInpSpec() );
    minamplatfld_->attach( rightOf, minamplfld_ );
    minamplatfld_->setElemSzPol( uiObject::Small );
    minamplatfld_->setReadOnly();
    uiLabel* lbl = new uiLabel( valgrp, zdomdef_.unitStr(true) );
    lbl->attach( rightOf, minamplatfld_ );

    maxamplfld_ = new uiGenInput( valgrp, "Maximum amplitude", FloatInpSpec() );
    maxamplfld_->attach( alignedBelow, minamplfld_ );
    maxamplfld_->setElemSzPol( uiObject::Small );
    maxamplfld_->setReadOnly();
    maxamplatfld_ = new uiGenInput( valgrp, "at", FloatInpSpec() );
    maxamplatfld_->attach( rightOf, maxamplfld_ );
    maxamplatfld_->setElemSzPol( uiObject::Small );
    maxamplatfld_->setReadOnly();
    lbl = new uiLabel( valgrp, zdomdef_.unitStr(true) );
    lbl->attach( rightOf, maxamplatfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, dispparamgrp_ );
    valgrp->attach( centeredBelow, sep );
    valgrp->attach( ensureBelow, sep );

    setPrefHeightInChar( 20 );
    setPrefWidthInChar( 50 );

    setTrace( trc );
}


void uiSeisBrowserInfoVwr::setTrace( const SeisTrc& trc )
{
    coordfld_->setValue( trc.info().coord );
    if ( !is2d_ )
	trcnrbinidfld_->setValue( trc.info().binid );
    else
    {
	trcnrbinidfld_->setValue( trc.info().binid.crl, 0 );
	trcnrbinidfld_->setValue( trc.info().refnr, 1 );
    }

    if ( trc.size() < 1 ) return;

    float v0, z0 = mUdf(float); int isamp;
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

    const float zfac = zdomdef_.userFactor();
    minamplfld_->setValue( amplrg.start );
    minamplatfld_->setText( getZValStr(peakzs.start,zfac) );
    maxamplfld_->setValue( amplrg.stop );
    maxamplatfld_->setText( getZValStr(peakzs.stop,zfac) );

    Array2DImpl<float> a2d( 1, vals.size() );
    for ( int idx=0; idx<vals.size(); idx++ )
	a2d.set( 0, idx, vals[idx] );
    setData( a2d );
}
