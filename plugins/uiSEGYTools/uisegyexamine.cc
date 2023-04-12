/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegyexamine.h"

#include "uifiledlg.h"
#include "uiflatviewer.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uisegytrchdrvalplot.h"
#include "uiseistrcbufviewer.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistatsdisplaywin.h"
#include "uitable.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "msgh.h"
#include "od_helpids.h"
#include "od_strstream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "ptrman.h"
#include "segyhdr.h"
#include "segytr.h"
#include "seisbufadapters.h"
#include "seisread.h"
#include "seistrc.h"
#include "separstr.h"
#include "timer.h"

const char* uiSEGYExamine::Setup::sKeyNrTrcs = "Examine.Number of traces";


uiSEGYExamine::Setup::Setup( Seis::GeomType gt, int nrtraces )
    : uiDialog::Setup(tr("SEG-Y Examiner"),mNoDlgTitle,
                      mODHelpKey(mSEGYExamineHelpID) )
    , geomtype_(gt)
    , nrtrcs_(nrtraces)
    , fp_(true)
{
    nrstatusflds( 2 ).modal( false );
}


void uiSEGYExamine::Setup::usePar( const IOPar& iop )
{
    Seis::getFromPar( iop, geomtype_ );
    fp_.usePar( iop );
    fs_.usePar( iop );
    iop.get( sKeyNrTrcs, nrtrcs_ );
}


void uiSEGYExamine::Setup::setFileName( const char* fnm )
{
    BufferString filenm( fnm );
#ifdef __win__
    if ( File::isLink(filenm) )
	filenm = File::linkTarget( filenm.str() );
#endif
    fs_.setFileName( fnm );
}



uiSEGYExamine::uiSEGYExamine( uiParent* p, const uiSEGYExamine::Setup& su )
    : uiDialog(p,su)
    , setup_(su)
    , tbuf_(*new SeisTrcBuf(true))
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

    uiTable::Setup tblsu( SEGY::TrcHeader::hdrDef().size(), setup_.nrtrcs_ );
    tblsu.rowdesc("Header field").coldesc("Trace").selmode(uiTable::SingleRow);
    tbl_ = new uiTable( tblgrp, tblsu, "Trace info" );
    tbl_->setPrefHeightInChar( 14 );
    tbl_->setPrefWidthInChar( 50 );
    tbl_->attach( ensureBelow, lbl );
    for ( int icol=0; icol<setup_.nrtrcs_; icol++ )
    {
	const int tidx = icol + 1;
	uiString tt( tr("Trace header info from %1 %2 trace").arg(tidx)
						.arg(getRankPostFix(tidx)) );
	tbl_->setColumnLabel( icol, toUiString(tidx) );
	tbl_->setColumnToolTip( icol, tt );
	tbl_->setColumnReadOnly( icol, true );
    }
    tbl_->selectionChanged.notify( mCB(this,uiSEGYExamine,rowClck) );


    auto* lsb = new uiLabeledSpinBox( tblgrp, tr("Trace / Step") );
    lsb->attach( leftAlignedBelow, tbl_ );
    trc0fld_ = lsb->box();
    trc0fld_->setToolTip( tr("First trace") );
    trc0fld_->setInterval(1,10000000,1);
    trc0fld_->setStretch( 2, 0 );
    mAttachCB( trc0fld_->valueChanged, uiSEGYExamine::firstTrcCB );

    stepfld_ = new uiSpinBox( tblgrp );
    stepfld_->setToolTip( tr("Number of traces to scroll back/forward") );
    stepfld_->setStretch( 2, 0 );
    stepfld_->setInterval( 1, 1000000, 1 );
    stepfld_->attach( rightTo, lsb );

    auto* prevbut = new uiToolButton( tblgrp, "leftarrow", tr("Scroll back"),
				mCB(this,uiSEGYExamine,backCB) );
    auto* nextbut = new uiToolButton( tblgrp, "rightarrow",tr("Scroll forward"),
				mCB(this,uiSEGYExamine,forwardCB) );
    prevbut->attach( rightTo, stepfld_ );
    nextbut->attach( rightTo, prevbut );

    uiToolButton* histtb = new uiToolButton( tblgrp, "histogram",
			    tr("Show histogram of sample values"),
			    mCB(this,uiSEGYExamine,dispHist) );
    histtb->attach( rightAlignedAbove, tbl_ );

    uiToolButton* seistb = new uiToolButton( tblgrp, "vd", tr("Display traces"),
			     mCB(this,uiSEGYExamine,dispSeis) );
    seistb->attach( leftOf, histtb );

    auto* nrtrcssb = new uiLabeledSpinBox( tblgrp, tr("Nr traces") );
    nrtrcssb->attach( leftOf, seistb );
    nrtrcsfld_ = nrtrcssb->box();
    mAttachCB( nrtrcsfld_->valueChanged, uiSEGYExamine::nrTrcsCB );
    nrtrcsfld_->setToolTip( tr("Number of traces to examine") );
    nrtrcsfld_->setInterval( 1, 100000, 1 );
    nrtrcsfld_->setValue( setup_.nrtrcs_ );
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
    updateMaxTrace();

    mAttachCB( afterPopup, uiSEGYExamine::updateInput );
}


uiSEGYExamine::~uiSEGYExamine()
{
    detachAllNotifiers();
    delete rdr_;
    delete &tbuf_;
}


void uiSEGYExamine::rowClck( CallBacker* )
{
    setRow( tbl_->currentRow() );
}


void uiSEGYExamine::saveHdr( CallBacker* )
{
    if ( !rdr_ )
	return;

    FilePath fp( setup_.fs_.fileName() );
    fp.setExtension( "sgyhdr" );
    uiFileDialog dlg( this, false, fp.fullPath(), nullptr,
		      tr("Save SEG-Y Textual Header to") );
    dlg.setDirectory( setup_.fs_.dirName() );
    if ( !dlg.go() )
	return;

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
    const BufferString fnm( FilePath(setup_.fs_.dispName()).fileName() );

    return tr("First %1 traces from %2").arg( tbuf_.size() ).arg( fnm );
}


void uiSEGYExamine::dispSeis( CallBacker* )
{
    uiSeisTrcBufViewer* vwr = new uiSeisTrcBufViewer( this,
				uiSeisTrcBufViewer::Setup(sGetWinTitle()) );
    vwr->selectDispTypes( true, true );
    vwr->setTrcBuf( tbuf_, Seis::Line, "SEG-Y.Examine", "sample value" );
    vwr->start(); vwr->handleBufChange();
}


void uiSEGYExamine::dispHist( CallBacker* )
{
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
    if ( !rdr_ )
	return;

    display( true );
    setName( setup_.fs_.dispName() );

    mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,rdr_->translator())
    if ( segytr )
    {
	const int estnrtrcs = segytr->estimatedNrTraces();
	if ( estnrtrcs < setup_.nrtrcs_ )
	    setup_.nrtrcs_ = estnrtrcs;

	nrtrcsfld_->setMaxValue( estnrtrcs );
    }

    updateInp();
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
    PtrMan<IOObj> ioobj = su.fs_.getIOObj( true );
    if ( !ioobj )
	return nullptr;

    if ( IOMan::isOK() )
	IOM().commitChanges( *ioobj );

    su.fp_.fillPar( ioobj->pars() );
    SeisStoreAccess::Setup ssasu( *ioobj.ptr(), &su.geomtype_ );
    if ( su.fp_.getCoordSys() )
	ssasu.coordsys( *su.fp_.getCoordSys().ptr() );

    PtrMan<SeisTrcReader> rdr = new SeisTrcReader( ssasu );
    if ( !rdr->prepareWork(Seis::PreScan) )
    {
	emsg = rdr->errMsg().getFullString();
	return nullptr;
    }

    return rdr.release();
}


int uiSEGYExamine::getRev() const
{
    return rdr_ ? getRev( *rdr_ ) : -1;
}


int uiSEGYExamine::getRev( const uiSEGYExamine::Setup& su, BufferString& emsg )
{
    PtrMan<SeisTrcReader> rdr = getReader( su, emsg );
    if ( !rdr && emsg.isEmpty() )
	emsg.set( "Error opening file."
	    "\nPlease check whether the file size is at least 3600 bytes." );
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
    OS::MachineCommand cmd( "od_SEGYExaminer" );
    Seis::putInMC( su.geomtype_, cmd );
    cmd.addKeyedArg( "nrtrcs", su.nrtrcs_ );
    if ( su.fp_.ns_ > 0 ) cmd.addKeyedArg( "ns", su.fp_.ns_ );
    if ( su.fp_.fmt_ > 0 ) cmd.addKeyedArg( "fmt", su.fp_.fmt_ );
    if ( su.fp_.byteswap_ ) cmd.addKeyedArg( "swapbytes", su.fp_.byteswap_ );
    if ( su.fs_.isMulti() )
    {
	FileMultiString fms;
	fms += su.fs_.nrs_.start;
	fms += su.fs_.nrs_.stop;
	fms += su.fs_.nrs_.step;
	if ( su.fs_.zeropad_ > 1 )
	    fms += su.fs_.zeropad_;
	cmd.addKeyedArg( "filenrs", fms );
    }

    BufferString fnm( su.fs_.fileName() );
    fnm.replace( "*", "+x+" );
    cmd.addArg( fnm );
    return cmd.execute( OS::RunInBG );
}


void uiSEGYExamine::firstTrcCB( CallBacker* )
{
    const int firsttrc = trc0fld_->getIntValue();
    if ( firsttrc != firsttrace_ )
	updateInp();
}


void uiSEGYExamine::backCB( CallBacker* )
{
    const int firsttrc = trc0fld_->getIntValue();
    const int step = stepfld_->getIntValue();
    trc0fld_->setValue( firsttrc-step );
    updateInp();
}


void uiSEGYExamine::forwardCB( CallBacker* )
{
    const int firsttrc = trc0fld_->getIntValue();
    const int step = stepfld_->getIntValue();
    trc0fld_->setValue( firsttrc+step );
    updateInp();
}


void uiSEGYExamine::nrTrcsCB( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->getIntValue();
    if ( nrtrcs == setup_.nrtrcs_ )
	return;

    setup_.nrtrcs_ = nrtrcs;
    updateMaxTrace();
    updateInp();
}


void uiSEGYExamine::updateMaxTrace()
{
    mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,
		    rdr_ ? rdr_->translator() : nullptr)
    if ( !segytr )
	return;

    int maxval = segytr->estimatedNrTraces()-setup_.nrtrcs_+1;
    if ( maxval < 1 )
	maxval = 1;
    trc0fld_->setMaxValue( maxval );
}


void uiSEGYExamine::updateInp()
{
    if ( !rdr_ )
	return;

    MouseCursorChanger mcc( MouseCursor::Wait );

    uiString rdstr( m3Dots(tr("Reading %1 traces").arg(setup_.nrtrcs_)) );
    outInfo( rdstr );

    tbuf_.erase();
    const int selrow = tbl_->currentRow();
    if ( tbl_->nrCols() < setup_.nrtrcs_ )
	tbl_->setNrCols( setup_.nrtrcs_ );

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr_->translator())
    const SEGY::TrcHeader& trhead = trans->trcHeader();

    SeisTrc trc; int nrdone = 0;
    bool stoppedatend = false;
    firsttrace_ = trc0fld_->getIntValue();
    trans->goToTrace( firsttrace_-1 );
    const float sampstep = trans->binHeader().sampleRate( !SI().zIsTime() );
    for ( int itrc=0; itrc<setup_.nrtrcs_; itrc++ )
    {
	if ( !trans->read(trc) )
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

	if ( !trhead.isusable )
	    tbl_->setColor( RowCol(SEGY::TrcHeader::EntryTrid(),itrc),
			    OD::Color::Red() );

	nrdone++;
	trc.info().seqnr_ = nrdone;
	trc.info().sampling.step = sampstep;
	tbuf_.add( new SeisTrc(trc) );
    }
    tbl_->setNrCols( nrdone > 0 ? nrdone : 1 );

    if ( stoppedatend || nrdone < 1 )
    {
	BufferString str( "\n\n----  " );
	const bool ismulti = !mIsUdf(setup_.fs_.nrs_.start);
	if ( nrdone < 1 )
	    str += "No traces found";
	else
	{
	    str += "Total number of traces present in file";
	    if ( ismulti ) str += "s";
	    str += ": "; str += nrdone;
	}
	str += "  ----";
	txtinfo_ += str;
    }

    const int estnrtrcs = trans->estimatedNrTraces();
    outInfo( tr("Total traces: %1").arg(estnrtrcs) );
    txtfld_->setText( txtinfo_ );
    tbl_->selectRow( selrow );
    if ( selrow==0 )
	setRow( selrow );
}


void uiSEGYExamine::handleFirstTrace( const SeisTrc& trc,
				      const SEGYSeisTrcTranslator& trans )
{
    const SEGY::TxtHeader& txthead = *trans.txtHeader();
    const SEGY::BinHeader& binhead = trans.binHeader();
    od_ostrstream thstrm, bhstrm;
    txthead.dump( thstrm );
    binhead.dump( bhstrm );

    txtinfo_ = thstrm.result();
    txtinfo_ += "\n------\n\n"
		"Binary header info (non-zero values displayed only):\n\n";
    txtinfo_ += bhstrm.result();

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	const SEGY::HdrEntry& he( *hdef[ival] );
	uiString rownm = toUiString("%1 [%2]")
				.arg(int(he.bytepos_)+1).arg(he.name());
	tbl_->setRowLabel( ival, rownm );
	tbl_->setRowToolTip( ival, mToUiStringTodo(he.description()) );
    }

    tbl_->resizeRowsToContents();
    tbl_->resizeHeaderToContents( false );
}


bool uiSEGYExamine::rejectOK(CallBacker*)
{
    return true;
}


void uiSEGYExamine::outInfo( const uiString& txt )
{
    toStatusBar( txt, 0 );
}
