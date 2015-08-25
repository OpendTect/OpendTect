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
#include "uilabel.h"
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


uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, const SEGY::ImpType* imptyp )
    : uiDialog(p,uiDialog::Setup(tr("Import SEG-Y Data"),
			imptyp ? uiString("Import %1").arg(imptyp->dispText())
				: mNoDlgTitle,
				  mTODOHelpKey ) )
    , filereadopts_(0)
    , typfld_(0)
    , veryfirstscan_(false)
    , userfilename_("x") // any non-empty
    , clipsampler_(*new DataClipSampler(100000))
    , filenamepopuptimer_(0)
{
    setCtrlStyle( RunAndClose );
    setOkText( tr("Next >>") );

    uiFileInput::Setup fisu( uiFileDialog::Gen, filespec_.fileName() );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true )
	.objtype( tr("SEG-Y") );
    inpfld_ = new uiFileInput( this, "Input file(s) (*=wildcard)",
				fisu );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );

    if ( imptyp )
	fixedimptype_ = *imptyp;
    else
    {
	typfld_ = new uiSEGYImpType( this );
	typfld_->typeChanged.notify( mCB(this,uiSEGYReadStarter,typChg) );
	typfld_->attach( alignedBelow, inpfld_ );
    }
    nrfileslbl_ = new uiLabel( this, uiString::emptyString() );
    nrfileslbl_->setPrefWidthInChar( 10 );
    nrfileslbl_->setAlignment( Alignment::Right );
    if ( !typfld_ )
	nrfileslbl_->attach( rightTo, inpfld_ );
    else
    {
	nrfileslbl_->attach( rightTo, typfld_ );
	nrfileslbl_->attach( rightBorder );
    }

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, nrfileslbl_ );

    infofld_ = new uiSEGYReadStartInfo( this, loaddef_, imptyp );
    infofld_->attach( ensureBelow, sep );
    infofld_->loaddefChanged.notify( mCB(this,uiSEGYReadStarter,defChg) );


    fullscanbut_ = new uiToolButton( this, "fullscan",
				    tr("Scan the entire input"),
				    mCB(this,uiSEGYReadStarter,fullScanReq) );
    fullscanbut_->attach( rightOf, infofld_ );

    uiGroup* examinegrp = new uiGroup( this, "Examine group" );
    examinebut_ = new uiToolButton( examinegrp, "examine",
				    uiString::emptyString(),
				    mCB(this,uiSEGYReadStarter,examineCB) );
    examinenrtrcsfld_ = new uiSpinBox( examinegrp, 0, "Examine traces" );
    examinenrtrcsfld_->setInterval( 0, 1000000, 10 );
    examinenrtrcsfld_->setHSzPol( uiObject::Small );
    examinenrtrcsfld_->setToolTip( tr("Number of traces to examine") );
    examinenrtrcsfld_->attach( alignedBelow, examinebut_ );
    int nrex = 1000; Settings::common().get( sKeySettNrTrcExamine, nrex );
    examinenrtrcsfld_->setInterval( 10, 1000000, 10 );
    examinenrtrcsfld_->setValue( nrex );
    examinegrp->attach( alignedBelow, fullscanbut_ );

    setToolStatuses();

    uiGroup* histgrp = new uiGroup( this, "Histogram group" );
    const CallBack histupdcb( mCB(this,uiSEGYReadStarter,updateAmplDisplay) );
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
    clipfld_->valueChanging.notify( histupdcb );
    inc0sbox_ = new uiCheckBox( histgrp, "Zeros" );
    inc0sbox_->attach( alignedBelow, clipfld_ );
    inc0sbox_->setHSzPol( uiObject::Small );
    inc0sbox_->setToolTip( tr("Include value '0' for histogram display") );
    inc0sbox_->activated.notify( histupdcb );
    histgrp->setStretch( 2, 1 );
    histgrp->attach( stretchedBelow, infofld_ );

    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filenamepopuptimer_;
    delete filereadopts_;
    deepErase( scaninfo_ );
    delete &clipsampler_;
}


FullSpec uiSEGYReadStarter::fullSpec() const
{
    const SEGY::ImpType& imptyp = impType();
    FullSpec ret( imptyp.geomType(), imptyp.isVSP() );
    ret.rev0_ = loaddef_.revision_ == 0;
    ret.spec_ = filespec_;
    ret.pars_ = filepars_;
    ret.zinfeet_ = infeet_;
    if ( filereadopts_ )
	ret.readopts_ = *filereadopts_;
    return ret;
}


void uiSEGYReadStarter::clearDisplay()
{
    infofld_->clearInfo();
    ampldisp_->setEmpty();
    setToolStatuses();
}


void uiSEGYReadStarter::setImpTypIdx( int tidx )
{
    if ( !typfld_ )
    {
	pErrMsg( "Cannot set type if fixed" );
	return;
    }

    typfld_->setTypIdx( tidx ); // should trigger its callback
}


const SEGY::ImpType& uiSEGYReadStarter::impType() const
{
    return typfld_ ? typfld_->impType() : fixedimptype_;
}


void uiSEGYReadStarter::execNewScan( bool fixedloaddef, bool full )
{
    deepErase( scaninfo_ );
    clipsampler_.reset();
    clearDisplay();
    if ( !getFileSpec() )
	return;

    MouseCursorChanger chgr( MouseCursor::Wait );
    if ( !scanFile(filespec_.fileName(0),fixedloaddef,full) )
	return;

    const int nrfiles = filespec_.nrFiles();
    for ( int idx=1; idx<nrfiles; idx++ )
	scanFile( filespec_.fileName(idx), true, full );

    displayScanResults();
}


void uiSEGYReadStarter::setToolStatuses()
{
    int nrfiles = scaninfo_.size();
    examinebut_->setSensitive( nrfiles > 0 );
    fullscanbut_->setSensitive( nrfiles > 0 );
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
    handleNewInputSpec( false );
}


void uiSEGYReadStarter::fullScanReq( CallBacker* cb )
{
    handleNewInputSpec( true );
}



void uiSEGYReadStarter::handleNewInputSpec( bool fullscan )
{
    const BufferString newusrfnm = inpfld_->fileName();
    if ( newusrfnm.isEmpty() )
	{ clearDisplay(); return; }

    if ( fullscan || newusrfnm != userfilename_ )
    {
	userfilename_ = newusrfnm;
	execNewScan( false, fullscan );
    }

    const int nrfiles = scaninfo_.size();
    uiString txt;
    if ( nrfiles > 1 )
	{ txt = tr( "[%1 files]" ); txt.arg( nrfiles ); }
    nrfileslbl_->setText( txt );
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

    const float* csvals = clipsampler_.vals();
    float clipval = clipfld_->getFValue();
    const bool useclip = !mIsUdf(clipval) && clipval > 0.05;
    const bool rm0 = !inc0sbox_->isChecked();
    if ( !useclip && !rm0 )
	{ ampldisp_->setData( csvals, nrvals ); return; }

    TypeSet<float> vals;
    if ( !rm0 )
	vals.append( csvals, nrvals );
    else
    {
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float val = csvals[idx];
	    if ( val != 0.f )
		vals += val;
	}
	nrvals = vals.size();
    }

    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    if ( useclip )
    {
	clipval *= 0.01f;
	DataClipper clipper;
	clipper.putData( vals.arr(), nrvals );
	Interval<float> rg;
	clipper.calculateRange( clipval, rg );
	TypeSet<float> oldvals( vals );
	vals.setEmpty();
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float val = oldvals[idx];
	    if ( rg.includes(val,false) )
		vals += val;
	}

	nrvals = vals.size();
	if ( nrvals < 1 )
	    { ampldisp_->setEmpty(); return; }
    }

    ampldisp_->setData( vals.arr(), nrvals );
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

bool uiSEGYReadStarter::scanFile( const char* fnm, bool fixedloaddef,
				  bool full )
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

    SEGY::ScanInfo* si = new SEGY::ScanInfo( fnm );
    SEGY::BasicFileInfo& bfi = si->basicinfo_;
    bool infeet = false;

    if ( !fixedloaddef )
	binhdr.guessIsSwapped();
    bfi.hdrsswapped_ = bfi.dataswapped_ = binhdr.isSwapped();
    if ( (fixedloaddef && loaddef_.hdrsswapped_)
	|| (!fixedloaddef && bfi.hdrsswapped_) )
	binhdr.unSwap();
    if ( !binhdr.isRev0() )
	binhdr.skipRev1Stanzas( strm );
    infeet = binhdr.isInFeet();

    bfi.ns_ = binhdr.nrSamples();
    if ( bfi.ns_ < 1 || bfi.ns_ > mMaxReasonableNS )
	bfi.ns_ = -1;
    bfi.revision_ = binhdr.revision();
    short fmt = binhdr.format();
    if ( fmt != 1 && fmt != 2 && fmt != 3 && fmt != 5 && fmt != 8 )
	fmt = 1;
    bfi.format_ = fmt;
    if ( !completeFileInfo(strm,bfi,isfirst) )
	return false;

    if ( isfirst && !fixedloaddef )
    {
	static_cast<SEGY::BasicFileInfo&>(loaddef_) = bfi;
	completeLoadDef( strm );
    }

    if ( !obtainScanInfo(*si,strm,isfirst,full) )
	{ delete si; return false; }

    si->infeet_ = infeet;
    si->fullscan_ = full;
    scaninfo_ += si;
    return true;
}


bool uiSEGYReadStarter::obtainScanInfo( SEGY::ScanInfo& si, od_istream& strm,
					bool isfirst, bool full )
{
    if ( !completeFileInfo(strm,si.basicinfo_,isfirst) )
	return false;

    si.getFromSEGYBody( strm, loaddef_, isfirst,
			Seis::is2D(impType().geomType()),
			clipsampler_, full, this );
    return true;
}


#define mErrRetResetStream(str) { \
    strm.setPosition( firsttrcpos ); \
    if ( emiterr ) \
	mErrRetFileName( str ) \
    return false; }

bool uiSEGYReadStarter::completeFileInfo( od_istream& strm,
				      SEGY::BasicFileInfo& bfi, bool emiterr )
{
    const bool isfirst = true; // for mErrRetFileName
    const od_stream::Pos firsttrcpos = strm.position();

    SEGY::LoadDef ld;
    PtrMan<SEGY::TrcHeader> thdr = loaddef_.getTrcHdr( strm );
    if ( !thdr )
	mErrRetResetStream( "File:\n%1\nNo traces found" )

    if ( bfi.ns_ < 1 )
    {
	bfi.ns_ = (int)thdr->nrSamples();
	if ( bfi.ns_ > mMaxReasonableNS )
	    mErrRetResetStream(
		    "File:\n%1\nNo proper 'number of samples per trace' found" )
    }

    if ( mIsUdf(bfi.sampling_.step) )
    {
	SeisTrcInfo ti; thdr->fill( ti, 1.0f );
	bfi.sampling_ = ti.sampling;
    }

    strm.setPosition( firsttrcpos );
    return true;
}


void uiSEGYReadStarter::completeLoadDef( od_istream& strm )
{
    if ( !veryfirstscan_ )
	return;

    veryfirstscan_ = false;
    const od_stream::Pos firsttrcpos = strm.position();

    //TODO do magic things to find byte positions

    strm.setPosition( firsttrcpos );
    infofld_->useLoadDef();
}


void uiSEGYReadStarter::displayScanResults()
{
    if ( scaninfo_.isEmpty() )
	{ clearDisplay(); return; }

    setToolStatuses();
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

    filepars_.ns_ = loaddef_.ns_;
    filepars_.fmt_ = loaddef_.format_;
    filepars_.setSwap( loaddef_.hdrsswapped_, loaddef_.dataswapped_ );

    filereadopts_ = new FileReadOpts( impType().geomType() );
    filereadopts_->thdef_ = *loaddef_.hdrdef_;
    filereadopts_->coordscale_ = loaddef_.coordscale_;
    filereadopts_->timeshift_ = loaddef_.sampling_.start;
    filereadopts_->sampleintv_ = loaddef_.sampling_.step;

    //TODO
    // filereadopts_->icdef_ ?
    // filereadopts_->psdef_ ?
    // filereadopts_->coorddef_ in next window

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
