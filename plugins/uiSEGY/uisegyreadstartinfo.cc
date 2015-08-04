/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadstartinfo.h"
#include "segyuiscandata.h"
#include "segyhdr.h"
#include "uitable.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uisegyimptype.h"
#include "uisegydef.h"

#define mNrInfoRows 9
#define mNrInfoCols 4

#define mRevRow 0
#define mDataFormatRow 1
#define mNrSamplesRow 2
#define mZRangeRow 3
#define mKey1Row 4
#define mKey2Row 5
#define mXRow 6
#define mYRow 7
#define mPSRow 8

#define mItemCol 0
#define mQSResCol 1
#define mUseTxtCol 2
#define mUseCol 3


class uiSEGYByteNr : public uiComboBox
{
public:

uiSEGYByteNr( uiParent* p, const char* nm )
    : uiComboBox(p,nm)
    , hdef_(SEGY::TrcHeader::hdrDef())
{
    for ( int idx=0; idx<hdef_.size(); idx++ )
    {
	const SEGY::HdrEntry& he = *hdef_[idx];
	BufferString txt( he.name(), "(byte " );
        txt.add( he.bytepos_+1 ).add( ") - \"" );
        txt.add( he.description() ).add( "\"" );
	addItem( txt );
    }
}

SEGY::HdrEntry hdrEntry() const
{
    const int selidx = currentItem();
    SEGY::HdrEntry ret;
    if( selidx >= 0 )
    {
	ret = *hdef_[selidx];
	ret.bytepos_++; // convert to 'user' byte number
    }
    return ret;
}

void setByteNr( short bnr )
{
    bnr--; // input will be 'user' byte number

    int selidx = -1;
    for ( int idx=0; idx<hdef_.size(); idx++ )
    {
	const SEGY::HdrEntry& he = *hdef_[idx];
	if ( he.bytepos_ == bnr )
	    { selidx = idx; break; }
    }
    if ( selidx >= 0 )
	setCurrentItem( selidx );
}

    const SEGY::HdrDef&	hdef_;

};


uiSEGYReadStartInfo::uiSEGYReadStartInfo( uiParent* p, SEGY::uiScanDef& scd )
    : uiGroup(p,"SEGY read start info")
    , scandef_(scd)
    , scandefChanged(this)
    , parsbeingset_(false)
{
    tbl_ = new uiTable( this, uiTable::Setup(mNrInfoRows,mNrInfoCols)
				  .manualresize(true), "Info table" );
    tbl_->setColumnLabel( mItemCol, "" );
    tbl_->setColumnLabel( mQSResCol, "Quick scan result" );
    tbl_->setColumnLabel( mUseTxtCol, "" );
    tbl_->setColumnLabel( mUseCol, "Actually use" );
    tbl_->setColumnStretchable( mItemCol, false );
    tbl_->setColumnStretchable( mQSResCol, true );
    tbl_->setColumnStretchable( mUseTxtCol, false );
    tbl_->setColumnStretchable( mUseCol, true );
    tbl_->setPrefWidthInChar( 120 );
    tbl_->setPrefHeightInRows( mNrInfoRows );
    tbl_->setTableReadOnly( true );
    tbl_->setLeftHeaderHidden( true );
    setCellTxt( mItemCol, mRevRow, "SEG-Y Revision" );
    setCellTxt( mUseTxtCol, mRevRow, "select" );
    setCellTxt( mItemCol, mDataFormatRow, "Data format" );
    setCellTxt( mUseTxtCol, mDataFormatRow, "select" );
    setCellTxt( mItemCol, mNrSamplesRow, "Number of samples" );
    setCellTxt( mUseTxtCol, mNrSamplesRow, "actual" );
    setCellTxt( mItemCol, mZRangeRow, "Z Range" );
    setCellTxt( mUseTxtCol, mZRangeRow, "Start / Interval" );

    const CallBack cb( mCB(this,uiSEGYReadStartInfo,parChg) );
#   define mAddToTbl(fld,row) \
    fld->setStretch( 2, 1 ); \
    tbl_->setCellObject( RowCol(row,mUseCol), fld );

    const char* revstrs[] = { "0", "1", 0 };
    revfld_ = new uiComboBox( 0, revstrs, "Revision" );
    revfld_->selectionChanged.notify( cb );
    mAddToTbl( revfld_, mRevRow );

    fmtfld_ = new uiComboBox( 0, SEGY::FilePars::getFmts(false), "Format" );
    fmtfld_->selectionChanged.notify( cb );
    mAddToTbl( fmtfld_, mDataFormatRow );

    nsfld_ = new uiSpinBox( 0, 0, "Samples" );
    nsfld_->setInterval( 1, mMaxReasonableNS, 1 );
    nsfld_->valueChanged.notify( cb );
    mAddToTbl( nsfld_, mNrSamplesRow );

    uiGroup* grp = new uiGroup( 0, "Z Range" );
    zstartfld_ = new uiLineEdit( grp, FloatInpSpec(0.f), "Z Start" );
    zstartfld_->editingFinished.notify( cb );
    zstartfld_->setStretch( 2, 1 );
    srfld_ = new uiLineEdit( grp, FloatInpSpec(0.f,0.f), "Z Interval" );
    srfld_->editingFinished.notify( cb );
    srfld_->setStretch( 2, 1 );
    srfld_->attach( rightOf, zstartfld_ );
    tbl_->setCellGroup( RowCol(mZRangeRow,mUseCol), grp );

    xcoordbytefld_ = new uiSEGYByteNr( 0, "X-coord byte" );
    xcoordbytefld_->selectionChanged.notify( cb );
    mAddToTbl( xcoordbytefld_, mXRow );
    ycoordbytefld_ = new uiSEGYByteNr( 0, "Y-coord byte" );
    ycoordbytefld_->selectionChanged.notify( cb );
    mAddToTbl( ycoordbytefld_, mYRow );
    key1bytefld_ = new uiSEGYByteNr( 0, "Key1 byte" );
    key1bytefld_->selectionChanged.notify( cb );
    mAddToTbl( key1bytefld_, mKey1Row );
    key2bytefld_ = new uiSEGYByteNr( 0, "Key2 byte" );
    key2bytefld_->selectionChanged.notify( cb );
    mAddToTbl( key2bytefld_, mKey2Row );
    offsetbytefld_ = new uiSEGYByteNr( 0, "Offset byte" );
    offsetbytefld_->selectionChanged.notify( cb );
    mAddToTbl( offsetbytefld_, mPSRow );

    useScanDef();
}


void uiSEGYReadStartInfo::setCellTxt( int col, int row, const char* txt )
{
    tbl_->setText( RowCol(row,col), txt );
    if ( col == mItemCol || col == mUseTxtCol )
	tbl_->resizeColumnToContents( col );
}


void uiSEGYReadStartInfo::parChg( CallBacker* )
{
    if ( parsbeingset_ )
	return;

    scandefChanged.trigger();
}


void uiSEGYReadStartInfo::setImpTypIdx( int idx )
{
    imptype_.tidx_ = idx;

    const Seis::GeomType gt = imptype_.geomType();
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    const bool isvsp = imptype_.isVSP();
    const char* k1ittxt; const char* k2ittxt; const char* psittxt;
    const char* xittxt; const char* yittxt;
    const char* k1hdrtxt; const char* k2hdrtxt; const char* pshdrtxt;
    const char* xhdrtxt; const char* yhdrtxt;
    k1ittxt = k2ittxt = psittxt = xittxt = yittxt = "";
    k1hdrtxt = k2hdrtxt = pshdrtxt = xhdrtxt = yhdrtxt = "";
    static const char* bytepostxt = "At header";

    if ( !isvsp )
    {
	xhdrtxt = yhdrtxt = k1hdrtxt = k2hdrtxt = bytepostxt;
	xittxt = "X-Coordinate range";
	yittxt = "Y-Coordinate range";
	if ( is2d )
	{
	    k1ittxt = "Trace number range";
	    k2ittxt = "Ref/SP number range";
	}
	else
	{
	    k1ittxt = "Inline range";
	    k2ittxt = "Crossline range";
	}
	if ( isps )
	{
	    psittxt = "Offset range";
	    pshdrtxt = bytepostxt;
	}
    }
    setCellTxt( mItemCol, mKey1Row, k1ittxt );
    setCellTxt( mItemCol, mKey2Row, k2ittxt );
    setCellTxt( mItemCol, mXRow, xittxt );
    setCellTxt( mItemCol, mYRow, yittxt );
    setCellTxt( mItemCol, mPSRow, psittxt );
    setCellTxt( mUseTxtCol, mKey1Row, k1hdrtxt );
    setCellTxt( mUseTxtCol, mKey2Row, k2hdrtxt );
    setCellTxt( mUseTxtCol, mXRow, xhdrtxt );
    setCellTxt( mUseTxtCol, mYRow, yhdrtxt );
    setCellTxt( mUseTxtCol, mPSRow, pshdrtxt );

    xcoordbytefld_->display( !isvsp );
    ycoordbytefld_->display( !isvsp );
    key1bytefld_->display( !isvsp );
    key2bytefld_->display( !isvsp );
    offsetbytefld_->display( !isvsp && isps );
}


void uiSEGYReadStartInfo::clearInfo()
{
    for ( int idx=0; idx<mNrInfoRows; idx++ )
	setCellTxt( 1, idx, "" );
}


void uiSEGYReadStartInfo::setScanData( const SEGY::uiScanData& sd )
{
    clearInfo();
    if ( !sd.isUsable() )
	return;

    BufferString txt;

    txt.set( scandef_.revision_ );
    setCellTxt( mQSResCol, mRevRow, txt );

    const char** fmts = SEGY::FilePars::getFmts(false);
    txt.set( scandef_.format_ < 4 ? fmts[scandef_.format_-1]
	    : (scandef_.format_==8 ? fmts[4] : fmts[3]) );
    if ( scandef_.hdrsswapped_ && scandef_.dataswapped_ )
	txt.add( " (all bytes swapped)" );
    else if ( scandef_.hdrsswapped_ )
	txt.add( " (header bytes swapped)" );
    else if ( scandef_.dataswapped_ )
	txt.add( " (data bytes swapped)" );
    setCellTxt( mQSResCol, mDataFormatRow, txt );

    txt.set( scandef_.ns_ ).add( " (" ).add( sd.nrtrcs_ )
	.add( sd.nrtrcs_ == 1 ? " trace)" : " traces)" );
    setCellTxt( mQSResCol, mNrSamplesRow, txt );

    const float endz = scandef_.sampling_.start
		     + (scandef_.ns_-1) * scandef_.sampling_.step;
    txt.set( scandef_.sampling_.start ).add( " - " ).add( endz )
	.add( " (s or " ).add( sd.infeet_ ? "ft)" : "m)" );
    setCellTxt( mQSResCol, mZRangeRow, txt );

    if ( imptype_.isVSP() )
	return;

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
	txt.set( sd.trcnrs_.start ).add( " - " ).add( sd.trcnrs_.stop );
    else
	txt.set( sd.inls_.start ).add( " - " ).add( sd.inls_.stop );
    setCellTxt( mQSResCol, mKey1Row, txt );

    if ( Seis::is2D(gt) )
	txt.set( sd.refnrs_.start ).add( " - " ).add( sd.refnrs_.stop );
    else
	txt.set( sd.crls_.start ).add( " - " ).add( sd.crls_.stop );
    setCellTxt( mQSResCol, mKey2Row, txt );

    txt.set( sd.xrg_.start ).add( " - " ).add( sd.xrg_.stop );
    setCellTxt( mQSResCol, mXRow, txt );

    txt.set( sd.yrg_.start ).add( " - " ).add( sd.yrg_.stop );
    setCellTxt( mQSResCol, mYRow, txt );

    if ( Seis::isPS(gt) )
    {
	txt.set( sd.offsrg_.start ).add( " - " ).add( sd.offsrg_.stop );
	setCellTxt( mQSResCol, mPSRow, txt );
    }

    useScanDef();
}


void uiSEGYReadStartInfo::useScanDef()
{
    parsbeingset_ = true;

    revfld_->setCurrentItem( scandef_.revision_ );

    const char** fmts = SEGY::FilePars::getFmts(false);
    const char* fmt = *fmts;
    for ( int idx=0; fmt; idx++ )
    {
	fmt = fmts[idx];
	if ( !fmt )
	    { pErrMsg("Format not found"); break; }
	else if ( (short)(*fmt - '0') == scandef_.format_ )
	    { fmtfld_->setCurrentItem( idx ); break; }
    }

    nsfld_->setValue( scandef_.ns_ );
    zstartfld_->setValue( scandef_.sampling_.start );
    srfld_->setValue( scandef_.sampling_.step );
    if ( imptype_.isVSP() )
	return;

    xcoordbytefld_->setByteNr( scandef_.hdrdef_->xcoord_.bytepos_ );
    ycoordbytefld_->setByteNr( scandef_.hdrdef_->ycoord_.bytepos_ );
    offsetbytefld_->setByteNr( scandef_.hdrdef_->offs_.bytepos_ );
    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
    {
	key1bytefld_->setByteNr( scandef_.hdrdef_->trnr_.bytepos_ );
	key2bytefld_->setByteNr( scandef_.hdrdef_->refnr_.bytepos_ );
    }
    else
    {
	key1bytefld_->setByteNr( scandef_.hdrdef_->inl_.bytepos_ );
	key2bytefld_->setByteNr( scandef_.hdrdef_->crl_.bytepos_ );
    }

    parsbeingset_ = false;
}


void uiSEGYReadStartInfo::fillScanDef()
{
    scandef_.revision_ = revfld_->currentItem();
    scandef_.format_ = (short)(*fmtfld_->text() - '0');
    scandef_.ns_ = nsfld_->getIntValue();
    scandef_.sampling_.start = zstartfld_->getFValue();
    scandef_.sampling_.step = srfld_->getFValue();

    if ( imptype_.isVSP() )
	return;

    scandef_.hdrdef_->xcoord_ = xcoordbytefld_->hdrEntry();
    scandef_.hdrdef_->ycoord_ = ycoordbytefld_->hdrEntry();

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::isPS(gt) )
	scandef_.hdrdef_->offs_ = offsetbytefld_->hdrEntry();
    if ( Seis::is2D(gt) )
    {
	scandef_.hdrdef_->trnr_ = key1bytefld_->hdrEntry();
	scandef_.hdrdef_->refnr_ = key2bytefld_->hdrEntry();
    }
    else
    {
	scandef_.hdrdef_->inl_ = key1bytefld_->hdrEntry();
	scandef_.hdrdef_->crl_ = key2bytefld_->hdrEntry();
    }
}
