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
#include "segyhdrkeydata.h"
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
#define mHaveNonBasicRows (nrrows_ > mKey1Row)
#define mHavePSRow (nrrows_ > mPSRow)

#define mItemCol 0
#define mQSResCol 1
#define mUseTxtCol 2
#define mUseCol 3

static const Color qscellcolor = Color( 255, 255, 225 ); // palish yellow
static const Color optqscellcolor = Color( 255, 255, 245 ); // even paler yellow
static const Color optcellcolor = Color( 245, 245, 245 ); // very light grey

#define mGetGeomType(varnm) const Seis::GeomType varnm = imptype_.geomType()
#define mGetParChgCB(varnm) \
    const CallBack varnm( mCB(this,uiSEGYReadStartInfo,parChg) )

#define sEmpty uiString::emptyString()


class uiSEGYByteNr : public uiGroup
{ mODTextTranslationClass(uiSEGYByteNr)
public:

uiSEGYByteNr( uiParent* p, const char* nm )
    : uiGroup(p,nm)
    , changed(this)
    , hdef_(SEGY::TrcHeader::hdrDef())
    , nonefound_(false)
{
    bytenrfld_ = new uiComboBox( this, "Byte number" );
    bytenrfld_->setStretch( 2, 2 );
    szfld_ = new uiComboBox( this, "Size" );
    szfld_->setHSzPol( uiObject::Small );
    szfld_->addItem( toUiString("2 bytes") );
    szfld_->addItem( toUiString("4 bytes") );
    szfld_->setCurrentItem( 1 );
    szfld_->attach( rightTo, bytenrfld_, 0 );

    mAttachCB( bytenrfld_->selectionChanged, uiSEGYByteNr::changeCB );
    mAttachCB( szfld_->selectionChanged, uiSEGYByteNr::changeCB );
}

~uiSEGYByteNr()
{
    detachAllNotifiers();
}

void changeCB( CallBacker* cb )
{
    if ( cb==bytenrfld_ )
    {
	const int selidx = nonefound_ ? -1 : bytenrfld_->currentItem();
	if ( selidx >= 0 )
	{
	    const bool issmall = hdef_[ heidxs_[selidx] ]->issmall_;
	    szfld_->setCurrentItem( issmall ? 0 : 1 );
	}
    }

    changed.trigger( cb );
}

SEGY::HdrEntry hdrEntry() const
{
    const int selidx = nonefound_ ? -1 : bytenrfld_->currentItem();
    SEGY::HdrEntry ret(nullptr,nullptr);
    if ( selidx >= 0 )
    {
	ret = *hdef_[ heidxs_[selidx] ];
	ret.bytepos_++; // convert to 'user' byte number
	ret.issmall_ = szfld_->currentItem()==0;
    }
    return ret;
}

void setHdrEntry( const SEGY::HdrEntry& entry )
{
    if ( bytenrfld_->isEmpty() || nonefound_ )
	return;

    short bnr = entry.bytepos_;
    const bool issmall = entry.issmall_;
    NotifyStopper ns( changed );
    if ( bnr%2 )
	bnr--; // input was 'user' byte number

    for ( int idx=0; idx<heidxs_.size(); idx++ )
    {
	const SEGY::HdrEntry& he = *hdef_[ heidxs_[idx] ];
	if ( he.bytepos_ == bnr )
	{
	    bytenrfld_->setCurrentItem( idx );
	    szfld_->setCurrentItem( issmall ? 0 : 1 );
	    break;
	}
    }
}

void setEntries( const SEGY::HdrEntryDataSet& ds )
{
    NotifyStopper ns( changed );
    bytenrfld_->setEmpty(); heidxs_.setEmpty();

    for ( int idx=0; idx<ds.idxs_.size(); idx++ )
    {
	const int heidx = ds.idxs_[idx];
	const SEGY::HdrEntry& he = *hdef_[ heidx ];
	uiString txt( tr("%1 (byte %2) - \"%3\"")
		     .arg(he.name()).arg(he.bytepos_+1).arg(he.description()) );

	bytenrfld_->addItem( txt ); heidxs_ += heidx;
    }

    uiString ttip; nonefound_ = false;
    if ( bytenrfld_->isEmpty() )
    {
	nonefound_ = true;
	ttip = tr("The scan found invalid numbers in all possible fields for "
		    "this entry.");
	bytenrfld_->addItem( tr("<Nothing valid found>") );
    }
    bytenrfld_->setToolTip( ttip );
}

    Notifier<uiSEGYByteNr> changed;
    uiComboBox*		bytenrfld_;
    uiComboBox*		szfld_;

    const SEGY::HdrDef&	hdef_;
    TypeSet<int>	heidxs_;
    bool		nonefound_;

};



uiSEGYReadStartInfo::uiSEGYReadStartInfo( uiParent* p, SEGY::LoadDef& scd,
					  const SEGY::ImpType* imptyp )
    : uiGroup(p,"SEGY read start info")
    , loaddef_(scd)
    , loaddefChanged(this)
    , revChanged(this)
    , parsbeingset_(false)
    , xcoordbytefld_(nullptr)
    , ycoordbytefld_(nullptr)
    , inlbytefld_(nullptr)
    , crlbytefld_(nullptr)
    , refnrbytefld_(nullptr)
    , trcnrbytefld_(nullptr)
    , trcnrsrcfld_(nullptr)
    , trcnrgengrp_(nullptr)
    , psoffsrcfld_(nullptr)
    , offsetbytefld_(nullptr)
    , offsgengrp_(nullptr)
    , inptypfixed_(imptyp)
    , nrunswappedfmts_(5)
    , sBytePos(tr("from header"))
{
    nrrows_ = mNrInfoRows;
    if ( inptypfixed_ )
    {
	imptype_ = *imptyp;
	if ( imptype_.isVSP() )
	    nrrows_ = 4;
	else if ( !Seis::isPS(imptype_.geomType()) )
	    nrrows_ = 8;
    }

    tbl_ = new uiTable( this, uiTable::Setup(nrrows_,mNrInfoCols)
				  .manualresize(true), "Info table" );
    tbl_->setColumnLabel( mItemCol, sEmpty );
    tbl_->setColumnLabel( mQSResCol, tr("Quick scan result") );
    tbl_->setColumnLabel( mUseTxtCol, sEmpty );
    tbl_->setColumnLabel( mUseCol, tr("Actually use") );
    tbl_->setColumnStretchable( mItemCol, false );
    tbl_->setColumnStretchable( mQSResCol, true );
    tbl_->setColumnStretchable( mUseTxtCol, false );
    tbl_->setColumnStretchable( mUseCol, true );
    tbl_->setPrefWidthInChar( 120 );
    tbl_->setPrefHeightInRows( nrrows_ );
    tbl_->setTableReadOnly( true );
    tbl_->setLeftHeaderHidden( true );
    setCellTxt( mItemCol, mRevRow, tr("SEG-Y Revision") );
    setCellTxt( mUseTxtCol, mRevRow, sEmpty );
    setCellTxt( mItemCol, mDataFormatRow, tr("Data format") );
    setCellTxt( mUseTxtCol, mDataFormatRow, sEmpty );
    setCellTxt( mItemCol, mNrSamplesRow, tr("Number of samples") );
    setCellTxt( mUseTxtCol, mNrSamplesRow, sEmpty );
    setCellTxt( mItemCol, mZRangeRow, tr("Z Range") );
    setCellTxt( mUseTxtCol, mZRangeRow, tr("start / interval") );

    mkBasicInfoFlds();

    setStretch( 2, 0 );
}


#   define mAdd2Tbl(fld,row,col) \
    fld->setStretch( 2, 1 ); \
    tbl_->setCellObject( RowCol(row,col), fld );

#   define mRemoveFromTable(fld,row,col) { \
    if ( fld ) \
    { \
	tbl_->clearCellObject( RowCol(row,col) ); \
	fld = nullptr; \
    } }


void uiSEGYReadStartInfo::mkBasicInfoFlds()
{
    const CallBack revchgcb( mCB(this,uiSEGYReadStartInfo,revChg) );
    mGetParChgCB( parchgcb );

    const char* revstrs[] = { "0", "1", "2", nullptr };
    revfld_ = new uiComboBox( nullptr, revstrs, "Revision" );
    revfld_->selectionChanged.notify( revchgcb );
    mAdd2Tbl( revfld_, mRevRow, mUseCol );

    BufferStringSet fmts( SEGY::FilePars::getFmts(true) );
    nrunswappedfmts_ = (short)fmts.size();
    BufferStringSet swpdfmts( SEGY::FilePars::getFmts(false) );
    swpdfmts.removeSingle( swpdfmts.size()-1 ); // 8-bits swapped makes no sense
    swpdfmts.addToAll( " (byte swapped)" );
    fmts.add( swpdfmts, true );
    fmtfld_ = new uiComboBox( nullptr, fmts, "Format" );
    fmtfld_->selectionChanged.notify( parchgcb );
    mAdd2Tbl( fmtfld_, mDataFormatRow, mUseCol );

    nsfld_ = new uiSpinBox( nullptr, 0, "Samples" );
    nsfld_->setInterval( 1, SEGY::cMaxReasonableNrSamples(), 1 );
    nsfld_->valueChanged.notify( parchgcb );
    mAdd2Tbl( nsfld_, mNrSamplesRow, mUseCol );

    uiGroup* grp = new uiGroup( nullptr, "Z Range" );
    zstartfld_ = new uiLineEdit( grp, FloatInpSpec(0.f), "Z Start" );
    zstartfld_->setToolTip( tr("Z position of first trace sample") );
    zstartfld_->editingFinished.notify( parchgcb );
    srfld_ = new uiLineEdit( grp, FloatInpSpec(1.f,0.f), "Z Interval" );
    srfld_->setToolTip( tr("Step between samples ('sample rate')") );
    srfld_->editingFinished.notify( parchgcb );
    srfld_->attach( rightTo, zstartfld_ );
    tbl_->setCellGroup( RowCol(mZRangeRow,mUseCol), grp );
}


void uiSEGYReadStartInfo::manNonBasicRows()
{
    if ( mHaveNonBasicRows )
    {
	manCoordDefFlds();
	man2DDefFlds();
	man3DDefFlds();
	manPSDefFlds();
    }
}


void uiSEGYReadStartInfo::manCoordDefFlds()
{
    if ( isVSP() || !loaddef_.isRev0() )
    {
	mRemoveFromTable( xcoordbytefld_, mXRow, mUseCol )
	mRemoveFromTable( ycoordbytefld_, mYRow, mUseCol )
    }
    else if ( !xcoordbytefld_ )
    {
	mGetParChgCB( parchgcb );
	xcoordbytefld_ = new uiSEGYByteNr( nullptr, "X-coord byte" );
	xcoordbytefld_->changed.notify( parchgcb );
	tbl_->setCellGroup( RowCol(mXRow,mUseCol), xcoordbytefld_ );
	ycoordbytefld_ = new uiSEGYByteNr( nullptr, "Y-coord byte" );
	ycoordbytefld_->changed.notify( parchgcb );
	tbl_->setCellGroup( RowCol(mYRow,mUseCol), ycoordbytefld_ );
    }
}


#define mRemoveRefNrByteFld() \
    mRemoveFromTable( refnrbytefld_, mKey2Row, mUseCol )
#define mRemoveTrcNrByteFld() \
    mRemoveFromTable( trcnrbytefld_, mKey1Row, mUseCol )
#define mRemoveTrcNrGenGrp() { \
    mRemoveFromTable( trcnrgengrp_, mKey1Row, mUseCol ); \
    trcnrgenstartfld_ = trcnrgenstepfld_ = nullptr; }


void uiSEGYReadStartInfo::remove2DDefFlds()
{
    mRemoveFromTable( trcnrsrcfld_, mKey1Row, mUseTxtCol );
    mRemoveTrcNrByteFld();
    mRemoveTrcNrGenGrp();
    mRemoveRefNrByteFld();
}


void uiSEGYReadStartInfo::remove3DDefFlds()
{
    mRemoveFromTable( inlbytefld_, mKey1Row, mUseCol )
    mRemoveFromTable( crlbytefld_, mKey2Row, mUseCol )
}


void uiSEGYReadStartInfo::man2DDefFlds()
{
    mGetGeomType( gt );

    if ( isVSP() || !Seis::is2D(gt) )
	{ remove2DDefFlds(); return; }

    remove3DDefFlds();
    mGetParChgCB( parchgcb );

    if ( !loaddef_.isRev0() )
	mRemoveRefNrByteFld()
    else if ( !refnrbytefld_ )
    {
	refnrbytefld_ = new uiSEGYByteNr( nullptr, "Ref/SP number byte" );
	refnrbytefld_->changed.notify( parchgcb );
	tbl_->setCellGroup( RowCol(mKey2Row,mUseCol), refnrbytefld_ );
    }

    if ( !trcnrsrcfld_ )
    {
	trcnrsrcfld_ = new uiComboBox( nullptr, "Trace number source" );
	trcnrsrcfld_->addItem( tr("In file") );
	trcnrsrcfld_->addItem( tr("Generate") );
	trcnrsrcfld_->selectionChanged.notify( parchgcb );
	mAdd2Tbl( trcnrsrcfld_, mKey1Row, mUseTxtCol );
    }

    if ( loaddef_.havetrcnrs_ )
    {
	mRemoveTrcNrGenGrp();
	if ( !trcnrbytefld_ )
	{
	    trcnrbytefld_ = new uiSEGYByteNr( nullptr, "Trace number byte" );
	    trcnrbytefld_->changed.notify( parchgcb );
	    tbl_->setCellGroup( RowCol(mKey1Row,mUseCol), trcnrbytefld_ );
	}
    }
    else
    {
	mRemoveTrcNrByteFld();
	if ( !trcnrgengrp_ )
	{
	    trcnrgengrp_ = new uiGroup( nullptr, "Trace number Generation" );
	    trcnrgenstartfld_ = new uiLineEdit( trcnrgengrp_,
		    IntInpSpec(1000), "Trace Number Start" );
	    trcnrgenstartfld_->setToolTip( tr("%1 start value")
					   .arg( uiStrings::sTraceNumber() ) );
	    trcnrgenstartfld_->editingFinished.notify( parchgcb );
	    trcnrgenstartfld_->setStretch( 2, 1 );
	    trcnrgenstepfld_ = new uiLineEdit( trcnrgengrp_,
		    IntInpSpec(1), "Trace number Step" );
	    trcnrgenstepfld_->setToolTip( tr("Step in trace numbers") );
	    trcnrgenstepfld_->editingFinished.notify( parchgcb );
	    trcnrgenstepfld_->setStretch( 2, 1 );
	    trcnrgenstepfld_->attach( rightOf, trcnrgenstartfld_ );
	    tbl_->setCellGroup( RowCol(mKey1Row,mUseCol), trcnrgengrp_ );
	}
    }
}


void uiSEGYReadStartInfo::man3DDefFlds()
{
    mGetGeomType( gt );

    if ( isVSP() || Seis::is2D(gt) || !loaddef_.isRev0() )
	{ remove3DDefFlds(); return; }

    remove2DDefFlds();
    mGetParChgCB( parchgcb );

    if ( !inlbytefld_ )
    {
	inlbytefld_ = new uiSEGYByteNr( nullptr, "Inline byte" );
	inlbytefld_->changed.notify( parchgcb );
	tbl_->setCellGroup( RowCol(mKey1Row,mUseCol), inlbytefld_ );
	crlbytefld_ = new uiSEGYByteNr( nullptr, "Crossline byte" );
	crlbytefld_->changed.notify( parchgcb );
	tbl_->setCellGroup( RowCol(mKey2Row,mUseCol), crlbytefld_ );
    }
}


void uiSEGYReadStartInfo::manPSDefFlds()
{
    if ( !mHavePSRow )
	return;

    mGetGeomType( gt );
    mGetParChgCB( parchgcb );

#define mRemoveOffsByteFld() \
	mRemoveFromTable( offsetbytefld_, mPSRow, mUseCol )
#define mRemoveOffsGenGrp() \
	mRemoveFromTable( offsgengrp_, mPSRow, mUseCol ); \
	offsgenstartfld_ = offsgenstepfld_ = nullptr

    if ( isVSP() || !Seis::isPS(gt) )
    {
	mRemoveFromTable( psoffsrcfld_, mPSRow, mUseTxtCol );
	mRemoveOffsByteFld();
	mRemoveOffsGenGrp();
    }
    else
    {
	if ( !psoffsrcfld_ )
	{
	    psoffsrcfld_ = new uiComboBox( nullptr, "Offset source" );
	    psoffsrcfld_->addItem( tr("In file") );
	    psoffsrcfld_->addItem( tr("From Src/Rcv (X,Y)") );
	    psoffsrcfld_->addItem( tr("Generate") );
	    psoffsrcfld_->selectionChanged.notify( parchgcb );
	    mAdd2Tbl( psoffsrcfld_, mPSRow, mUseTxtCol );
	}
	switch ( loaddef_.psoffssrc_ )
	{
	    case SEGY::FileReadOpts::InFile:
	    {
		mRemoveOffsGenGrp();
		if ( !offsetbytefld_ )
		{
		    offsetbytefld_ = new uiSEGYByteNr( nullptr, "Offset byte" );
		    offsetbytefld_->changed.notify( parchgcb );
		    tbl_->setCellGroup( RowCol(mPSRow,mUseCol), offsetbytefld_);
		}
	    }
	    break;
	    case SEGY::FileReadOpts::UsrDef:
	    {
		mRemoveOffsByteFld();
		if ( !offsgengrp_ )
		{
		    offsgengrp_ = new uiGroup( nullptr, "Offset Generation" );
		    offsgenstartfld_ = new uiLineEdit( offsgengrp_,
			    FloatInpSpec(0.f), "Offset Start" );
		    offsgenstartfld_->setToolTip( tr("Offset start value") );
		    offsgenstartfld_->editingFinished.notify( parchgcb );
		    offsgenstartfld_->setStretch( 2, 1 );
		    offsgenstepfld_ = new uiLineEdit( offsgengrp_,
			    FloatInpSpec(100.f,0.f), "Offset Step" );
		    offsgenstepfld_->setToolTip( tr("Step in offset values") );
		    offsgenstepfld_->editingFinished.notify( parchgcb );
		    offsgenstepfld_->setStretch( 2, 1 );
		    offsgenstepfld_->attach( rightOf, offsgenstartfld_ );
		    tbl_->setCellGroup( RowCol(mPSRow,mUseCol), offsgengrp_ );
		}
	    }
	    break;
	    case SEGY::FileReadOpts::SrcRcvCoords:
	    {
		mRemoveOffsByteFld();
		mRemoveOffsGenGrp();
	    }
	    break;
	}
    }
}


#define mJoinUiStrRange( txt )\
    uiStrings::phrJoinStrings( uiStrings::txt, rangestr )

void uiSEGYReadStartInfo::updateCellTexts()
{
    mGetGeomType( gt );
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    const bool isvsp = isVSP();

    uiString xittxt; uiString yittxt;
    uiString ky1ittxt; uiString ky2ittxt; uiString offsittxt;
    uiString xustxt; uiString yustxt;
    uiString ky1ustxt; uiString ky2ustxt;
    uiString nrtrcsusrtxt;
    xittxt = yittxt = ky1ittxt = ky2ittxt = offsittxt =
    xustxt = yustxt = ky1ustxt = ky2ustxt = nrtrcsusrtxt = sEmpty;
    if ( isvsp )
	nrtrcsusrtxt = tr("(1 trace used)");
    else
    {
	const uiString rangestr = tr("range");
	xittxt = mJoinUiStrRange( sXcoordinate() );
	yittxt = mJoinUiStrRange( sYcoordinate() );
	ky1ittxt = is2d ? mJoinUiStrRange( sTraceNumber() )
			: mJoinUiStrRange( sInline() );
	ky2ittxt = is2d ? mJoinUiStrRange( sSPNumber() )
			: mJoinUiStrRange( sCrossline() );
	if ( isps )
	    offsittxt = mJoinUiStrRange( sOffset() );

	if ( loaddef_.isRev0() )
	    xustxt = yustxt = ky1ustxt = ky2ustxt = sBytePos;

	const uiString iscalcstr = tr( "[Calculated]" );
	if ( is2d )
	    ky1ustxt = sBytePos;
	else
	{
	    if ( loaddef_.icvsxytype_ == SEGY::FileReadOpts::ICOnly )
		xustxt = yustxt = iscalcstr;
	    else if ( loaddef_.icvsxytype_ == SEGY::FileReadOpts::XYOnly )
		ky1ustxt = ky2ustxt = iscalcstr;
	}
    }

    setCellTxt( mItemCol, mXRow, xittxt );
    setCellTxt( mItemCol, mYRow, yittxt );
    setCellTxt( mItemCol, mKey1Row, ky1ittxt );
    setCellTxt( mItemCol, mKey2Row, ky2ittxt );
    if ( mHavePSRow )
	setCellTxt( mItemCol, mPSRow, offsittxt );
    setCellTxt( mQSResCol, mKey1Row, isvsp ? sEmpty
		: (is2d ? trcnrinfotxt_ : inlinfotxt_) );
    setCellTxt( mQSResCol, mKey2Row, isvsp ? sEmpty
		: (is2d ? refnrinfotxt_ : crlinfotxt_) );
    setCellTxt( mQSResCol, mXRow, isvsp ? sEmpty : xinfotxt_ );
    setCellTxt( mQSResCol, mYRow, isvsp ? sEmpty : yinfotxt_ );
    setCellTxt( mQSResCol, mPSRow, !isvsp && isps ? offsetinfotxt_ : sEmpty );
    setCellTxt( mUseTxtCol, mNrSamplesRow, nrtrcsusrtxt );
    setCellTxt( mUseTxtCol, mKey2Row, ky2ustxt );
    setCellTxt( mUseTxtCol, mXRow, xustxt );
    setCellTxt( mUseTxtCol, mYRow, yustxt );
    if ( !is2d )
	setCellTxt( mUseTxtCol, mKey1Row, ky1ustxt );
}


void uiSEGYReadStartInfo::showZSamplingSetting( bool yn )
{
    nsfld_->display( yn );
    zstartfld_->display( yn );
    srfld_->display( yn );
}


void uiSEGYReadStartInfo::setByteFldContents( const SEGY::HdrEntryKeyData& hkd )
{
#define mSetBFCont(nm,cont) \
    if ( nm##bytefld_ ) \
	nm##bytefld_->setEntries( cont )

    mSetBFCont( xcoord, hkd.x_ );
    mSetBFCont( ycoord, hkd.y_ );
    mSetBFCont( inl, hkd.inl_ );
    mSetBFCont( crl, hkd.crl_ );
    mSetBFCont( trcnr, hkd.trcnr_ );
    mSetBFCont( refnr, hkd.refnr_ );
    mSetBFCont( offset, hkd.offs_ );
}


void uiSEGYReadStartInfo::setCellTxt( int col, int row, const uiString& txt )
{
    const RowCol rc( row, col );
    tbl_->setText( rc, txt );
    if ( col == mItemCol || col == mUseTxtCol )
	tbl_->resizeColumnToContents( col );
    tbl_->setColor( rc, col != mQSResCol || txt.isEmpty()
			? Color::White() : qscellcolor );
}


void uiSEGYReadStartInfo::revChg( CallBacker* )
{
    parChanged( true );
}


void uiSEGYReadStartInfo::parChg( CallBacker* )
{
    parChanged( false );
}


void uiSEGYReadStartInfo::parChanged( bool revchg )
{
    if ( parsbeingset_ )
	return;

    fillLoadDef();

    if ( revchg )
	revChanged.trigger();
    else
	loaddefChanged.trigger();
}


void uiSEGYReadStartInfo::setImpTypIdx( int idx, bool doupd )
{
    if ( idx != imptype_.tidx_ )
    {
	if ( inptypfixed_ )
	    { pErrMsg( "Input type fixed, should not set" ); }
	imptype_.tidx_ = idx;
    }

    if ( doupd )
	parChanged( false );
}


void uiSEGYReadStartInfo::clearInfo()
{
    xinfotxt_.setEmpty(); yinfotxt_.setEmpty();
    inlinfotxt_.setEmpty(); crlinfotxt_.setEmpty();
    trcnrinfotxt_.setEmpty(); refnrinfotxt_.setEmpty();
    offsetinfotxt_.setEmpty();
    for ( int irow=0; irow<mKey1Row; irow++ )
	setCellTxt( mQSResCol, irow, sEmpty );

    updateCellTexts();
}


void uiSEGYReadStartInfo::setScanInfo( const SEGY::ScanInfoSet& sis )
{
    tbl_->setColumnLabel( mQSResCol, sis.isFull() ? tr("Full scan result")
						  : tr("Quick scan result") );

    const int nrfiles = sis.size();
    uiString txt = nrfiles < 1	? uiString::emptyString()
		: (nrfiles < 2	? tr( "[1 file]")
				: tr( "[%1 files]" ).arg( nrfiles ));
    tbl_->setTopLeftCornerLabel( txt );

    if ( nrfiles < 1 )
	{ clearInfo(); return; }

    manNonBasicRows();
    setByteFldContents( sis.keyData() );
    useLoadDef();
    setScanInfoTexts( sis );
    updateCellTexts();
}


void uiSEGYReadStartInfo::setScanInfoTexts( const SEGY::ScanInfoSet& sis )
{
    uiString txt;
    const SEGY::BasicFileInfo& bi = sis.basicInfo();

    txt.set( "%1" );
    txt.arg( bi.revision_ );
    setCellTxt( mQSResCol, mRevRow, txt );

    const char** fmts = SEGY::FilePars::getFmts(false);
    txt.set( "%1%2" );
    txt.arg( bi.format_ < 4 ? fmts[bi.format_-1]
			    : (bi.format_==8 ? fmts[4] : fmts[3]) );
    if ( bi.hdrsswapped_ && bi.dataswapped_ )
	txt.arg( tr(" (all bytes swapped)") );
    else if ( bi.hdrsswapped_ )
	txt.arg( tr(" (header bytes swapped)") );
    else if ( bi.dataswapped_ )
	txt.arg( tr(" (data bytes swapped)") );
    else
	txt.arg( sEmpty  );
    setCellTxt( mQSResCol, mDataFormatRow, txt );

    txt.set( "%1 (%2 %3)" );
    const int nrtrcs = sis.nrTraces();
    txt.arg( bi.ns_ ).arg( nrtrcs ).arg( nrtrcs == 1?tr("trace"):tr("traces") );
    setCellTxt( mQSResCol, mNrSamplesRow, txt );

    if ( mIsUdf(bi.sampling_.step) )
	txt = sEmpty;
    else
    {
	txt.set( "%1 - %2 (s or %3)" );
	const float endz = loaddef_.sampling_.start
			 + (bi.ns_-1) * loaddef_.sampling_.step;
	txt.arg( loaddef_.sampling_.start ).arg( endz )
		 .arg( sis.inFeet() ? tr("ft") : tr("m") );
    }
    setCellTxt( mQSResCol, mZRangeRow, txt );

    const SEGY::ScanRangeInfo& rgs = sis.ranges();
    const char* rgstr = "%1 - %2";
    inlinfotxt_.set( rgstr ).arg( rgs.inls_.start ).arg( rgs.inls_.stop );
    crlinfotxt_.set( rgstr ).arg( rgs.crls_.start ).arg( rgs.crls_.stop );
    trcnrinfotxt_.set( rgstr ).arg( rgs.trcnrs_.start ).arg( rgs.trcnrs_.stop );
    xinfotxt_.set( rgstr ).arg( rgs.xrg_.start ).arg( rgs.xrg_.stop );
    yinfotxt_.set( rgstr ).arg( rgs.yrg_.start ).arg( rgs.yrg_.stop );
    offsetinfotxt_.set( rgstr ).arg( rgs.offs_.start ).arg( rgs.offs_.stop );
    if ( mIsUdf(rgs.refnrs_.start) )
	refnrinfotxt_ =  tr( "<no data>" );
    else
	refnrinfotxt_.set(rgstr).arg( rgs.refnrs_.start )
				.arg( rgs.refnrs_.stop );
}


void uiSEGYReadStartInfo::useLoadDef()
{
    parsbeingset_ = true;

    revfld_->setCurrentItem( loaddef_.revision_ );

    if ( loaddef_.useformatinfile_ || loaddef_.format_ == 0 )
	fmtfld_->setCurrentItem( 0 );
    else
    {
	const char** fmts = SEGY::FilePars::getFmts(false);
	const char* fmt = *fmts;
	for ( short idx=0; fmt; idx++ )
	{
	    fmt = fmts[idx];
	    if ( !fmt )
		{ pErrMsg("Format not found"); break; }

	    if ( (short)(*fmt - '0') == loaddef_.format_ )
	    {
		int listidx = idx + 1;
		if ( loaddef_.dataswapped_ )
		{
		    listidx += nrunswappedfmts_;
		    if ( listidx >= fmtfld_->size() )
			{ pErrMsg("Huh"); listidx -= nrunswappedfmts_; }
		}
		fmtfld_->setCurrentItem( listidx );
		break;
	    }
	}
    }

    nsfld_->setValue( loaddef_.ns_ );
    zstartfld_->setValue( loaddef_.sampling_.start );
    srfld_->setValue( loaddef_.sampling_.step );
    if ( imptype_.isVSP() )
	{ parsbeingset_ = false; return; }

#   define mSetToByteNr(fld,memb) \
    if ( fld ) fld->setHdrEntry( loaddef_.hdrdef_->memb )

    mSetToByteNr( xcoordbytefld_, xcoord_  );
    mSetToByteNr( ycoordbytefld_, ycoord_  );

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
    {
	trcnrsrcfld_->setCurrentItem( loaddef_.havetrcnrs_ ? 0 : 1 );
	mSetToByteNr( refnrbytefld_, refnr_  );
	mSetToByteNr( trcnrbytefld_, trnr_  );
	if ( trcnrgengrp_ )
	{
	    trcnrgenstartfld_->setValue( loaddef_.trcnrdef_.start );
	    trcnrgenstepfld_->setValue( loaddef_.trcnrdef_.step );
	}
    }
    else
    {
	mSetToByteNr( inlbytefld_, inl_  );
	mSetToByteNr( crlbytefld_, crl_  );

	if ( xcoordbytefld_ )
	{
	    const bool isic = loaddef_.icvsxytype_==SEGY::FileReadOpts::ICOnly;
	    const bool isxy = loaddef_.icvsxytype_==SEGY::FileReadOpts::XYOnly;
	    xcoordbytefld_->setSensitive( !isic );
	    ycoordbytefld_->setSensitive( !isic );
	    inlbytefld_->setSensitive( !isxy );
	    crlbytefld_->setSensitive( !isxy );
	}
    }

    if ( Seis::isPS(gt) )
    {
	psoffsrcfld_->setCurrentItem( (int)loaddef_.psoffssrc_ );
	mSetToByteNr( offsetbytefld_, offs_  );
	if ( offsgengrp_ )
	{
	    offsgenstartfld_->setValue( loaddef_.psoffsdef_.start );
	    offsgenstepfld_->setValue( loaddef_.psoffsdef_.step );
	}
    }

    parsbeingset_ = false;
}


void uiSEGYReadStartInfo::fillLoadDef()
{
    loaddef_.revision_ = revfld_->currentItem();

    const int fmtidx = fmtfld_->currentItem();
    loaddef_.dataswapped_ = fmtidx > nrunswappedfmts_;
    if ( fmtidx > 0 )
	loaddef_.format_ = (short)(*fmtfld_->text() - '0');
    else
    {
	loaddef_.format_ = 0;
	loaddef_.useformatinfile_ = true;
    }

    int newns = nsfld_->getIntValue();
    if ( newns > 0 )
	loaddef_.ns_ = newns;

    SamplingData<float> sampling = loaddef_.sampling_;
    sampling.start = zstartfld_->getFValue();
    if ( !mIsUdf(sampling.start) )
	loaddef_.sampling_.start = sampling.start;
    sampling.step = srfld_->getFValue();
    if ( !mIsUdf(sampling.step) && sampling.step != 0.f )
	loaddef_.sampling_.step = sampling.step;

    if ( imptype_.isVSP() )
	return;

    const Seis::GeomType gt = imptype_.geomType();
#   define mSetByteNr(fld,memb) \
    if ( fld ) loaddef_.hdrdef_->memb = fld->hdrEntry()

    mSetByteNr( xcoordbytefld_, xcoord_  );
    mSetByteNr( ycoordbytefld_, ycoord_  );

    if ( Seis::is2D(gt) )
    {
	mSetByteNr( refnrbytefld_, refnr_  );
	mSetByteNr( trcnrbytefld_, trnr_  );
	loaddef_.havetrcnrs_ = trcnrsrcfld_ && trcnrsrcfld_->currentItem() == 0;
	if ( trcnrgengrp_ )
	{
	    SamplingData<int>& def = loaddef_.trcnrdef_;
	    def.start = trcnrgenstartfld_->getIntValue();
	    if ( mIsUdf(def.start) ) def.start = 1;
	    def.step = trcnrgenstepfld_->getIntValue();
	    if ( mIsUdf(def.step) || def.step == 0 ) def.step = 1;
	}
    }
    else
    {
	mSetByteNr( inlbytefld_, inl_  );
	mSetByteNr( crlbytefld_, crl_  );
    }

    if ( Seis::isPS(gt) )
    {
	loaddef_.psoffssrc_ = (SEGY::FileReadOpts::PSDefType)
				    psoffsrcfld_->currentItem();
	mSetByteNr( offsetbytefld_, offs_  );
	if ( offsgengrp_ )
	{
	    SamplingData<float>& def = loaddef_.psoffsdef_;
	    def.start = offsgenstartfld_->getFValue();
	    if ( mIsUdf(def.start) ) def.start = 0.f;
	    def.step = offsgenstepfld_->getFValue();
	    if ( mIsUdf(def.step) ) def.step = 1.f;
	}
    }
}
