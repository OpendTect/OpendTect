/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2007
 RCS:		$Id: uisegyexamine.cc,v 1.1 2008-09-11 13:56:09 cvsbert Exp $
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
#include "iostrm.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "seisread.h"
#include "segytr.h"
#include "segyhdr.h"
#include "iopar.h"
#include "timer.h"
#include "envvars.h"
#include <sstream>


uiSEGYExamine::Setup::Setup( int nrtraces )
    : uiDialog::Setup("SEG-Y Examiner",0,mNoHelpID)
    , nrtrcs_(nrtraces)
    , filenrs_(mUdf(int),0,1)
    , ns_(-1)
    , fmt_(-1)
    , nrzeropad_(-1)
{
    nrstatusflds( 2 ).modal( false );
    nrzeropad_ = GetEnvVarIVal( "OD_SEGY_NRZEROS_PADDING", nrzeropad_ );
}


uiSEGYExamine::uiSEGYExamine( uiParent* p, const char* fnm,
			      const uiSEGYExamine::Setup& su )
	: uiDialog(p,su)
	, fname_(fnm)
	, setup_(su)
	, timer_(*new Timer("Startup timer"))
	, tbuf_(*new SeisTrcBuf(true))
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

    uiTable::Setup tblsu( SegyTraceheader::nrVals(), setup_.nrtrcs_ );
    tblsu.rowdesc("Header field").coldesc("Trace");
    tbl_ = new uiTable( tblgrp, tblsu, "Trace info" );
    tbl_->setPrefHeightInChar( 14 );
    tbl_->attach( centeredBelow, lbl );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( txtgrp );
    splitter->addGroup( tblgrp );

    finaliseDone.notify( mCB(this,uiSEGYExamine,onStartUp) );
}


uiSEGYExamine::~uiSEGYExamine()
{
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
	    				      FilePath(fname_).fileName() );
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
    txtfld_->setText( txtinfo_ );
    setName( fname_ );
}


void uiSEGYExamine::updateInp()
{
    MultiID tmpid( "100010." ); tmpid += BufferString(IOObj::tmpID);
    IOStream iostrm( fname_, tmpid.buf() );
    iostrm.setFileName( fname_ );
    iostrm.setGroup( "Seismic Data" );
    iostrm.setTranslator( "SEG-Y" );
    iostrm.setFileName( fname_ );
    bool ismulti = !mIsUdf(setup_.filenrs_.start);
    if ( ismulti )
    {
	iostrm.fileNumbers() = setup_.filenrs_;
	iostrm.setZeroPadding( setup_.nrzeropad_ );
    }
    if ( setup_.ns_ > 0 )
	iostrm.pars().set(SEGYSeisTrcTranslator::sExternalNrSamples,setup_.ns_);
    if ( setup_.fmt_ >= 0 )
	iostrm.pars().set( SEGYSeisTrcTranslator::sNumberFormat, setup_.fmt_ );

    toStatusBar( fname_, 1 );
    outInfo( "Opening input" );
    SeisTrcReader rdr( &iostrm );
    if ( *rdr.errMsg() || !rdr.prepareWork(Seis::PreScan) )
	{ txtinfo_ = rdr.errMsg(); return; }

    BufferString str( "Reading first " );
    str += setup_.nrtrcs_; str += " traces ...";
    outInfo( str );

    SeisTrc trc; int nrdone = 0;
    bool stoppedatend = false;
    const int nrvals = SegyTraceheader::nrVals();
    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr.translator())
    if ( !tr )
	{ txtinfo_ = "Internal: cannot obtain SEG-Y Translator"; return; }

    const SegyTraceheader& trhead = tr->trcHeader();
    for ( int itrc=0; itrc<setup_.nrtrcs_; itrc++ )
    {
	if ( !rdr.get(trc) )
	    { stoppedatend = true; break; }

	if ( nrdone == 0 )
	    handleFirstTrace( trc, *tr );

	for ( int ival=0; ival<nrvals; ival++ )
	{
	    SegyTraceheader::Val val = trhead.getVal( ival+1 );
	    RowCol rc( ival, itrc );
	    tbl_->setValue( rc, val.val_ );
	}

	nrdone++;
	trc.info().nr = nrdone;
	tbuf_.add( new SeisTrc(trc) );
    }

    str = "\n\n";
    str += nrdone < 1 ? "No traces found"
	   : (stoppedatend ? (ismulti ? "Number of traces in first file: "
		       		      : "Total traces present in file: ")
			   : "Traces displayed: ");
    if ( nrdone > 0 ) str += nrdone;
    txtinfo_ += str;
    outInfo( "" );
}


void uiSEGYExamine::handleFirstTrace( const SeisTrc& trc,
				      const SEGYSeisTrcTranslator& tr )
{
    const SegyTxtHeader& txthead = tr.txtHeader();
    const SegyBinHeader& binhead = tr.binHeader();
    std::ostringstream thstrm, bhstrm;
    txthead.dump( thstrm );
    binhead.dump( bhstrm );

    txtinfo_ = thstrm.str().c_str();
    txtinfo_ += "\n------\n"
		"Binary header info (non-zero values displayed only):\n\n";
    txtinfo_ += bhstrm.str().c_str();

    const SegyTraceheader& trhead = tr.trcHeader();
    const int nrvals = SegyTraceheader::nrVals();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	SegyTraceheader::Val val = trhead.getVal( ival+1 );
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
