/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

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

static const char* sBytePos = "from header";
static const Color qscellcolor = Color( 255, 255, 225 ); // palish yellow


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


uiSEGYReadStartInfo::uiSEGYReadStartInfo( uiParent* p, SEGY::LoadDef& scd )
    : uiGroup(p,"SEGY read start info")
    , loaddef_(scd)
    , loaddefChanged(this)
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
    setCellTxt( mUseTxtCol, mRevRow, "" );
    setCellTxt( mItemCol, mDataFormatRow, "Data format" );
    setCellTxt( mUseTxtCol, mDataFormatRow, "" );
    setCellTxt( mItemCol, mNrSamplesRow, "Number of samples" );
    setCellTxt( mUseTxtCol, mNrSamplesRow, "" );
    setCellTxt( mItemCol, mZRangeRow, "Z Range" );
    setCellTxt( mUseTxtCol, mZRangeRow, "start / interval" );

    mkLoadDefFields();
}


void uiSEGYReadStartInfo::mkLoadDefFields()
{
    const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );
#   define mAddToTbl(fld,row) \
    fld->setStretch( 2, 1 ); \
    tbl_->setCellObject( RowCol(row,mUseCol), fld );

    const char* revstrs[] = { "0", "1", "2", 0 };
    revfld_ = new uiComboBox( 0, revstrs, "Revision" );
    revfld_->selectionChanged.notify( mCB(this,uiSEGYReadStartInfo,revChg) );
    mAddToTbl( revfld_, mRevRow );

    fmtfld_ = new uiComboBox( 0, SEGY::FilePars::getFmts(false), "Format" );
    fmtfld_->selectionChanged.notify( parchgcb );
    mAddToTbl( fmtfld_, mDataFormatRow );

    nsfld_ = new uiSpinBox( 0, 0, "Samples" );
    nsfld_->setInterval( 1, mMaxReasonableNS, 1 );
    nsfld_->valueChanged.notify( parchgcb );
    mAddToTbl( nsfld_, mNrSamplesRow );

    uiGroup* grp = new uiGroup( 0, "Z Range" );
    zstartfld_ = new uiLineEdit( grp, FloatInpSpec(0.f), "Z Start" );
    zstartfld_->editingFinished.notify( parchgcb );
    zstartfld_->setStretch( 2, 1 );
    srfld_ = new uiLineEdit( grp, FloatInpSpec(0.f,0.f), "Z Interval" );
    srfld_->editingFinished.notify( parchgcb );
    srfld_->setStretch( 2, 1 );
    srfld_->attach( rightOf, zstartfld_ );
    tbl_->setCellGroup( RowCol(mZRangeRow,mUseCol), grp );

    xcoordbytefld_ = new uiSEGYByteNr( 0, "X-coord byte" );
    xcoordbytefld_->selectionChanged.notify( parchgcb );
    mAddToTbl( xcoordbytefld_, mXRow );
    ycoordbytefld_ = new uiSEGYByteNr( 0, "Y-coord byte" );
    ycoordbytefld_->selectionChanged.notify( parchgcb );
    mAddToTbl( ycoordbytefld_, mYRow );
    key1bytefld_ = new uiSEGYByteNr( 0, "Key1 byte" );
    key1bytefld_->selectionChanged.notify( parchgcb );
    mAddToTbl( key1bytefld_, mKey1Row );
    key2bytefld_ = new uiSEGYByteNr( 0, "Key2 byte" );
    key2bytefld_->selectionChanged.notify( parchgcb );
    mAddToTbl( key2bytefld_, mKey2Row );
    offsetbytefld_ = new uiSEGYByteNr( 0, "Offset byte" );
    offsetbytefld_->selectionChanged.notify( parchgcb );
    mAddToTbl( offsetbytefld_, mPSRow );

    useLoadDef();
}


void uiSEGYReadStartInfo::setCellTxt( int col, int row, const char* txt )
{
    tbl_->setText( RowCol(row,col), txt );
    if ( col == mItemCol || col == mUseTxtCol )
	tbl_->resizeColumnToContents( col );
    if ( col == mQSResCol )
	tbl_->setColor( RowCol(row,mQSResCol), *txt ? qscellcolor
						    : Color::White() );
}


void uiSEGYReadStartInfo::revChg( CallBacker* )
{
    parChg( 0 );
    showRelevantInfo();
}


void uiSEGYReadStartInfo::parChg( CallBacker* )
{
    if ( parsbeingset_ )
	return;

    fillLoadDef();
    showRelevantInfo();
    loaddefChanged.trigger();
}


void uiSEGYReadStartInfo::setImpTypIdx( int idx )
{
    imptype_.tidx_ = idx;
    showRelevantInfo();
}


void uiSEGYReadStartInfo::showRelevantInfo()
{
    const Seis::GeomType gt = imptype_.geomType();
    const bool isvsp = imptype_.isVSP();
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    const bool isrev0 = loaddef_.isRev0();

    const char* xittxt; const char* yittxt;
    const char* ky1ittxt; const char* ky2ittxt; const char* offsittxt;
    const char* xustxt; const char* yustxt;
    const char* ky1ustxt; const char* ky2ustxt; const char* offsustxt;
    xittxt = yittxt = ky1ittxt = ky2ittxt = offsittxt =
    xustxt = yustxt = ky1ustxt = ky2ustxt = offsustxt = "";

    if ( !isvsp )
    {
	xittxt = "X-Coordinate range"; yittxt = "Y-Coordinate range";
	ky1ittxt = is2d ? "Trace number range" : "Inline range";
	ky2ittxt = is2d ? "Ref/SP number range" : "Crossline range";
	if ( isps )
	    offsittxt = "Offset range";

	if ( isrev0 )
	{
	    xustxt = yustxt = ky1ustxt = ky2ustxt = sBytePos;
	    if ( isps )
		offsustxt = sBytePos;
	}
    }

    setCellTxt( mItemCol, mXRow, xittxt );
    setCellTxt( mItemCol, mYRow, yittxt );
    setCellTxt( mItemCol, mKey1Row, ky1ittxt );
    setCellTxt( mItemCol, mKey2Row, ky2ittxt );
    setCellTxt( mItemCol, mPSRow, offsittxt );
    setCellTxt( mQSResCol, mKey1Row,
		isvsp ? "" : (is2d ? trcnrinfotxt_ : inlinfotxt_).buf() );
    setCellTxt( mQSResCol, mKey2Row,
		isvsp ? "" : (is2d ? refnrinfotxt_ : crlinfotxt_).buf() );
    setCellTxt( mQSResCol, mXRow, isvsp ? "" : xinfotxt_ );
    setCellTxt( mQSResCol, mYRow, isvsp ? "" : yinfotxt_ );
    setCellTxt( mQSResCol, mPSRow, isvsp || !isps ? "" : offsetinfotxt_.buf() );
    setCellTxt( mUseTxtCol, mKey1Row, ky1ustxt );
    setCellTxt( mUseTxtCol, mKey2Row, ky2ustxt );
    setCellTxt( mUseTxtCol, mXRow, xustxt );
    setCellTxt( mUseTxtCol, mYRow, yustxt );
    setCellTxt( mUseTxtCol, mPSRow, offsustxt );

    xcoordbytefld_->display( !isvsp && isrev0 );
    ycoordbytefld_->display( !isvsp && isrev0 );
    key1bytefld_->display( !isvsp && (isrev0 || is2d) );
    key2bytefld_->display( !isvsp && isrev0 );
    offsetbytefld_->display( !isvsp && isps );
}


void uiSEGYReadStartInfo::clearInfo()
{
    xinfotxt_.setEmpty(); yinfotxt_.setEmpty();
    inlinfotxt_.setEmpty(); crlinfotxt_.setEmpty();
    trcnrinfotxt_.setEmpty(); refnrinfotxt_.setEmpty();
    offsetinfotxt_.setEmpty();

    showRelevantInfo();

    for ( int irow=0; irow<mKey1Row; irow++ )
	setCellTxt( mQSResCol, irow, "" );
}


void uiSEGYReadStartInfo::setScanInfo( const SEGY::ScanInfo& si )
{
    clearInfo();
    if ( !si.isUsable() )
	return;

    BufferString txt;

    txt.set( loaddef_.revision_ );
    setCellTxt( mQSResCol, mRevRow, txt );

    const char** fmts = SEGY::FilePars::getFmts(false);
    txt.set( loaddef_.format_ < 4 ? fmts[loaddef_.format_-1]
	    : (loaddef_.format_==8 ? fmts[4] : fmts[3]) );
    if ( loaddef_.hdrsswapped_ && loaddef_.dataswapped_ )
	txt.add( " (all bytes swapped)" );
    else if ( loaddef_.hdrsswapped_ )
	txt.add( " (header bytes swapped)" );
    else if ( loaddef_.dataswapped_ )
	txt.add( " (data bytes swapped)" );
    setCellTxt( mQSResCol, mDataFormatRow, txt );

    txt.set( loaddef_.ns_ ).add( " (" ).add( si.nrtrcs_ )
	.add( si.nrtrcs_ == 1 ? " trace)" : " traces)" );
    setCellTxt( mQSResCol, mNrSamplesRow, txt );

    if ( mIsUdf(loaddef_.sampling_.step) )
	txt.set( "" );
    else
    {
	const float endz = loaddef_.sampling_.start
			 + (loaddef_.ns_-1) * loaddef_.sampling_.step;
	txt.set( loaddef_.sampling_.start ).add( " - " ).add( endz )
	    .add( " (s or " ).add( si.infeet_ ? "ft)" : "m)" );
    }
    setCellTxt( mQSResCol, mZRangeRow, txt );

    inlinfotxt_.set( si.inls_.start ).add( " - " ).add( si.inls_.stop );
    crlinfotxt_.set( si.crls_.start ).add( " - " ).add( si.crls_.stop );
    trcnrinfotxt_.set( si.trcnrs_.start ).add( " - " ).add( si.trcnrs_.stop );
    xinfotxt_.set( si.xrg_.start ).add( " - " ).add( si.xrg_.stop );
    yinfotxt_.set( si.yrg_.start ).add( " - " ).add( si.yrg_.stop );
    offsetinfotxt_.set( si.offsrg_.start ).add( " - " ).add( si.offsrg_.stop );
    if ( mIsUdf(si.refnrs_.start) )
	refnrinfotxt_.set( "<no data>" );
    else
	refnrinfotxt_.set(si.refnrs_.start).add(" - ").add(si.refnrs_.stop);

    useLoadDef();
    showRelevantInfo();
}


void uiSEGYReadStartInfo::useLoadDef()
{
    parsbeingset_ = true;

    revfld_->setCurrentItem( loaddef_.revision_ );

    const char** fmts = SEGY::FilePars::getFmts(false);
    const char* fmt = *fmts;
    for ( int idx=0; fmt; idx++ )
    {
	fmt = fmts[idx];
	if ( !fmt )
	    { pErrMsg("Format not found"); break; }
	else if ( (short)(*fmt - '0') == loaddef_.format_ )
	    { fmtfld_->setCurrentItem( idx ); break; }
    }

    nsfld_->setValue( loaddef_.ns_ );
    zstartfld_->setValue( loaddef_.sampling_.start );
    srfld_->setValue( loaddef_.sampling_.step );
    if ( imptype_.isVSP() )
	return;

    xcoordbytefld_->setByteNr( loaddef_.hdrdef_->xcoord_.bytepos_ );
    ycoordbytefld_->setByteNr( loaddef_.hdrdef_->ycoord_.bytepos_ );
    offsetbytefld_->setByteNr( loaddef_.hdrdef_->offs_.bytepos_ );
    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
    {
	key1bytefld_->setByteNr( loaddef_.hdrdef_->trnr_.bytepos_ );
	key2bytefld_->setByteNr( loaddef_.hdrdef_->refnr_.bytepos_ );
    }
    else
    {
	key1bytefld_->setByteNr( loaddef_.hdrdef_->inl_.bytepos_ );
	key2bytefld_->setByteNr( loaddef_.hdrdef_->crl_.bytepos_ );
    }

    parsbeingset_ = false;
}


void uiSEGYReadStartInfo::fillLoadDef()
{
    loaddef_.revision_ = revfld_->currentItem();
    loaddef_.format_ = (short)(*fmtfld_->text() - '0');
    loaddef_.ns_ = nsfld_->getIntValue();
    loaddef_.sampling_.start = zstartfld_->getFValue();
    loaddef_.sampling_.step = srfld_->getFValue();

    if ( imptype_.isVSP() )
	return;

    loaddef_.hdrdef_->xcoord_ = xcoordbytefld_->hdrEntry();
    loaddef_.hdrdef_->ycoord_ = ycoordbytefld_->hdrEntry();

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::isPS(gt) )
	loaddef_.hdrdef_->offs_ = offsetbytefld_->hdrEntry();
    if ( Seis::is2D(gt) )
    {
	loaddef_.hdrdef_->trnr_ = key1bytefld_->hdrEntry();
	loaddef_.hdrdef_->refnr_ = key2bytefld_->hdrEntry();
    }
    else
    {
	loaddef_.hdrdef_->inl_ = key1bytefld_->hdrEntry();
	loaddef_.hdrdef_->crl_ = key2bytefld_->hdrEntry();
    }
}
