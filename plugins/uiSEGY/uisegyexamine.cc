/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2007
________________________________________________________________________

-*/

#include "uisegyexamine.h"
#include "uisegytrchdrvalplot.h"
#include "uitextedit.h"
#include "uitable.h"
#include "uisplitter.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uiflatviewer.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "uiseistrcbufviewer.h"
#include "uistatsdisplaywin.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "seisread.h"
#include "seisbufadapters.h"
#include "segytr.h"
#include "segyhdr.h"
#include "iopar.h"
#include "timer.h"
#include "envvars.h"
#include "separstr.h"
#include "oscommand.h"
#include "od_strstream.h"
#include "oddirs.h"
#include "dbman.h"
#include "od_helpids.h"

const char* uiSEGYExamine::Setup::sKeyNrTrcs = "Examine.Number of traces";


uiSEGYExamine::Setup::Setup( int nrtraces )
    : uiDialog::Setup(tr("SEG-Y Examiner"),mNoDlgTitle,
                      mODHelpKey(mSEGYExamineHelpID) )
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
	, tbuf_(*new SeisTrcBuf(true))
	, rdr_(0)
{
    setCtrlStyle( CloseOnly );

    uiGroup* txtgrp = new uiGroup( this, "Txt fld group" );
    uiLabel* lbl = new uiLabel( txtgrp, tr("File header information") );
    uiToolButton* savesettb = new uiToolButton( txtgrp, "save",
					 tr("Save textual header to a file"),
				         mCB(this,uiSEGYExamine,saveHdr) );
    savesettb->attach( rightBorder );
    txtfld_ = new uiTextEdit( txtgrp, "", true );
    txtfld_->setPrefHeightInChar( 14 );
    txtfld_->setPrefWidthInChar( 80 );
    txtfld_->attach( ensureBelow, lbl );

    uiGroup* logrp = new uiGroup( this, "Low group" );
    uiGroup* tblgrp = new uiGroup( logrp, "Table group" );
    lbl = new uiLabel( tblgrp, tr("Trace header information") );
    uiToolButton* seistb = new uiToolButton( tblgrp, "vd", tr("Display traces"),
			     mCB(this,uiSEGYExamine,dispSeis) );

    uiTable::Setup tblsu( SEGY::TrcHeader::hdrDef().size(), setup_.nrtrcs_ );
    tblsu.rowdesc("Header field").coldesc("Trace").selmode(uiTable::SingleRow);
    tbl_ = new uiTable( tblgrp, tblsu, "Trace info" );
    tbl_->setPrefHeightInChar( 14 );
    tbl_->setPrefWidthInChar( 40 );
    tbl_->attach( ensureBelow, lbl );
    for ( int icol=0; icol<setup_.nrtrcs_; icol++ )
    {
	const int tidx = icol + 1;
	uiString tt( tr("Trace header info from %1").arg(tidx) );
	tt.append(" %1 trace").arg(getRankPostFix(tidx));
	tbl_->setColumnLabel( icol, toUiString(tidx) );
	tbl_->setColumnToolTip( icol, tt );
	tbl_->setColumnReadOnly( icol, true );
    }
    tbl_->selectionChanged.notify( mCB(this,uiSEGYExamine,rowClck) );
    uiToolButton* histtb = new uiToolButton( tblgrp, "histogram",
			    tr("Show histogram of sample values"),
			    mCB(this,uiSEGYExamine,dispHist) );
    histtb->attach( rightAlignedAbove, tbl_ );
    seistb->attach( leftOf, histtb );

    hvaldisp_ = new uiSEGYTrcHdrValPlot( this, true );

    uiSplitter* vsplit = new uiSplitter( logrp, "VSplitter", true );
    vsplit->addGroup( tblgrp );
    vsplit->addGroup( hvaldisp_ );

    uiSplitter* hsplit = new uiSplitter( this, "HSplitter", false );
    hsplit->addGroup( txtgrp );
    hsplit->addGroup( logrp );

    toStatusBar( toUiString(setup_.fs_.dispName()), 1 );
    outInfo( tr("Opening input") );
    rdr_ = getReader( setup_, txtinfo_ );
    txtfld_->setText( txtinfo_ );

    uiString str( m3Dots(tr("Reading first %1 traces").arg(su.nrtrcs_)) );
    outInfo( str );

    afterPopup.notify( mCB(this,uiSEGYExamine,updateInput) );
}


uiSEGYExamine::~uiSEGYExamine()
{
    delete rdr_;
    delete &tbuf_;
}


void uiSEGYExamine::rowClck( CallBacker* )
{
    setRow( tbl_->currentRow() );
}


void uiSEGYExamine::saveHdr( CallBacker* )
{
    if ( !rdr_ ) return;
    uiFileDialog dlg( this, false,
		      File::Path(GetDataDir(),"Seismics").fullPath(), 0,
		      tr("Save SEG-Y Textual Header to") );
    if ( !dlg.go() ) return;

    od_ostream strm( dlg.fileName() );
    if ( !strm.isOK() )
	{ uiMSG().error(tr("Cannot open file for writing")); return; }

    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr_->translator())
    const SEGY::TxtHeader& th = *trans->txtHeader();
    BufferString buf; th.getText( buf );
    strm << buf << od_endl;
}


uiString uiSEGYExamine::sGetWinTitle()
{
    const BufferString fnm( File::Path(setup_.fs_.dispName()).fileName() );

    return ( tr("First %1 traces from %2").arg(tbuf_.size())
			     .arg(fnm) );
}

void uiSEGYExamine::dispSeis( CallBacker* )
{
    sGetWinTitle();
    uiSeisTrcBufViewer* vwr = new uiSeisTrcBufViewer( this,
				uiSeisTrcBufViewer::Setup(sGetWinTitle()) );
    vwr->selectDispTypes( true, true );
    vwr->setTrcBuf( tbuf_, Seis::Line, "SEG-Y.Examine", "sample value" );
    vwr->start(); vwr->handleBufChange();
}


void uiSEGYExamine::dispHist( CallBacker* )
{
    sGetWinTitle();
    SeisTrcBufArray2D a2d( &tbuf_, 0 );
    uiStatsDisplay::Setup su; su.withname( false );
    uiStatsDisplayWin* mw = new uiStatsDisplayWin( this, su, 1, false );
    mw->statsDisplay(0)->setData( &a2d );
    mw->setDeleteOnClose( true );
    mw->setCaption( sGetWinTitle() );
    mw->show();
}


void uiSEGYExamine::updateInput( CallBacker* )
{
    display( true );
    updateInp();
    setName( setup_.fs_.dispName() );
}


void uiSEGYExamine::setRow( int irow )
{
    if ( irow < 0 ) return;

    const int nrcols = tbl_->nrCols();
    TypeSet<float> data;
    for ( int icol=0; icol<nrcols; icol++ )
    {
	const char* txt = tbl_->text( RowCol(irow,icol) );
	if ( !txt || !*txt )
	    break;
	data += Conv::to<float>( txt );
    }

    hvaldisp_->setData( *SEGY::TrcHeader::hdrDef()[irow],
			data.arr(), data.size() );
}


SeisTrcReader* uiSEGYExamine::getReader( const uiSEGYExamine::Setup& su,
					 BufferString& emsg )
{
    IOObj* ioobj = su.fs_.getIOObj( true );
    if ( !ioobj )
	return 0;

    DBM().setEntry( *ioobj );
    su.fp_.fillPar( ioobj->pars() );

    SeisTrcReader* rdr = new SeisTrcReader( ioobj );
    delete ioobj;
    if ( !rdr->errMsg().isEmpty() || !rdr->prepareWork(Seis::PreScan) )
	{ emsg = rdr->errMsg().getFullString(); delete rdr; return 0; }

    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr->translator())
    if ( !trans )
	{ emsg = "Internal: cannot obtain SEG-Y Translator";
		delete rdr; return 0; }

    return rdr;
}


SeisTrcReader* uiSEGYExamine::getReader( const uiSEGYExamine::Setup& su,
					 uiString& emsg )
{
    IOObj* ioobj = su.fs_.getIOObj( true );
    if ( !ioobj )
	return 0;

    DBM().setEntry( *ioobj );
    su.fp_.fillPar( ioobj->pars() );

    SeisTrcReader* rdr = new SeisTrcReader( ioobj );
    delete ioobj;
    if ( !rdr->errMsg().isEmpty() || !rdr->prepareWork(Seis::PreScan) )
	{ emsg = rdr->errMsg(); delete rdr; return 0; }

    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr->translator())
    if ( !trans )
	{ emsg = tr("Internal: cannot obtain SEG-Y Translator");
		delete rdr; return 0; }

    return rdr;
}


int uiSEGYExamine::getRev() const
{
    return rdr_ ? getRev( *rdr_ ) : -1;
}


int uiSEGYExamine::getRev( const uiSEGYExamine::Setup& su, BufferString& emsg )
{
    uiString errmsg;
    PtrMan<SeisTrcReader> rdr = getReader( su, errmsg );
    emsg = mFromUiStringTodo(errmsg);
    if ( !rdr && emsg.isEmpty() )
	emsg.set( "Error opening file."
	    "\nPlease check whether the file size is at least 3600 bytes." );
    return rdr ? getRev( *rdr ) : -1;
}


int uiSEGYExamine::getRev( const uiSEGYExamine::Setup& su, uiString& emsg )
{
    PtrMan<SeisTrcReader> rdr = getReader( su, emsg );
    if ( !rdr && emsg.isEmpty() )
	emsg = uiStrings::phrCannotOpen(tr("file."
	    "\nPlease check whether the file size is at least 3600 bytes."));
    return rdr ? getRev( *rdr ) : -1;
}


int uiSEGYExamine::getRev( const SeisTrcReader& rdr )
{
    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr.translator());
    if ( !trans ) return -1;
    return trans->isRev0() ? 0 : 1;
}


bool uiSEGYExamine::launch( const uiSEGYExamine::Setup& su )
{
    BufferString cmd( "od_SEGYExaminer --nrtrcs " );
    cmd += su.nrtrcs_;
    if ( su.fp_.ns_ > 0 )
	{ cmd += " --ns "; cmd += su.fp_.ns_; }
    if ( su.fp_.fmt_ > 0 )
	{ cmd += " --fmt "; cmd += su.fp_.fmt_; }
    if ( su.fp_.byteswap_ )
	{ cmd += " --swapbytes "; cmd += su.fp_.byteswap_; }
    if ( su.fs_.isMulti() )
    {
	FileMultiString fms;
	fms += su.fs_.nrs_.start;
	fms += su.fs_.nrs_.stop;
	fms += su.fs_.nrs_.step;
	if ( su.fs_.zeropad_ > 1 )
	    fms += su.fs_.zeropad_;
	cmd += " --filenrs '"; cmd += fms; cmd += "'";
    }

    BufferString fnm( su.fs_.fileName() );
    fnm.replace( "*", "+x+" );
    return ExecODProgram( cmd, fnm );
}


void uiSEGYExamine::updateInp()
{
    if ( !rdr_ || !tbuf_.isEmpty() ) return;

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr_->translator())
    const SEGY::TrcHeader& trhead = trans->trcHeader();

    SeisTrc trc; int nrdone = 0;
    bool stoppedatend = false;
    for ( int itrc=0; itrc<setup_.nrtrcs_; itrc++ )
    {
	if ( !rdr_->get(trc) )
	    stoppedatend = true;
	if ( nrdone == 0 )
	    handleFirstTrace( trc, *trans );
	if ( stoppedatend )
	    break;

	for ( int ival=0; ival<nrvals; ival++ )
	{
	    int val = trhead.entryVal( ival );
	    RowCol rc( ival, itrc );
	    tbl_->setValue( rc, val );
	}

	nrdone++;
	tbuf_.add( new SeisTrc(trc) );
    }
    tbl_->setNrCols( nrdone > 0 ? nrdone : 1 );

    if ( stoppedatend || nrdone < 1 )
    {
	uiString str( toUiString("\n\n----  ") );
	const bool ismulti = !mIsUdf(setup_.fs_.nrs_.start);
	if ( nrdone < 1 )
	    str.append(tr(" No traces found"));
	else
	{
	    str.append(tr(" Total number of traces present in file"));
	    str.append(toUiString(": ")); str.append(toUiString(nrdone));
	}
	str.append(toUiString("  ----"));
	txtinfo_.append(" ").append(str);
    }
    outInfo( uiString::emptyString() );
    txtfld_->setText( txtinfo_ );
}


void uiSEGYExamine::handleFirstTrace( const SeisTrc& trc,
				      const SEGYSeisTrcTranslator& trans )
{
    const SEGY::TxtHeader& txthead = *trans.txtHeader();
    const SEGY::BinHeader& binhead = trans.binHeader();
    od_ostrstream thstrm, bhstrm;
    txthead.dump( thstrm );
    binhead.dump( bhstrm );

    txtinfo_ = toUiString(thstrm.result());
    txtinfo_.append(tr("\n------\n\n"
		"Binary header info (non-zero values displayed only):\n\n"));
    txtinfo_.append(bhstrm.result());

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	const SEGY::HdrEntry& he( *hdef[ival] );
	BufferString rownm( "", ((int)he.bytepos_)+1 );
	rownm.add( " [" ).add( he.name() ).add( "]" );
	tbl_->setRowLabel( ival, rownm );
	tbl_->setRowToolTip( ival, he.description() );
    }

    tbl_->resizeRowsToContents();
    tbl_->resizeHeaderToContents( false );
}


bool uiSEGYExamine::rejectOK()
{
    return true;
}


void uiSEGYExamine::outInfo( const uiString txt )
{
    toStatusBar( txt, 0 );}
