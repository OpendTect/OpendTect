/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadstarter.h"

#include "uisegydef.h"
#include "uisegyimptype.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uitable.h"
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

static const int cMaxReasonableNS = 25000;
// Time: 50 (2ms) or 100 seconds (4ms); Depth: 25 km

#define mNrInfoRows 9
#define mRevRow 0
#define mDataFormatRow 1
#define mNrSamplesRow 2
#define mZRangeRow 3
#define mKey1Row 4
#define mKey2Row 5
#define mXRow 6
#define mYRow 7
#define mPSRow 8


uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, const FileSpec* fs )
    : uiDialog(p,uiDialog::Setup(tr("Import SEG-Y Data"),mNoDlgTitle,
				  mTODOHelpKey ) )
    , filespec_(fs?*fs:FileSpec())
    , filereadopts_(0)
    , scandefguessed_(false)
    , userfilename_("x") // any non-empty
    , clipsampler_(*new DataClipSampler(100000))
    , infeet_(false)
    , parsbeingset_(false)
{
    const CallBack inpchgcb( mCB(this,uiSEGYReadStarter,inpChg) );
    typfld_ = new uiSEGYImpType( this );
    typfld_->typeChanged.notify( inpchgcb );

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true );
    inpfld_ = new uiFileInput( this, "Input file(s) (*=wildcard)",
				fisu );
    inpfld_->attach( alignedBelow, typfld_ );
    inpfld_->valuechanged.notify( inpchgcb );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, inpfld_ );

    buildTable();
    setTableFromScanDef();
    infotbl_->attach( ensureBelow, sep );

    uiHistogramDisplay::Setup hdsu;
    hdsu.noyaxis( false ).noygridline(true).annoty( false );
    ampldisp_ = new uiHistogramDisplay( this, hdsu );
    ampldisp_->setTitle( tr("Amplitudes") );
    ampldisp_->setStretch( 2, 1 );
    ampldisp_->setPrefHeight( 250 );
    ampldisp_->attach( stretchedBelow, infotbl_ );

    filereadopts_ = new FileReadOpts( typfld_->impType().geomType() );
    postFinalise().notify( inpchgcb );
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filereadopts_;
    delete &clipsampler_;
}


void uiSEGYReadStarter::buildTable()
{
    infotbl_ = new uiTable( this, uiTable::Setup(mNrInfoRows,3)
				  .manualresize(true), "Info table" );
    infotbl_->setColumnLabel( 0, "" );
    infotbl_->setColumnLabel( 1, "Quick scan result" );
    infotbl_->setColumnLabel( 2, "Use" );
    infotbl_->setColumnStretchable( 0, false );
    infotbl_->setColumnStretchable( 1, true );
    infotbl_->setPrefWidthInChar( 80 );
    infotbl_->setPrefHeightInRows( mNrInfoRows );
    infotbl_->setTableReadOnly( true );
    infotbl_->setLeftHeaderHidden( true );
    setCellTxt( 0, mRevRow, "SEG-Y Revision" );
    setCellTxt( 0, mDataFormatRow, "Data format" );
    setCellTxt( 0, mNrSamplesRow, "Number of samples" );
    setCellTxt( 0, mZRangeRow, "Z Range" );

    const CallBack cb( mCB(this,uiSEGYReadStarter,parChg) );

    uiGroup* grp = new uiGroup( 0, "rev group" );
    const char* revstrs[] = { "0", "1", 0 };
    revfld_ = new uiComboBox( grp, revstrs, "Revision" );
    revfld_->selectionChanged.notify( cb );
    revfld_->setStretch( 2, 1 );
    infotbl_->setCellGroup( RowCol(mRevRow,2), grp );
}


void uiSEGYReadStarter::setCellTxt( int col, int row, const char* txt )
{
    infotbl_->setText( RowCol(row,col), txt );
    if ( col==0 )
	infotbl_->resizeColumnToContents( col );
}


void uiSEGYReadStarter::setImpTypIdx( int tidx )
{
    typfld_->setTypIdx( tidx );
}


void uiSEGYReadStarter::parChg( CallBacker* )
{
    if ( !parsbeingset_ )
    {
	setScanDefFromTable();
	handleChange();
    }
}


void uiSEGYReadStarter::inpChg( CallBacker* cb )
{
    const BufferString newusrfnm = inpfld_->fileName();
    const bool isnewfile = newusrfnm != userfilename_;
    const bool isnewtype = cb == typfld_;
    if ( !isnewfile && !isnewtype )
	return;

    handleChange();
}


void uiSEGYReadStarter::handleChange()
{
    userfilename_ = inpfld_->fileName();
    const SEGY::ImpType& imptyp = typfld_->impType();
    const Seis::GeomType gt = imptyp.geomType();
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    const char* k1str; const char* k2str; const char* psstr;
    const char* xstr; const char* ystr;
    k1str = k2str = psstr = xstr = ystr = "";

    if ( !imptyp.isVSP() )
    {
	xstr = "X-Coordinate range";
	ystr = "Y-Coordinate range";
	if ( is2d )
	{
	    k1str = "Trace number range";
	    k2str = "Ref/SP number range";
	}
	else
	{
	    k1str = "Inline range";
	    k2str = "Crossline range";
	}
	if ( isps )
	    psstr = "Offset range";
    }
    setCellTxt( 0, mKey1Row, k1str );
    setCellTxt( 0, mKey2Row, k2str );
    setCellTxt( 0, mXRow, xstr );
    setCellTxt( 0, mYRow, ystr );
    setCellTxt( 0, mPSRow, psstr );

    if ( getFileSpec() )
	scanInput();
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSEGYReadStarter::getFileSpec()
{
    for ( int idx=0; idx<mNrInfoRows; idx++ )
	setCellTxt( 1, idx, "" );
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


void uiSEGYReadStarter::scanInput()
{
    deepErase( scandata_ );
    clipsampler_.reset();
    scandef_.reInit();

    MouseCursorChanger chgr( MouseCursor::Wait );
    if ( !scanFile(filespec_.fileName(0)) )
	return;

    const int nrfiles = filespec_.nrFiles();
    for ( int idx=1; idx<nrfiles; idx++ )
	scanFile( filespec_.fileName(idx) );

    displayScanResults();
}


#define mErrRetFileName(s) \
{ \
    if ( isfirst ) \
	uiMSG().error( uiString(s).arg(strm.fileName()) ); \
    return false; \
}

bool uiSEGYReadStarter::scanFile( const char* fnm )
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

    if ( isfirst )
    {
	binhdr.guessIsSwapped();
	scandef_.hdrsswapped_ = scandef_.dataswapped_ = binhdr.isSwapped();
	if ( binhdr.isSwapped() )
	    binhdr.unSwap();
	if ( binhdr.isRev1() )
	    binhdr.skipRev1Stanzas( strm );

	if ( scandef_.ns_ < 1 )
	{
	    scandef_.ns_ = binhdr.nrSamples();
	    if ( scandef_.ns_ < 1 || scandef_.ns_ > cMaxReasonableNS )
		scandef_.ns_ = -1;
	}
	scandef_.revision_ = binhdr.isRev1() ? 1 : 0;
	short fmt = binhdr.format();
	if ( fmt != 1 && fmt != 2 && fmt != 3 && fmt != 5 && fmt != 8 )
	    fmt = 1;
	scandef_.format_ = fmt;

	infeet_ = binhdr.isInFeet();
    }

    SEGY::uiScanData* sd = new SEGY::uiScanData( fnm );
    if ( !obtainScanData(*sd,strm,isfirst) )
	{ delete sd; return false; }

    scandata_ += sd;
    return true;
}


bool uiSEGYReadStarter::obtainScanData( SEGY::uiScanData& sd, od_istream& strm,
					bool isfirst )
{
    if ( isfirst && !scandefguessed_ )
    {
	if ( !guessScanDef(strm) )
	    return false;
    }

    sd.getFromSEGYBody( strm, scandef_, isfirst,
			Seis::is2D(typfld_->impType().geomType()),
			clipsampler_ );
    return true;
}


bool uiSEGYReadStarter::guessScanDef( od_istream& strm )
{
    const bool isfirst = true; // for mErrRetFileName
    const od_stream::Pos firsttrcpos = strm.position();

    PtrMan<SEGY::TrcHeader> thdr = scandef_.getTrcHdr( strm );
    if ( !thdr )
	mErrRetFileName( "File:\n%1\nNo traces found" )

    if ( scandef_.ns_ < 1 )
    {
	scandef_.ns_ = (int)thdr->nrSamples();
	if ( scandef_.ns_ > cMaxReasonableNS )
	    mErrRetFileName(
		    "File:\n%1\nNo proper 'number of samples per trace' found" )
    }

    SeisTrcInfo ti; thdr->fill( ti, scandef_.coordscale_ );
    scandef_.sampling_ = ti.sampling;

    //TODO do magic things to find byte positions

    strm.setPosition( firsttrcpos );
    scandefguessed_ = true;
    setTableFromScanDef();
    return true;
}


void uiSEGYReadStarter::displayScanResults()
{
    int nrvals = (int)clipsampler_.nrVals();
    if ( nrvals < 1 )
	ampldisp_->setEmpty();
    else
	ampldisp_->setData( clipsampler_.vals(), nrvals );

    if ( scandata_.isEmpty() )
	return;

    SEGY::uiScanData sd( *scandata_[0] );
    sd.filenm_ = userfilename_;
    for ( int idx=1; idx<scandata_.size(); idx++ )
	sd.merge( *scandata_[idx] );

    BufferString txt;

    txt.set( scandef_.revision_ );
    setCellTxt( 1, mRevRow, txt );

    const char** fmts = SEGY::FilePars::getFmts(false);
    txt.set( scandef_.format_ < 4 ? fmts[scandef_.format_-1]
	    : (scandef_.format_==8 ? fmts[4] : fmts[3]) );
    if ( scandef_.hdrsswapped_ && scandef_.dataswapped_ )
	txt.add( " (all bytes swapped)" );
    else if ( scandef_.hdrsswapped_ )
	txt.add( " (header bytes swapped)" );
    else if ( scandef_.dataswapped_ )
	txt.add( " (data bytes swapped)" );
    setCellTxt( 1, mDataFormatRow, txt );

    txt.set( scandef_.ns_ ).add( " (" ).add( sd.nrtrcs_ )
	.add( sd.nrtrcs_ == 1 ? " trace)" : " traces)" );
    setCellTxt( 1, mNrSamplesRow, txt );

    const float endz = scandef_.sampling_.start
		     + (scandef_.ns_-1) * scandef_.sampling_.step;
    txt.set( scandef_.sampling_.start ).add( " - " ).add( endz )
	.add( " step " ).add( scandef_.sampling_.step )
	.add( " (s or " ).add( infeet_ ? "ft)" : "m)" );
    setCellTxt( 1, mZRangeRow, txt );

    const SEGY::ImpType& imptyp = typfld_->impType();
    if ( imptyp.isVSP() )
	return;

    const Seis::GeomType gt = imptyp.geomType();
    if ( Seis::is2D(gt) )
	txt.set( sd.trcnrs_.start ).add( " - " ).add( sd.trcnrs_.stop );
    else
	txt.set( sd.inls_.start ).add( " - " ).add( sd.inls_.stop );
    setCellTxt( 1, mKey1Row, txt );

    if ( Seis::is2D(gt) )
	txt.set( sd.refnrs_.start ).add( " - " ).add( sd.refnrs_.stop );
    else
	txt.set( sd.crls_.start ).add( " - " ).add( sd.crls_.stop );
    setCellTxt( 1, mKey2Row, txt );

    txt.set( sd.xrg_.start ).add( " - " ).add( sd.xrg_.stop );
    setCellTxt( 1, mXRow, txt );

    txt.set( sd.yrg_.start ).add( " - " ).add( sd.yrg_.stop );
    setCellTxt( 1, mYRow, txt );

    if ( Seis::isPS(gt) )
    {
	txt.set( sd.offsrg_.start ).add( " - " ).add( sd.offsrg_.stop );
	setCellTxt( 1, mPSRow, txt );
    }
}


void uiSEGYReadStarter::setTableFromScanDef()
{
    parsbeingset_ = true;
    revfld_->setCurrentItem( scandef_.revision_ );
    parsbeingset_ = false;
}


void uiSEGYReadStarter::setScanDefFromTable()
{
    scandef_.revision_ = revfld_->currentItem();
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    // transfer scandef to filepars_ and filereadopts_
    return false;
}
