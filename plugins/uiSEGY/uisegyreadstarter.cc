/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyreadstarter.h"

#include "uisegyreadstartinfo.h"
#include "uisegyreadfinisher.h"
#include "uisegyimptype.h"
#include "uisegyexamine.h"
#include "uisegydef.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uihistogramdisplay.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "segyhdr.h"
#include "seisinfo.h"
#include "dataclipper.h"
#include "filepath.h"
#include "dirlist.h"
#include "oddirs.h"
#include "od_istream.h"
#include "settings.h"
#include "timer.h"


uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, const FileSpec* fs )
    : uiDialog(p,uiDialog::Setup(tr("Import SEG-Y Data"),mNoDlgTitle,
				  mTODOHelpKey ) )
    , filespec_(fs?*fs:FileSpec())
    , filereadopts_(0)
    , veryfirstscan_(false)
    , userfilename_("x") // any non-empty
    , clipsampler_(*new DataClipSampler(100000))
    , filenamepopuptimer_(0)
{
    uiFileInput::Setup fisu( uiFileDialog::Gen, filespec_.fileName() );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true )
	.objtype( tr("SEG-Y") );
    inpfld_ = new uiFileInput( this, "Input file(s) (*=wildcard)",
				fisu );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );

    typfld_ = new uiSEGYImpType( this );
    typfld_->typeChanged.notify( mCB(this,uiSEGYReadStarter,typChg) );
    typfld_->attach( alignedBelow, inpfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, typfld_ );

    infofld_ = new uiSEGYReadStartInfo( this, loaddef_ );
    infofld_->attach( ensureBelow, sep );
    infofld_->loaddefChanged.notify( mCB(this,uiSEGYReadStarter,defChg) );

    uiGroup* examinegrp = new uiGroup( this, "Examine group" );
    examinebut_ = new uiToolButton( examinegrp, "examine",
				    uiString::emptyString(),
				    mCB(this,uiSEGYReadStarter,examineCB) );
    setExamineStatus();
    examinenrtrcsfld_ = new uiSpinBox( examinegrp, 0, "Examine traces" );
    examinenrtrcsfld_->setInterval( 0, 1000000, 10 );
    examinenrtrcsfld_->setHSzPol( uiObject::Small );
    examinenrtrcsfld_->setToolTip( tr("Number of traces to examine") );
    examinenrtrcsfld_->attach( centeredBelow, examinebut_ );
    int nrex = 1000; Settings::common().get( sKeySettNrTrcExamine, nrex );
    examinenrtrcsfld_->setInterval( 10, 1000000, 10 );
    examinenrtrcsfld_->setValue( nrex );
    examinegrp->attach( rightOf, infofld_ );

    uiGroup* histgrp = new uiGroup( this, "Histogram group" );
    uiHistogramDisplay::Setup hdsu;
    hdsu.noyaxis( false ).noygridline(true).annoty( false );
    ampldisp_ = new uiHistogramDisplay( histgrp, hdsu );
    ampldisp_->setTitle( tr("Amplitudes") );
    ampldisp_->setPrefHeight( 250 );
    clipfld_ = new uiSpinBox( histgrp, 1, "Clipping percentage" );
    clipfld_->setInterval( 0.f, 49.9f, 0.1f );
    clipfld_->setValue( 0.1f );
    clipfld_->setToolTip( tr("Percentage clip for display") );
    clipfld_->setSuffix( uiString("%") );
    clipfld_->setHSzPol( uiObject::Small );
    clipfld_->attach( rightOf, ampldisp_ );
    clipfld_->valueChanging.notify(
			mCB(this,uiSEGYReadStarter,updateAmplDisplay) );
    histgrp->setStretch( 2, 1 );
    histgrp->attach( stretchedBelow, infofld_ );

    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filenamepopuptimer_;
    delete filereadopts_;
    delete &clipsampler_;
}


FullSpec uiSEGYReadStarter::fullSpec() const
{
    const SEGY::ImpType& imptyp = impType();
    FullSpec ret( imptyp.geomType(), imptyp.isVSP() );
    ret.spec_ = filespec_;
    ret.pars_ = filepars_;
    if ( filereadopts_ )
	ret.readopts_ = *filereadopts_;
    return ret;
}


void uiSEGYReadStarter::clearDisplay()
{
    infofld_->clearInfo();
    ampldisp_->setEmpty();
    setExamineStatus();
}


void uiSEGYReadStarter::setImpTypIdx( int tidx )
{
    typfld_->setTypIdx( tidx ); // should trigger its callback
}


const SEGY::ImpType& uiSEGYReadStarter::impType() const
{
    return typfld_->impType();
}


void uiSEGYReadStarter::execNewScan( bool fixedloaddef )
{
    userfilename_ = inpfld_->fileName();
    deepErase( scaninfo_ );
    clipsampler_.reset();
    clearDisplay();
    if ( !getFileSpec() )
	return;

    MouseCursorChanger chgr( MouseCursor::Wait );
    if ( !scanFile(filespec_.fileName(0),fixedloaddef) )
	return;

    const int nrfiles = filespec_.nrFiles();
    for ( int idx=1; idx<nrfiles; idx++ )
	scanFile( filespec_.fileName(idx), true );

    displayScanResults();
}


void uiSEGYReadStarter::setExamineStatus()
{
    int nrfiles = scaninfo_.size();
    examinebut_->setSensitive( nrfiles > 0 );
    examinebut_->setToolTip( nrfiles > 1 ? tr("Examine first input file")
					 : tr("Examine input file") );
}


void uiSEGYReadStarter::initWin( CallBacker* )
{
    typChg( 0 );
    inpChg( 0 );

    if ( filespec_.isEmpty() )
    {
	filenamepopuptimer_ = new Timer( "File selector popup timer" );
	filenamepopuptimer_->start( 1, true );
	filenamepopuptimer_->tick.notify( mCB(inpfld_,uiFileInput,selectFile) );
    }
}


void uiSEGYReadStarter::typChg( CallBacker* )
{
    infofld_->setImpTypIdx( impType().tidx_ );
}


void uiSEGYReadStarter::inpChg( CallBacker* cb )
{
    const BufferString newusrfnm = inpfld_->fileName();
    if ( newusrfnm.isEmpty() )
    {
	if ( cb )
	    clearDisplay();
	return;
    }

    if ( newusrfnm != userfilename_ )
    {
	userfilename_ = inpfld_->fileName();
	execNewScan( false );
    }
}


void uiSEGYReadStarter::examineCB( CallBacker* )
{
    if ( !commit() )
	return;

    MouseCursorChanger chgr( MouseCursor::Wait );
    uiSEGYExamine::Setup su( examinenrtrcsfld_->getIntValue() );
    su.fs_ = filespec_; su.fp_ = filepars_;
    uiSEGYExamine* dlg = new uiSEGYExamine( this, su );
    dlg->setDeleteOnClose( true );
    dlg->go();
}


void uiSEGYReadStarter::updateAmplDisplay( CallBacker* )
{
    int nrvals = (int)clipsampler_.nrVals();
    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    const float* samps = clipsampler_.vals();
    ampldisp_->setData( samps, nrvals );

    float clipval = clipfld_->getFValue();
    const bool useclip = !mIsUdf(clipval) && clipval > 0.05;
    if ( useclip )
    {
	clipval *= 0.01f;
	DataClipper clipper;
	clipper.putData( samps, nrvals );
	Interval<float> rg;
	clipper.calculateRange( clipval, rg );
	ampldisp_->setDrawRange( rg );
    }

    ampldisp_->useDrawRange( useclip );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSEGYReadStarter::getFileSpec()
{
    if ( userfilename_.isEmpty() )
	return false;

    filespec_.setEmpty();
    if ( !userfilename_.find('*') )
    {
	if ( !getExistingFileName(userfilename_) )
	    return false;
	filespec_.setFileName( userfilename_ );
    }
    else
    {
	FilePath fp( userfilename_ );
	if ( !fp.isAbsolute() )
	    mErrRet(
	    tr("Please specify the absolute file name when using a wildcard.") )

	DirList dl( fp.pathOnly(), DirList::FilesOnly, fp.fileName() );
	for ( int idx=0; idx<dl.size(); idx++ )
	    filespec_.fnames_.add( dl.fullPath(idx) );

	if ( filespec_.isEmpty() )
	    mErrRet( tr("No file names matching your wildcard(s).") )
    }

    return true;
}


bool uiSEGYReadStarter::getExistingFileName( BufferString& fnm, bool emiterr )
{
    FilePath fp( fnm );
    if ( fp.isAbsolute() )
    {
	if ( !File::exists(fnm) )
	{
	    if ( emiterr )
		uiMSG().error( uiString(
			    "SEG-Y file does not exist:\n%1").arg(fnm) );
	    return false;
	}
    }
    else
    {
	FilePath newfp( GetDataDir(), fnm );
	if ( !File::exists(newfp.fullPath()) )
	{
	    newfp.set( GetDataDir() ).add( "Seismics" );
	    if ( !File::exists(newfp.fullPath()) )
	    {
		if ( emiterr )
		    uiMSG().error(
			    tr("SEG-Y file not found in survey directory") );
		return false;
	    }
	}
	fnm = newfp.fullPath();
    }

    return true;
}


#define mErrRetFileName(s) \
{ \
    if ( isfirst ) \
	uiMSG().error( uiString(s).arg(strm.fileName()) ); \
    return false; \
}

bool uiSEGYReadStarter::scanFile( const char* fnm, bool fixedloaddef )
{
    const bool isfirst = scaninfo_.isEmpty();
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRetFileName( "Cannot open file: %1" )

    SEGY::TxtHeader txthdr; SEGY::BinHeader binhdr;
    strm.getBin( txthdr.txt_, SegyTxtHeaderLength );
    if ( !strm.isOK() )
	mErrRetFileName( "File:\n%1\nhas no textual header" )
    strm.getBin( binhdr.buf(), SegyBinHeaderLength );
    if ( strm.isBad() )
	mErrRetFileName( "File:\n%1\nhas no binary header" )

    bool infeet = false;
    if ( isfirst )
    {
	if ( !fixedloaddef )
	{
	    binhdr.guessIsSwapped();
	    loaddef_.hdrsswapped_ = loaddef_.dataswapped_ = binhdr.isSwapped();
	}
	if ( loaddef_.hdrsswapped_ )
	    binhdr.unSwap();
	if ( !binhdr.isRev0() )
	    binhdr.skipRev1Stanzas( strm );
	infeet = binhdr.isInFeet();

	if ( !fixedloaddef )
	{
	    loaddef_.ns_ = binhdr.nrSamples();
	    if ( loaddef_.ns_ < 1 || loaddef_.ns_ > mMaxReasonableNS )
		loaddef_.ns_ = -1;
	    loaddef_.revision_ = binhdr.revision();
	    short fmt = binhdr.format();
	    if ( fmt != 1 && fmt != 2 && fmt != 3 && fmt != 5 && fmt != 8 )
		fmt = 1;
	    loaddef_.format_ = fmt;
	}
    }

    SEGY::ScanInfo* si = new SEGY::ScanInfo( fnm );
    if ( !obtainScanInfo(*si,strm,isfirst) )
	{ delete si; return false; }

    si->infeet_ = infeet;
    scaninfo_ += si;
    return true;
}


bool uiSEGYReadStarter::obtainScanInfo( SEGY::ScanInfo& si, od_istream& strm,
					bool isfirst )
{
    if ( isfirst )
    {
	if ( !completeLoadDef(strm) )
	    return false;
    }

    si.getFromSEGYBody( strm, loaddef_, isfirst,
			Seis::is2D(typfld_->impType().geomType()),
			clipsampler_ );
    return true;
}


bool uiSEGYReadStarter::completeLoadDef( od_istream& strm )
{
    const bool isfirst = true; // for mErrRetFileName
    const od_stream::Pos firsttrcpos = strm.position();

    PtrMan<SEGY::TrcHeader> thdr = loaddef_.getTrcHdr( strm );
    if ( !thdr )
	mErrRetFileName( "File:\n%1\nNo traces found" )

    if ( loaddef_.ns_ < 1 )
    {
	loaddef_.ns_ = (int)thdr->nrSamples();
	if ( loaddef_.ns_ > mMaxReasonableNS )
	    mErrRetFileName(
		    "File:\n%1\nNo proper 'number of samples per trace' found" )
    }

    SeisTrcInfo ti; thdr->fill( ti, loaddef_.coordscale_ );
    if ( mIsUdf(loaddef_.sampling_.step) )
	loaddef_.sampling_ = ti.sampling;

    if ( veryfirstscan_ )
    {
	//TODO do magic things to find byte positions
	veryfirstscan_ = false;
    }

    strm.setPosition( firsttrcpos );
    infofld_->useLoadDef();
    return true;
}


void uiSEGYReadStarter::displayScanResults()
{
    if ( scaninfo_.isEmpty() )
	{ clearDisplay(); return; }

    setExamineStatus();
    updateAmplDisplay( 0 );

    SEGY::ScanInfo si( *scaninfo_[0] );
    si.filenm_ = userfilename_;
    for ( int idx=1; idx<scaninfo_.size(); idx++ )
	si.merge( *scaninfo_[idx] );

    infofld_->setScanInfo( si );
}


bool uiSEGYReadStarter::commit()
{
    if ( filespec_.isEmpty() )
	return false;

    // fill filepars_, filereadopts_
    pErrMsg( "TODO: finish commit" );
    return true;
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    if ( !commit() )
	return false;

    const FullSpec fullspec = fullSpec();
    uiSEGYReadFinisher dlg( this, fullspec, userfilename_ );
    dlg.go();

    return false;
}
