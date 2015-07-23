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
#include "uimsg.h"
#include "survinfo.h"
#include "seistype.h"
#include "filepath.h"
#include "dirlist.h"
#include "oddirs.h"

#define mNrInfoRows 9
#define mRevRow 0
#define mByteOrderRow 1
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
    infotbl_->setPrefHeightInRows( mNrInfoRows );
    infotbl_->setTableReadOnly( true );
    infotbl_->setLeftHeaderHidden( true );
    setCellTxt( 0, mRevRow, "SEG-Y Revision" );
    setCellTxt( 0, mByteOrderRow, "Byte order (headers, data)" );
    setCellTxt( 0, mNrSamplesRow, "Samples per trace" );
    setCellTxt( 0, mZRangeRow, "Z Range" );

    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


void uiSEGYReadStarter::setCellTxt( int col, int row, const char* txt )
{
    infotbl_->setText( RowCol(row,col), txt );
    if ( col==0 )
	infotbl_->resizeColumnToContents( col );
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


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filereadopts_;
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

    scanInput();
}


void uiSEGYReadStarter::scanInput()
{
    for ( int idx=0; idx<mNrInfoRows; idx++ )
	setCellTxt( 1, idx, "" );

    if ( curusrfname_.isEmpty() )
	return;

    filespec_.setEmpty();
    if ( curusrfname_.find('*') )
    {
	if ( !getMultipleFileNames() )
	    return;
    }
    else
    {
	if ( !checkExist(curusrfname_) )
	    return;
	filespec_.setFileName( curusrfname_ );
    }

    BufferString msg( "TODO: scan:\n" );
    const int nrfiles = filespec_.nrFiles();
    for ( int idx=0; idx<nrfiles; idx++ )
	msg.add( filespec_.fileName(idx) ).add( "\n" );
    uiMSG().error( msg );
}


bool uiSEGYReadStarter::getMultipleFileNames()
{
    FilePath fp( curusrfname_ );
    if ( !fp.isAbsolute() )
    {
	uiMSG().error(
	   tr("Please specify the absolute file name when using a wildcard.") );
	return false;
    }

    DirList dl( fp.pathOnly(), DirList::FilesOnly, fp.fileName() );
    for ( int idx=0; idx<dl.size(); idx++ )
	filespec_.fnames_.add( dl.fullPath(idx) );

    if ( filespec_.isEmpty() )
    {
	uiMSG().error(
	   tr("No file names matching your wildcard(s).") );
	return false;
    }

    return true;
}


bool uiSEGYReadStarter::checkExist( BufferString& fnm, bool emiterr )
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


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    return false;
}
