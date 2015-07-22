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
#include "uifileinput.h"
#include "uitable.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "survinfo.h"
#include "seistype.h"

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
    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true );
    inpfld_ = new uiFileInput( this, "Input file (multiple use wildcard)",
				fisu );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );

    uiStringSet inps;
    if ( SI().has3D() )
    {
	addTyp( inps, (int)Seis::Vol );
	addTyp( inps, (int)Seis::VolPS );
    }
    if ( SI().has2D() )
    {
	addTyp( inps, (int)Seis::Line );
	addTyp( inps, (int)Seis::LinePS );
    }
    addTyp( inps, -1 );
    filereadopts_ = new FileReadOpts( (Seis::GeomType)inptyps_[0] );

    typfld_ = new uiGenInput( this, tr("Data contained"),
				StringListInpSpec(inps) );
    typfld_->attach( alignedBelow, inpfld_ );
    typfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, typfld_ );

    infotbl_ = new uiTable( this, uiTable::Setup(mNrInfoRows,2)
				  .manualresize(true), "Info table" );
    infotbl_->attach( ensureBelow, sep );
    infotbl_->setColumnLabel( 0, "" );
    infotbl_->setColumnLabel( 1, "Detected" );
    for ( int idx=0; idx<mNrInfoRows; idx++ )
	infotbl_->setRowLabel( idx, "" );
    setCellTxt( 0, mRevRow, "SEG-Y Revision" );
    setCellTxt( 0, mByteOrderRow, "Byte order (headers, data)" );
    setCellTxt( 0, mNrSamplesRow, "Samples per trace" );
    setCellTxt( 0, mZRangeRow, "Z Range" );

    postFinalise().notify( mCB(this,uiSEGYReadStarter,inpChg) );
}


void uiSEGYReadStarter::setCellTxt( int col, int row, const char* txt )
{
    infotbl_->setText( RowCol(row,col), txt );
}


void uiSEGYReadStarter::addTyp( uiStringSet& inps, int typ )
{
    inptyps_ += typ;
    if ( typ < 0 )
	inps += tr( "Zero-offset VSP" );
    else if ( typ == (int)Seis::Vol )
	inps += tr( "3D seismic data" );
    else if ( typ == (int)Seis::VolPS )
	inps += tr( "3D PreStack data" );
    else if ( typ == (int)Seis::Line )
	inps += tr( "2D Seismic data" );
    else if ( typ == (int)Seis::LinePS )
	inps += tr( "2D PreStack data" );
    else
	{ pErrMsg( "Huh" ); }
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete filereadopts_;
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

    uiMSG().error( BufferString("TODO: scan ",curusrfname_) );
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    return false;
}
