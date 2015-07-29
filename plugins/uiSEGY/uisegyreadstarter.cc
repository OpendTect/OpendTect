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
#include "uicombobox.h"
#include "uifileinput.h"
#include "uitable.h"
#include "uiseparator.h"
#include "uihistogramdisplay.h"
#include "uimsg.h"
#include "survinfo.h"
#include "segyhdr.h"
#include "seistype.h"
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
    , geomtype_(Seis::Vol)
    , isvsp_(false)
    , filespec_(fs?*fs:FileSpec())
    , filereadopts_(0)
    , curusrfname_("x") // any non-empty
    , clipsampler_(*new DataClipSampler(100000))
    , infeet_(false)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("Data type") );
    typfld_ = lcb->box();
    typfld_->setHSzPol( uiObject::MedVar );
    typfld_->selectionChanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );
    if ( SI().has3D() )
    {
	addTyp( (int)Seis::Vol );
	addTyp( (int)Seis::VolPS );
    }
    if ( SI().has2D() )
    {
	addTyp( (int)Seis::Line );
	addTyp( (int)Seis::LinePS );
    }
    addTyp( -1 );
    filereadopts_ = new FileReadOpts( (Seis::GeomType)inptyps_[0] );

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true );
    inpfld_ = new uiFileInput( this, "Input file(s) (*=wildcard)",
				fisu );
    inpfld_->attach( alignedBelow, lcb );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, inpfld_ );

    infotbl_ = new uiTable( this, uiTable::Setup(mNrInfoRows,2)
				  .manualresize(true), "Info table" );
    infotbl_->attach( ensureBelow, sep );
    infotbl_->setColumnLabel( 0, "" );
    infotbl_->setColumnLabel( 1, "Detected" );
    infotbl_->setColumnStretchable( 0, false );
    infotbl_->setColumnStretchable( 1, true );
    infotbl_->setPrefWidthInChar( 80 );
    infotbl_->setPrefHeightInChar( mNrInfoRows+3 );
    infotbl_->setTableReadOnly( true );
    infotbl_->setLeftHeaderHidden( true );
    setCellTxt( 0, mRevRow, "SEG-Y Revision" );
    setCellTxt( 0, mDataFormatRow, "Data format" );
    setCellTxt( 0, mNrSamplesRow, "Samples per trace" );
    setCellTxt( 0, mZRangeRow, "Z Range" );

    uiHistogramDisplay::Setup hdsu;
    hdsu.noyaxis( false ).noygridline(true).annoty( false );
    ampldisp_ = new uiHistogramDisplay( this, hdsu );
    ampldisp_->setTitle( tr("Amplitude values") );
    ampldisp_->setStretch( 2, 1 );
    ampldisp_->setPrefHeight( 250 );
    ampldisp_->attach( stretchedBelow, infotbl_ );

    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filereadopts_;
    delete &clipsampler_;
}


void uiSEGYReadStarter::addTyp( int typ )
{
    inptyps_ += typ;

#    define mAddItem(txt,ic) { \
    typfld_->addItem( tr(txt) ); \
    typfld_->setIcon( typfld_->size()-1, ic ); }

    if ( typ < 0 )
        mAddItem( "Zero-offset VSP", "vsp0" )
    else if ( typ == (int)Seis::Vol )
	mAddItem( "3D seismic data", "seismiccube" )
    else if ( typ == (int)Seis::VolPS )
	mAddItem( "3D PreStack data", "prestackdataset" )
    else if ( typ == (int)Seis::Line )
	mAddItem( "2D Seismic data", "seismicline2d" )
    else if ( typ == (int)Seis::LinePS )
	mAddItem( "2D PreStack data", "prestackdataset2d" )
    else
	{ pErrMsg( "Huh" ); }
}


void uiSEGYReadStarter::setCellTxt( int col, int row, const char* txt )
{
    infotbl_->setText( RowCol(row,col), txt );
    if ( col==0 )
	infotbl_->resizeColumnToContents( col );
}


void uiSEGYReadStarter::initWin( CallBacker* )
{
    inpChg( 0 );
}


void uiSEGYReadStarter::inpChg( CallBacker* )
{
    const BufferString newusrfnm = inpfld_->fileName();
    const int newtype = inptyps_[ typfld_->getIntValue() ];
    const int oldtype = isvsp_ ? -1 : (int)geomtype_;
    const bool isnewfile = newusrfnm != curusrfname_;
    const bool isnewtype = newtype != oldtype;
    if ( !isnewfile && !isnewtype )
	return;

    curusrfname_ = newusrfnm;
    isvsp_ = newtype < 0;
    geomtype_ = (Seis::GeomType)(isvsp_ ? inptyps_[0] : newtype);

    const bool is2d = Seis::is2D(geomtype_);
    const bool isps = Seis::isPS(geomtype_);
    const char* k1str; const char* k2str; const char* psstr;
    const char* xstr; const char* ystr;
    k1str = k2str = psstr = xstr = ystr = "";
    if ( !isvsp_ )
    {
	xstr = "X-Coordinate range";
	ystr = "Y-Coordinate range";
	if ( is2d )
	{
	    k1str = "Trace numbers";
	    k2str = "Ref/SP numbers";
	}
	else
	{
	    k1str = "Inline numbers";
	    k2str = "Crossline numbers";
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
    if ( curusrfname_.isEmpty() )
	return false;

    filespec_.setEmpty();
    if ( !curusrfname_.find('*') )
    {
	if ( !getExistingFileName(curusrfname_) )
	    return false;
	filespec_.setFileName( curusrfname_ );
    }
    else
    {
	FilePath fp( curusrfname_ );
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
    if ( isfirst )
    {
	if ( !guessScanDef(strm) )
	    return false;
    }

    sd.getFromSEGYBody( strm, scandef_, isfirst ? &clipsampler_ : 0 );
    return true;
}


bool uiSEGYReadStarter::getHeaderBufData( od_istream& strm, char* buf )
{
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
    return true;
}


void uiSEGYReadStarter::displayScanResults()
{
    if ( scandata_.isEmpty() )
	return;

    SEGY::uiScanData sd( *scandata_[0] );
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

    txt.set( scandef_.ns_ );
    setCellTxt( 1, mNrSamplesRow, txt );

    const float endz = scandef_.sampling_.start
		     + (scandef_.ns_-1) * scandef_.sampling_.step;
    txt.set( scandef_.sampling_.start ).add( " to " ).add( endz )
	.add( " step " ).add( scandef_.sampling_.step )
	.add( " (seconds or " ).add( infeet_ ? "feet)" : "meter)" );
    setCellTxt( 1, mZRangeRow, txt );

    /* TODO: finish
    setCellTxt( 1, mKey1Row, txt );

    setCellTxt( 1, mKey2Row, txt );

    setCellTxt( 1, mXRow, txt );

    setCellTxt( 1, mYRow, txt );

    setCellTxt( 1, mPSRow, txt );
    */
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    // transfer scandef to filepars_ and filereadopts_
    return false;
}
