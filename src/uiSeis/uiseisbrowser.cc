/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.cc,v 1.13 2008-01-10 11:16:13 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiseisbrowser.h"

#include "uibutton.h"
#include "uiexecutor.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uiseistrcbufviewer.h"
#include "uitable.h"
#include "uitoolbar.h"
#include "uitextedit.h"

#include "cbvsreadmgr.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "datapack.h"
#include "executor.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "ptrman.h"
#include "safefileio.h"
#include "samplingdata.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seisinfo.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include <iostream>



uiSeisBrowser::uiSeisBrowser( uiParent* p, const uiSeisBrowser::Setup& setup )
    : uiDialog(p,setup)
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
    , stepout_(5)
    , compnr_(0)
    , nrcomps_(1)
    , sd_(0)
    , strcbufview_(0)
    , setup_(setup)
{
    if ( !openData(setup) )
    {
	setTitleText( "Error" );
	BufferString lbltxt( "Cannot open input data (" );
	lbltxt += Seis::nameOf(setup.geom_); lbltxt += ")\n";
	lbltxt += IOM().nameOf( setup.id_ );
	if ( !setup.linekey_.isEmpty() )
	    { lbltxt += " - "; lbltxt += setup.linekey_; }
	new uiLabel( this, lbltxt );
	setCtrlStyle( LeaveOnly );
	return;
    }

    createMenuAndToolBar();
    createTable();

    setPos( setup.startpos_ );
    setZ( setup.startz_ );
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


const float uiSeisBrowser::curZ() const
{
    return sd_.start + tbl_->currentRow() * sd_.step;
}


void uiSeisBrowser::setZ( float z ) 
{
    if ( mIsUdf(z) ) return;

    int newrow = (int)((int)z - (int)sd_.start) / (int)sd_.step;
    tbl_->setCurrentCell( RowCol(tbl_->currentCol(),newrow) );
}


bool uiSeisBrowser::openData( const uiSeisBrowser::Setup& setup )
{
    PtrMan<IOObj> ioobj = IOM().get( setup.id_ );
    if ( !ioobj ) return false;

    BufferString emsg;
    tr_ = CBVSSeisTrcTranslator::make( ioobj->fullUserExpr(true), false,
	    			Seis::is2D(setup.geom_), &emsg );
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
    uitb_->addButton( fnm, mCB(this,uiSeisBrowser,func), tip, toggle )

void uiSeisBrowser::createMenuAndToolBar()
{
    uitb_ = new uiToolBar( this, "Tool Bar" );
    mAddButton("gotopos.png",goToPush,"",false );
    mAddButton("info.png",infoPush,"",false );
    crlwisebutidx_ = mAddButton("crlwise.png",switchViewTypePush,"",true );
    mAddButton("leftarrow.png",leftArrowPush,"",false );
    mAddButton("rightarrow.png",rightArrowPush,"",false );
    showwgglbutidx_ = mAddButton("viewflat.png",showWigglePush,"",false );
}


void uiSeisBrowser::createTable()
{
    uiTable::Size sz( 2 * stepout_ + 1, tr_->readMgr()->info().nrsamples );
    tbl_ = new uiTable( this, uiTable::Setup()
			     .size(sz) 
			     .selmode(uiTable::Multi)
    			     .manualresize( true ) );
    
    tbl_->valueChanged.notify( mCB(this,uiSeisBrowser,valChgReDraw) );
    tbl_->setStretch(1,1);
}


BinID uiSeisBrowser::getNextBid( const BinID& cur, int idx,
				   bool before ) const
{
    const BinID& step = tr_->readMgr()->info().geom.step;
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
    if ( !tbl_ )
	return false;
    if ( !force && bid == ctrc_.info().binid )
    {
	//setZ( z );
	return true;
    }

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
	uiMSG().warning( "Cannot read data at specified location" );
    if ( !havetrc || !canread )
    {	
	ctrc_.info().binid = bid;
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

    for ( int idx=0; idx<tbuf_.size(); idx++ )
	tbuf_.get(idx)->info().nr = idx;

    fillTable();
    //setZ( z );
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


void uiSeisBrowser::fillTable()
{
    const CBVSInfo& info = tr_->readMgr()->info();
    const float zfac = SI().zFactor();
    BufferString lbl;
    for ( int idx=0; idx<info.nrsamples; idx++ )
    {
	float dispz = zfac * info.sd.atIndex( idx ) * 10;
	int idispz = mNINT( dispz );
	dispz = idispz * 0.1;
	lbl = dispz;
	tbl_->setRowLabel( idx, lbl );
    }

    for ( int idx=0; idx<tbuf_.size(); idx++ )
	fillTableColumn( idx );
}


void uiSeisBrowser::fillTableColumn( int colidx )
{
    const SeisTrc& trc = *tbuf_.get( colidx );

    RowCol rc; rc.col = colidx;
    BufferString lbl;
    if ( is2D() )
	lbl = trc.info().nr;
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
				   "0.0.0") )
{
    PositionInpSpec inpspec(
	    PositionInpSpec::Setup(false,is2d,isps).binid(cur) );
    posfld_ = new uiGenInput( this, "New Position", inpspec );
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


void uiSeisBrowser::goTo( const BinID& bid )
{
    doSetPos( bid, true );
}


class uiSeisBrowserInfoDlg : public uiDialog
{
public:

uiSeisBrowserInfoDlg( uiParent* p, SeisTrc& ctrc_ )
    : uiDialog( p, uiDialog::Setup("Info","","0.0.0") )
{
    uiTextEdit* infofld_ = new uiTextEdit( this, "Trace Info", true );
    
    BufferString txt;
    txt += "Coordinate of central trace: ";
    txt += ctrc_.info().coord.x; txt += ", ";
    txt += ctrc_.info().coord.y;

    infofld_->setText( txt );
}
};


void uiSeisBrowser::infoPush( CallBacker* )
{
    uiSeisBrowserInfoDlg dlg( this, ctrc_); 
    dlg.go(); 
}


void uiSeisBrowser::goToPush( CallBacker* )
{
    uiSeisBrowserGoToDlg dlg( this, curBinID(),is2D() );
    if ( dlg.go() )
	/* user pressed OK AND input is OK */
	setPos( dlg.pos_ );
    setTrcBufViewTitle();
    if (strcbufview_)
        strcbufview_->handleBufChange();
}


void uiSeisBrowser::rightArrowPush( CallBacker* )
{
    goTo( getNextBid(curBinID(),stepout_,false) );
    setTrcBufViewTitle();
    if (strcbufview_)
	strcbufview_->handleBufChange();
}


void uiSeisBrowser::leftArrowPush( CallBacker* )
{
    goTo( getNextBid(curBinID(),stepout_,true) );
    setTrcBufViewTitle();
    if (strcbufview_)
	strcbufview_->handleBufChange();
}


void uiSeisBrowser::switchViewTypePush( CallBacker* )
{
    crlwise_ = uitb_->isOn( crlwisebutidx_ );
    doSetPos( curBinID(), true );
    setTrcBufViewTitle();
    if ( strcbufview_ )
	strcbufview_->handleBufChange();
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
	    const float tableval = tbl_->getValue( pos );
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


bool uiSeisBrowser::acceptOK( CallBacker* )
{
    commitChanges();
    if ( tbufchgdtrcs_.isEmpty() )
	return true;

    if ( uiMSG(). askGoOn(" Do you want to save the changes permanently? ",
	          	   true));
	storeChgdData();
    //TODO store traces if user wants to
    return true;
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
	 uiMSG().error( "" );
	return false;
    if ( !safeio_->open(false) )
	uiMSG().error( "Unable to open the file" );
	return false;
    StreamConn* conno = new StreamConn( safeio_->strmdata() );
    if ( !tro_->initWrite( conno, *tbufchgdtrcs_.get(0)) )
	uiMSG().error( "Unable to write" );
	return false;
    if ( !tri_->readInfo(trc_.info()) )
    {
	uiMSG().error( "Input cube is empty" );
	return false;
    }
    return true;
}


    int			totalNr() const		{ return totalnr_; }
    int                 nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Traces done"; }

protected:
int nextStep()
{
    if ( nrdone_ == 0 && !init() )
	return ErrorOccurred;

    if ( tri_->read(trc_) ) 
    {
	const int chgidx = tbufchgdtrcs_.find( trc_.info().binid, is2d_ );
	const bool res = chgidx<0 ? tro_->write( trc_ ) 
				  : tro_->write( *tbufchgdtrcs_.get(chgidx) );
	if ( !res )
	{
	    safeio_->closeFail();
	    return ErrorOccurred;
	}

	nrdone_++;
	return MoreToDo;
    }

    safeio_->closeSuccess();
    return Finished;
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
    
    uiSeisBrowseWriter* wrtr = new uiSeisBrowseWriter( setup_, tbufchgdtrcs_,
	    					       is2D() );
    uiExecutor dlg( this, *wrtr );
    return dlg.go();
}


void uiSeisBrowser::showWigglePush( CallBacker* )
{
    if ( strcbufview_ )
	strcbufview_->start();
    else
    {
	const char* name = IOM().nameOf( setup_.id_ );
	uiSeisTrcBufViewer::Setup stbvsetup( title, 1 );
	strcbufview_ = new uiSeisTrcBufViewer( this, stbvsetup );
	SeisTrcBufDataPack* dp = strcbufview_->setTrcBuf
	                            ( &tbuf_, setup_.geom_, "Seismics", name);
	dp->trcBufArr2D().setBufMine( false );
	strcbufview_->getViewer()->usePack( true, dp->id() );
	strcbufview_->start();
	strcbufview_->windowClosed.notify(
			 mCB(this,uiSeisBrowser,trcbufViewerClosed) );
    }

    updateWiggleButtonStatus();
    setTrcBufViewTitle();
}


void uiSeisBrowser::updateWiggleButtonStatus()
{
    const bool turnon = !strcbufview_ || strcbufview_->isHidden();
    uitb_->turnOn( showwgglbutidx_, turnon );
}


void uiSeisBrowser::trcbufViewerClosed( CallBacker* )
{
    updateWiggleButtonStatus();
}


void uiSeisBrowser::valChgReDraw( CallBacker* )
{
    
    commitChanges();
    const RowCol rc = tbl_->currentCell();
    SeisTrc* trace = tbuf_.get( rc.col );
    const float chgdval = tbl_->getfValue( rc );
    trace->set( rc.row, chgdval, compnr_ );
    if ( strcbufview_ )
	strcbufview_->handleBufChange();
}


void uiSeisBrowser::setTrcBufViewTitle()
{
    BufferString title( "Central Trace :" );
    title += curBinID().inl; title += "/";
    title += curBinID().crl;
    if ( strcbufview_ )
        strcbufview_->setWinTitle( title );

}
