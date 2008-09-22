/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2007
 RCS:		$Id: uisegyexamine.cc,v 1.4 2008-09-22 15:09:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyexamine.h"
#include "uitextedit.h"
#include "uitable.h"
#include "uisplitter.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigroup.h"
#include "uiflatviewer.h"
#include "uiseistrcbufviewer.h"
#include "pixmap.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "seisread.h"
#include "segytr.h"
#include "segyhdr.h"
#include "iopar.h"
#include "timer.h"
#include "envvars.h"
#include "separstr.h"
#include <sstream>

const char* uiSEGYExamine::Setup::sKeyNrTrcs = "Number of traces";


uiSEGYExamine::Setup::Setup( int nrtraces )
    : uiDialog::Setup("SEG-Y Examiner",mNoDlgTitle,mNoHelpID)
    , nrtrcs_(nrtraces)
    , fp_(true)
{
    nrstatusflds( 2 ).modal( false );
}


void uiSEGYExamine::Setup::usePar( const IOPar& iop )
{
    fp_.usePar( iop ); fs_.usePar( iop );
    iop.get( sKeyNrTrcs, nrtrcs_ );
}


uiSEGYExamine::uiSEGYExamine( uiParent* p, const uiSEGYExamine::Setup& su )
	: uiDialog(p,su)
	, setup_(su)
	, timer_(*new Timer("Startup timer"))
	, tbuf_(*new SeisTrcBuf(true))
	, rdr_(0)
{
    setCtrlStyle( LeaveOnly );

    uiGroup* txtgrp = new uiGroup( 0, "Txt fld group" );
    uiLabel* lbl = new uiLabel( txtgrp, "File header information" );
    txtfld_ = new uiTextEdit( txtgrp, "", true );
    txtfld_->setPrefHeightInChar( 14 );
    txtfld_->setPrefWidthInChar( 80 );
    txtfld_->attach( centeredBelow, lbl );

    uiGroup* tblgrp = new uiGroup( 0, "Table group" );
    lbl = new uiLabel( tblgrp, "Trace header information" );
    uiToolButton* tb = new uiToolButton( tblgrp, "Preview data",
	    				 ioPixmap("viewflat.png"),
				         mCB(this,uiSEGYExamine,dispSeis) );
    tb->setToolTip( "Display traces" );
    tb->attach( rightBorder );

    uiTable::Setup tblsu( SEGY::TrcHeader::nrVals(), setup_.nrtrcs_ );
    tblsu.rowdesc("Header field").coldesc("Trace");
    tbl_ = new uiTable( tblgrp, tblsu, "Trace info" );
    tbl_->setPrefHeightInChar( 14 );
    tbl_->attach( centeredBelow, lbl );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( txtgrp );
    splitter->addGroup( tblgrp );

    openInput();
    txtfld_->setText( txtinfo_ );

    finaliseDone.notify( mCB(this,uiSEGYExamine,onStartUp) );
}


uiSEGYExamine::~uiSEGYExamine()
{
    delete rdr_;
    delete &timer_;
    delete &tbuf_;
}


void uiSEGYExamine::onStartUp( CallBacker* )
{
    timer_.tick.notify( mCB(this,uiSEGYExamine,updateInput) );
    timer_.start( 100, true );
}


void uiSEGYExamine::dispSeis( CallBacker* )
{
    uiSeisTrcBufViewer::Setup su( "Trace display", 1 );
    uiSeisTrcBufViewer* vwr = new uiSeisTrcBufViewer( this, su );
    SeisTrcBufDataPack* dp = vwr->setTrcBuf( tbuf_, Seis::Line, "Examine",
				      FilePath(setup_.fs_.fname_).fileName() );
    vwr->getViewer()->usePack( true, dp->id(), true );
    vwr->getViewer()->usePack( false, dp->id(), true );
    vwr->getViewer()->appearance().ddpars_.show( true, true );
    vwr->start();
    vwr->handleBufChange();
    vwr->show();
}


void uiSEGYExamine::updateInput( CallBacker* )
{
    display( true );
    updateInp();
    setName( setup_.fs_.fname_ );
}


void uiSEGYExamine::openInput()
{
    IOObj* ioobj = setup_.fs_.getIOObj( true );
    setup_.fp_.fillPar( ioobj->pars() );

    toStatusBar( setup_.fs_.fname_, 1 );
    outInfo( "Opening input" );
    rdr_ = new SeisTrcReader( ioobj );
    delete ioobj;
    if ( *rdr_->errMsg() || !rdr_->prepareWork(Seis::PreScan) )
	{ txtinfo_ = rdr_->errMsg(); delete rdr_; rdr_ = 0; return; }

    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr_->translator())
    if ( !tr )
	{ txtinfo_ = "Internal: cannot obtain SEG-Y Translator";
	    	delete rdr_; rdr_ = 0; return; }

    BufferString str( "Reading first " );
    str += setup_.nrtrcs_; str += " traces ...";
    outInfo( str );
}


int uiSEGYExamine::getRev() const
{
    if ( !rdr_ ) return -1;
    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr_->translator())
    if ( !tr ) return -1;
    return tr->isRev1() ? 1 : 0;
}


void uiSEGYExamine::updateInp()
{
    if ( !rdr_ ) return;

    const int nrvals = SEGY::TrcHeader::nrVals();
    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr_->translator())
    const SEGY::TrcHeader& trhead = tr->trcHeader();

    SeisTrc trc; int nrdone = 0;
    bool stoppedatend = false;
    for ( int itrc=0; itrc<setup_.nrtrcs_; itrc++ )
    {
	if ( !rdr_->get(trc) )
	    { stoppedatend = true; break; }

	if ( nrdone == 0 )
	    handleFirstTrace( trc, *tr );

	for ( int ival=0; ival<nrvals; ival++ )
	{
	    SEGY::TrcHeader::Val val = trhead.getVal( ival+1 );
	    RowCol rc( ival, itrc );
	    tbl_->setValue( rc, val.val_ );
	}

	nrdone++;
	trc.info().nr = nrdone;
	tbuf_.add( new SeisTrc(trc) );
    }

    BufferString str( "\n\n" );
    const bool ismulti = !mIsUdf(setup_.fs_.nrs_.start);
    str += nrdone < 1 ? "No traces found"
	   : (stoppedatend ? (ismulti ? "Number of traces in first file: "
		       		      : "Total traces present in file: ")
			   : "Traces displayed: ");
    if ( nrdone > 0 ) str += nrdone;
    txtinfo_ += str;
    outInfo( "" );
    txtfld_->setText( txtinfo_ );
}


void uiSEGYExamine::handleFirstTrace( const SeisTrc& trc,
				      const SEGYSeisTrcTranslator& tr )
{
    const SEGY::TxtHeader& txthead = tr.txtHeader();
    const SEGY::BinHeader& binhead = tr.binHeader();
    std::ostringstream thstrm, bhstrm;
    txthead.dump( thstrm );
    binhead.dump( bhstrm );

    txtinfo_ = thstrm.str().c_str();
    txtinfo_ += "\n------\n"
		"Binary header info (non-zero values displayed only):\n\n";
    txtinfo_ += bhstrm.str().c_str();

    const SEGY::TrcHeader& trhead = tr.trcHeader();
    const int nrvals = SEGY::TrcHeader::nrVals();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	SEGY::TrcHeader::Val val = trhead.getVal( ival+1 );
	BufferString rownm;
	rownm += val.byte_+1; rownm += " [";
	rownm += val.desc_; rownm += "]";
	tbl_->setRowLabel( ival, rownm );
    }
}


bool uiSEGYExamine::rejectOK()
{
    return true;
}


void uiSEGYExamine::outInfo( const char* txt )
{
    toStatusBar( txt, 0 );
}
