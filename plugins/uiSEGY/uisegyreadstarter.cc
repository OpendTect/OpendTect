/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadstarter.h"

#include "uisegyreadstartinfo.h"
#include "uisegyimptype.h"
#include "uisegydef.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uihistogramdisplay.h"
#include "uimsg.h"
#include "segyhdr.h"
#include "seisinfo.h"
#include "dataclipper.h"
#include "filepath.h"
#include "dirlist.h"
#include "oddirs.h"
#include "od_istream.h"


uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, const FileSpec* fs )
    : uiDialog(p,uiDialog::Setup(tr("Import SEG-Y Data"),mNoDlgTitle,
				  mTODOHelpKey ) )
    , filespec_(fs?*fs:FileSpec())
    , filereadopts_(0)
    , veryfirstscan_(false)
    , userfilename_("x") // any non-empty
    , clipsampler_(*new DataClipSampler(100000))
{
    typfld_ = new uiSEGYImpType( this );
    typfld_->typeChanged.notify( mCB(this,uiSEGYReadStarter,typChg) );

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true );
    inpfld_ = new uiFileInput( this, "Input file(s) (*=wildcard)",
				fisu );
    inpfld_->attach( alignedBelow, typfld_ );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, inpfld_ );

    infofld_ = new uiSEGYReadStartInfo( this, scandef_ );
    infofld_->attach( ensureBelow, sep );
    infofld_->scandefChanged.notify( mCB(this,uiSEGYReadStarter,defChg) );

    uiHistogramDisplay::Setup hdsu;
    hdsu.noyaxis( false ).noygridline(true).annoty( false );
    ampldisp_ = new uiHistogramDisplay( this, hdsu );
    ampldisp_->setTitle( tr("Amplitudes") );
    ampldisp_->setStretch( 2, 1 );
    ampldisp_->setPrefHeight( 250 );
    ampldisp_->attach( stretchedBelow, infofld_ );

    filereadopts_ = new FileReadOpts( typfld_->impType().geomType() );
    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filereadopts_;
    delete &clipsampler_;
}


void uiSEGYReadStarter::clearDisplay()
{
    infofld_->clearInfo();
    ampldisp_->setEmpty();
}


void uiSEGYReadStarter::setImpTypIdx( int tidx )
{
    typfld_->setTypIdx( tidx ); // should trigger its callback
}


void uiSEGYReadStarter::execNewScan( bool fixedscandef )
{
    userfilename_ = inpfld_->fileName();
    deepErase( scandata_ );
    clipsampler_.reset();
    if ( !getFileSpec() )
	{ clearDisplay(); return; }

    MouseCursorChanger chgr( MouseCursor::Wait );
    if ( !scanFile(filespec_.fileName(0),fixedscandef) )
	return;

    const int nrfiles = filespec_.nrFiles();
    for ( int idx=1; idx<nrfiles; idx++ )
	scanFile( filespec_.fileName(idx), true );

    displayScanResults();
}


void uiSEGYReadStarter::initWin( CallBacker* )
{
    typChg( 0 );
    inpChg( 0 );
}


void uiSEGYReadStarter::typChg( CallBacker* )
{
    infofld_->setImpTypIdx( typfld_->impType().tidx_ );
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

bool uiSEGYReadStarter::scanFile( const char* fnm, bool fixedscandef )
{
    const bool isfirst = scandata_.isEmpty();
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
	if ( !fixedscandef )
	{
	    binhdr.guessIsSwapped();
	    scandef_.hdrsswapped_ = scandef_.dataswapped_ = binhdr.isSwapped();
	}
	if ( scandef_.hdrsswapped_ )
	    binhdr.unSwap();
	if ( binhdr.isRev1() )
	    binhdr.skipRev1Stanzas( strm );
	infeet = binhdr.isInFeet();

	if ( !fixedscandef )
	{
	    scandef_.ns_ = binhdr.nrSamples();
	    if ( scandef_.ns_ < 1 || scandef_.ns_ > mMaxReasonableNS )
		scandef_.ns_ = -1;
	    scandef_.revision_ = binhdr.isRev1() ? 1 : 0;
	    short fmt = binhdr.format();
	    if ( fmt != 1 && fmt != 2 && fmt != 3 && fmt != 5 && fmt != 8 )
		fmt = 1;
	    scandef_.format_ = fmt;
	}
    }

    SEGY::uiScanData* sd = new SEGY::uiScanData( fnm );
    if ( !obtainScanData(*sd,strm,isfirst) )
	{ delete sd; return false; }

    sd->infeet_ = infeet;
    scandata_ += sd;
    return true;
}


bool uiSEGYReadStarter::obtainScanData( SEGY::uiScanData& sd, od_istream& strm,
					bool isfirst )
{
    if ( isfirst )
    {
	if ( !completeScanDef(strm) )
	    return false;
    }

    sd.getFromSEGYBody( strm, scandef_, isfirst,
			Seis::is2D(typfld_->impType().geomType()),
			clipsampler_ );
    return true;
}


bool uiSEGYReadStarter::completeScanDef( od_istream& strm )
{
    const bool isfirst = true; // for mErrRetFileName
    const od_stream::Pos firsttrcpos = strm.position();

    PtrMan<SEGY::TrcHeader> thdr = scandef_.getTrcHdr( strm );
    if ( !thdr )
	mErrRetFileName( "File:\n%1\nNo traces found" )

    if ( scandef_.ns_ < 1 )
    {
	scandef_.ns_ = (int)thdr->nrSamples();
	if ( scandef_.ns_ > mMaxReasonableNS )
	    mErrRetFileName(
		    "File:\n%1\nNo proper 'number of samples per trace' found" )
    }

    SeisTrcInfo ti; thdr->fill( ti, scandef_.coordscale_ );
    scandef_.sampling_ = ti.sampling;

    if ( veryfirstscan_ )
    {
	//TODO do magic things to find byte positions
	veryfirstscan_ = false;
    }

    strm.setPosition( firsttrcpos );
    infofld_->useScanDef();
    return true;
}


void uiSEGYReadStarter::displayScanResults()
{
    if ( scandata_.isEmpty() )
	{ clearDisplay(); return; }

    int nrvals = (int)clipsampler_.nrVals();
    if ( nrvals < 1 )
	ampldisp_->setEmpty();
    else
	ampldisp_->setData( clipsampler_.vals(), nrvals );

    SEGY::uiScanData sd( *scandata_[0] );
    sd.filenm_ = userfilename_;
    for ( int idx=1; idx<scandata_.size(); idx++ )
	sd.merge( *scandata_[idx] );

    infofld_->setScanData( sd );
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    // transfer scandef to filepars_ and filereadopts_
    return false;
}
