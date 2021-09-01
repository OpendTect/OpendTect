/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril / Nanne Hemstra
 Date:		July 2015
________________________________________________________________________

-*/

#include "uisegyreadstartinfo.h"

#include "hiddenparam.h"
#include "segyhdr.h"
#include "segyhdrkeydata.h"
#include "segyuiscandata.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uisegydef.h"
#include "uisegyimptype.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"

#define mNrInfoRows 10
#define mNrInfoCols 4

#define mRevRow 0
#define mDataFormatRow 1
#define mNrSamplesRow 2
#define mZRangeRow 3
#define mKey1Row 4
#define mKey2Row 5
#define mXRow 6
#define mYRow 7
#define mOffsetRow 8
#define mAzimuthRow 9
#define mHaveNonBasicRows (nrrows_ > mKey1Row)
#define mHavePSRow (nrrows_ > mOffsetRow)

#define mItemCol 0
#define mQSResCol 1
#define mUseTxtCol 2
#define mUseCol 3

// palish yellow
static const Color qscellcolor( 255, 255, 225 );
// even paler yellow
static const Color optqscellcolor( 255, 255, 245 );
// very light grey
static const Color optcellcolor( 245, 245, 245 );

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



static HiddenParam<uiSEGYReadStartInfo,SEGY::LoadDef*> hp_loaddefcache(nullptr);

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
    , azimuthbytefld_(nullptr)
    , offsgengrp_(nullptr)
    , inptypfixed_(imptyp)
    , nrunswappedfmts_(5)
    , sBytePos(tr("From header"))
{
    hp_loaddefcache.setParam( this, new SEGY::LoadDef(loaddef_) );

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
    tbl_->setColumnLabel( mItemCol, uiString::empty() );
    tbl_->setColumnLabel( mQSResCol, tr("Quick scan result") );
    tbl_->setColumnLabel( mUseTxtCol, uiStrings::sSource() );
    tbl_->setColumnLabel( mUseCol, tr("Actually use") );
    tbl_->setColumnStretchable( mItemCol, false );
    tbl_->setColumnStretchable( mQSResCol, true );
    tbl_->setColumnStretchable( mUseTxtCol, false );
    tbl_->setColumnStretchable( mUseCol, true );
    tbl_->setPrefWidthInChar( 120 );
    tbl_->setPrefHeightInRows( nrrows_ );
    tbl_->setTableReadOnly( true );
    tbl_->setLeftHeaderHidden( true );
    setCellTxt( mItemCol, mRevRow, tr("SEG-Y revision") );
    setCellTxt( mUseTxtCol, mRevRow, uiString::empty() );
    setCellTxt( mItemCol, mDataFormatRow, tr("Data format") );
    setCellTxt( mUseTxtCol, mDataFormatRow, uiString::empty() );
    setCellTxt( mItemCol, mNrSamplesRow, tr("Number of samples") );
    setCellTxt( mItemCol, mZRangeRow, tr("Z range") );

    mkBasicInfoFlds();

    setStretch( 2, 0 );
}


uiSEGYReadStartInfo::~uiSEGYReadStartInfo()
{
    hp_loaddefcache.removeAndDeleteParam( this );
}


void uiSEGYReadStartInfo::mkBasicInfoFlds()
{
    const CallBack revchgcb( mCB(this,uiSEGYReadStartInfo,revChg) );
    const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );

    const char* revstrs[] =
		{ "User defined", "Revision 1", "Revision 2", nullptr };
    revfld_ = new uiComboBox( nullptr, revstrs, "Revision" );
    revfld_->selectionChanged.notify( revchgcb );
    tbl_->setCellObject( RowCol(mRevRow,mUseCol), revfld_ );

    BufferStringSet fmts( SEGY::FilePars::getFmts(true) );
    nrunswappedfmts_ = (short)(fmts.size() - 1); // not the 'From file header'
    BufferStringSet swpdfmts( SEGY::FilePars::getFmts(false) );
    swpdfmts.removeSingle( swpdfmts.size()-1 ); // 8-bits swapped makes no sense
    swpdfmts.addToAll( " (byte swapped)" );
    fmts.add( swpdfmts, true );
    fmtfld_ = new uiComboBox( nullptr, fmts, "Format" );
    fmtfld_->selectionChanged.notify( parchgcb );
    tbl_->setCellObject( RowCol(mDataFormatRow,mUseCol), fmtfld_ );

    const uiString fromhdrstr = tr("From header");
    const uiString usrdefstr = tr("User defined");
    nrsampsrcfld_ = new uiComboBox( nullptr, "Nr samples source" );
    nrsampsrcfld_->addItem( fromhdrstr );
    nrsampsrcfld_->addItem( usrdefstr );
    nrsampsrcfld_->selectionChanged.notify( parchgcb );
    tbl_->setCellObject( RowCol(mNrSamplesRow,mUseTxtCol), nrsampsrcfld_ );

    zsampsrcfld_ = new uiComboBox( nullptr, "Z sampling source" );
    zsampsrcfld_->addItem( fromhdrstr );
    zsampsrcfld_->addItem( usrdefstr );
    zsampsrcfld_->selectionChanged.notify( parchgcb );
    tbl_->setCellObject( RowCol(mZRangeRow,mUseTxtCol), zsampsrcfld_ );

    nsfld_ = new uiSpinBox( nullptr, 0, "Samples" );
    nsfld_->setInterval( 1, INT32_MAX );
    nsfld_->valueChanging.notify( parchgcb );
    tbl_->setCellObject( RowCol(mNrSamplesRow,mUseCol), nsfld_ );

    auto* grp = new uiGroup( nullptr, "Z Range" );
    auto lbl = new uiLabel( grp, uiStrings::sStart() );
    zstartfld_ = new uiLineEdit( grp, FloatInpSpec(0.f), "Z Start" );
    zstartfld_->setToolTip( tr("Z position of first trace sample") );
    zstartfld_->editingFinished.notify( parchgcb );
    zstartfld_->setHSzPol( uiObject::MedVar );
    zstartfld_->attach( rightTo, lbl );
    lbl = new uiLabel( grp, uiStrings::sStep() );
    lbl->attach( rightTo, zstartfld_ );
    srfld_ = new uiLineEdit( grp, FloatInpSpec(1.f,0.f), "Z Interval" );
    srfld_->setToolTip( tr("Step between samples ('sample rate')") );
    srfld_->editingFinished.notify( parchgcb );
    srfld_->setHSzPol( uiObject::MedVar );
    srfld_->attach( rightTo, lbl );
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


void uiSEGYReadStartInfo::clearTable()
{
    remove2DDefFlds();
    remove3DDefFlds();

    removeFromTable( xcoordbytefld_ ); xcoordbytefld_ = nullptr;
    removeFromTable( ycoordbytefld_ ); ycoordbytefld_ = nullptr;

    removeFromTable( psoffsrcfld_ ); psoffsrcfld_ = nullptr;
    removeFromTable( offsgengrp_ ); offsgengrp_ = nullptr;
    offsgenstartfld_ = offsgenstepfld_ = nullptr;
    offsetbytefld_ = nullptr;
    removeFromTable( azimuthbytefld_ ); azimuthbytefld_ = nullptr;
}


void uiSEGYReadStartInfo::remove2DDefFlds()
{
    removeFromTable( refnrbytefld_ ); refnrbytefld_ = nullptr;
    removeFromTable( trcnrsrcfld_ ); trcnrsrcfld_ = nullptr;
    removeFromTable( trcnrgengrp_ ); trcnrgengrp_ = nullptr;
    trcnrgenstartfld_ = trcnrgenstepfld_ = nullptr;
    trcnrbytefld_ = nullptr;
}


void uiSEGYReadStartInfo::remove3DDefFlds()
{
    removeFromTable( inlbytefld_ ); inlbytefld_ = nullptr;
    removeFromTable( crlbytefld_ ); crlbytefld_ = nullptr;
}


void uiSEGYReadStartInfo::removeFromTable( uiObject* obj )
{
    if ( obj )
	tbl_->clearCellObject( tbl_->getCell(obj) );
}


void uiSEGYReadStartInfo::removeFromTable( uiGroup* obj )
{
    if ( obj )
	tbl_->clearCellObject( tbl_->getCell(obj) );
}


void uiSEGYReadStartInfo::manCoordDefFlds()
{
    if ( xcoordbytefld_ || isVSP() )
	return;

    const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );
    xcoordbytefld_ = new uiSEGYByteNr( nullptr, "X-coord byte" );
    xcoordbytefld_->changed.notify( parchgcb );
    tbl_->setCellGroup( RowCol(mXRow,mUseCol), xcoordbytefld_ );
    ycoordbytefld_ = new uiSEGYByteNr( nullptr, "Y-coord byte" );
    ycoordbytefld_->changed.notify( parchgcb );
    tbl_->setCellGroup( RowCol(mYRow,mUseCol), ycoordbytefld_ );
}


void uiSEGYReadStartInfo::man2DDefFlds()
{
    const Seis::GeomType gt = imptype_.geomType();
    if ( isVSP() || !Seis::is2D(gt) )
	return;

    const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );
    if ( !refnrbytefld_ )
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
	tbl_->setCellObject( RowCol(mKey1Row,mUseTxtCol), trcnrsrcfld_ );
    }

    if ( !trcnrgengrp_ )
    {
	trcnrgengrp_ = new uiGroup( nullptr, "Trace number Generation" );

	trcnrbytefld_ = new uiSEGYByteNr( trcnrgengrp_, "Trace number byte" );
	trcnrbytefld_->changed.notify( parchgcb );

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

    trcnrbytefld_->display( loaddef_.havetrcnrs_ );
    trcnrgenstartfld_->display( !loaddef_.havetrcnrs_ );
    trcnrgenstepfld_->display( !loaddef_.havetrcnrs_ );
}


void uiSEGYReadStartInfo::man3DDefFlds()
{
    const Seis::GeomType gt = imptype_.geomType();
    if ( isVSP() || Seis::is2D(gt) )
	return;

    if ( !inlbytefld_ )
    {
	const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );
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

    const Seis::GeomType gt = imptype_.geomType();
    if ( isVSP() || !Seis::isPS(gt) )
	return;

    const CallBack parchgcb( mCB(this,uiSEGYReadStartInfo,parChg) );
    if ( !psoffsrcfld_ )
    {
	psoffsrcfld_ = new uiComboBox( nullptr, "Offset source" );
	psoffsrcfld_->addItem( tr("In file") );
	psoffsrcfld_->addItem( tr("From Src/Rcv (X,Y)") );
	psoffsrcfld_->addItem( tr("Generate") );
	psoffsrcfld_->selectionChanged.notify( parchgcb );
	tbl_->setCellObject( RowCol(mOffsetRow,mUseTxtCol), psoffsrcfld_ );
    }

    if ( !offsgengrp_ )
    {
	offsgengrp_ = new uiGroup( nullptr, "Offset Generation" );

	offsetbytefld_ = new uiSEGYByteNr( offsgengrp_, "Offset byte" );
	offsetbytefld_->changed.notify( parchgcb );

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

	tbl_->setCellGroup( RowCol(mOffsetRow,mUseCol), offsgengrp_ );
    }

    switch ( loaddef_.psoffssrc_ )
    {
	case SEGY::FileReadOpts::InFile:
	{
	    offsetbytefld_->display( true );
	    offsgenstartfld_->display( false );
	    offsgenstepfld_->display( false );
	}
	break;
	case SEGY::FileReadOpts::UsrDef:
	{
	    offsetbytefld_->display( false );
	    offsgenstartfld_->display( true );
	    offsgenstepfld_->display( true );
	}
	break;
	case SEGY::FileReadOpts::SrcRcvCoords:
	{
	    offsgenstartfld_->display( false );
	    offsgenstepfld_->display( false );
	    offsetbytefld_->display( false );
	}
	break;
    }

    if ( !azimuthbytefld_ )
    {
	azimuthbytefld_ = new uiSEGYByteNr( nullptr, "Azimuth byte" );
	azimuthbytefld_->changed.notify( parchgcb );
	tbl_->setCellGroup( RowCol(mAzimuthRow,mUseCol), azimuthbytefld_);
    }
}


#define mJoinUiStrRange( txt )\
    uiStrings::phrJoinStrings( uiStrings::txt, rangestr )

void uiSEGYReadStartInfo::updateCellTexts()
{
    const Seis::GeomType gt = imptype_.geomType();
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    const bool isvsp = isVSP();

    uiString xittxt; uiString yittxt;
    uiString ky1ittxt; uiString ky2ittxt;
    uiString offsittxt; uiString azitxt;
    uiString xustxt; uiString yustxt;
    uiString ky1ustxt; uiString ky2ustxt;
    uiString nrtrcsusrtxt;
    xittxt = yittxt = ky1ittxt = ky2ittxt = offsittxt = azitxt =
    xustxt = yustxt = ky1ustxt = ky2ustxt = nrtrcsusrtxt = uiString::empty();
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
	{
	    offsittxt = mJoinUiStrRange( sOffset() );
	    azitxt = mJoinUiStrRange( sAzimuth() );
	}

	xustxt = yustxt = ky1ustxt = ky2ustxt = sBytePos;

	const uiString iscalcstr = tr( "Calculated" );
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
    {
	setCellTxt( mItemCol, mOffsetRow, offsittxt );
	setCellTxt( mItemCol, mAzimuthRow, azitxt );
    }

    setCellTxt( mQSResCol, mKey1Row, isvsp ? uiString::empty()
		: (is2d ? trcnrinfotxt_ : inlinfotxt_) );
    setCellTxt( mQSResCol, mKey2Row, isvsp ? uiString::empty()
		: (is2d ? refnrinfotxt_ : crlinfotxt_) );
    setCellTxt( mQSResCol, mXRow, isvsp ? uiString::empty() : xinfotxt_ );
    setCellTxt( mQSResCol, mYRow, isvsp ? uiString::empty() : yinfotxt_ );
    setCellTxt( mQSResCol, mOffsetRow,
		!isvsp && isps ? offsetinfotxt_ : uiString::empty() );
    setCellTxt( mQSResCol, mAzimuthRow,
		!isvsp && isps ? azimuthinfotxt_ : uiString::empty() );
    setCellTxt( mUseTxtCol, mKey2Row, ky2ustxt );
    setCellTxt( mUseTxtCol, mXRow, xustxt );
    setCellTxt( mUseTxtCol, mYRow, yustxt );
    if ( !is2d )
	setCellTxt( mUseTxtCol, mKey1Row, ky1ustxt );
    setCellTxt( mUseTxtCol, mAzimuthRow, isps ? sBytePos : uiString::empty() );
}


void uiSEGYReadStartInfo::showNrSamplesSetting( bool yn )
{
    nsfld_->setSensitive( yn );
}


void uiSEGYReadStartInfo::showZSamplingSetting( bool yn )
{
    uiGroup* zsampgrp = tbl_->getCellGroup( RowCol(mZRangeRow,mUseCol) );
    if ( zsampgrp )
	zsampgrp->setSensitive( yn );
}


static void setEntries( uiSEGYByteNr* fld, const SEGY::HdrEntryDataSet& heds,
			int rev1entry )
{
    if ( !fld )
	return;

    SEGY::HdrEntryDataSet& entries = const_cast<SEGY::HdrEntryDataSet&>(heds);
    if ( rev1entry>=0 && !entries.idxs_.isPresent(rev1entry) )
    {
	entries.idxs_.add( rev1entry );
	sort( entries.idxs_ );
	entries.rejectedidxs_ -= rev1entry;
    }

    fld->setEntries( entries );
}


void uiSEGYReadStartInfo::setByteFldContents( const SEGY::HdrEntryKeyData& hkd )
{
    setEntries( xcoordbytefld_, hkd.x_, SEGY::TrcHeader::EntryXcdp() );
    setEntries( ycoordbytefld_, hkd.y_, SEGY::TrcHeader::EntryYcdp() );
    setEntries( inlbytefld_, hkd.inl_, SEGY::TrcHeader::EntryInline() );
    setEntries( crlbytefld_, hkd.crl_, SEGY::TrcHeader::EntryCrossline() );
    setEntries( trcnrbytefld_, hkd.trcnr_, SEGY::TrcHeader::EntryCdp() );
    setEntries( refnrbytefld_, hkd.refnr_, SEGY::TrcHeader::EntrySP() );
    setEntries( offsetbytefld_, hkd.offs_, SEGY::TrcHeader::EntryOffset() );
    setEntries( azimuthbytefld_, hkd.azimuth_, -1 );
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


void uiSEGYReadStartInfo::parChg( CallBacker* cb )
{
    if ( cb == nrsampsrcfld_ )
    {
	const bool fromheader = nrsampsrcfld_->currentItem() == 0;
	if ( fromheader )
	    nsfld_->setValue( hp_loaddefcache.getParam(this)->ns_ );
	showNrSamplesSetting( !fromheader );
    }
    else if ( cb == zsampsrcfld_ )
    {
	const bool fromheader = zsampsrcfld_->currentItem() == 0;
	if ( fromheader )
	{
	    const SEGY::LoadDef& loaddef = *hp_loaddefcache.getParam( this );
	    zstartfld_->setValue( loaddef.sampling_.start );
	    srfld_->setValue( loaddef.sampling_.step );
	}
	showZSamplingSetting( !fromheader );
    }

    parChanged( false );
}


void uiSEGYReadStartInfo::setRev1Values()
{
    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    if ( xcoordbytefld_ )
    {
	const SEGY::HdrEntry xentry = *hdef[SEGY::TrcHeader::EntryXcdp()];
	xcoordbytefld_->setHdrEntry( xentry );
	const SEGY::HdrEntry yentry = *hdef[SEGY::TrcHeader::EntryYcdp()];
	ycoordbytefld_->setHdrEntry( yentry );
    }

    if ( inlbytefld_ )
    {
	const SEGY::HdrEntry ientry = *hdef[SEGY::TrcHeader::EntryInline()];
	inlbytefld_->setHdrEntry( ientry );
	const SEGY::HdrEntry centry = *hdef[SEGY::TrcHeader::EntryCrossline()];
	crlbytefld_->setHdrEntry( centry );
    }

    if ( trcnrbytefld_ )
    {
	const SEGY::HdrEntry tentry = *hdef[SEGY::TrcHeader::EntryCdp()];
	trcnrbytefld_->setHdrEntry( tentry );
	const SEGY::HdrEntry sentry = *hdef[SEGY::TrcHeader::EntrySP()];
	refnrbytefld_->setHdrEntry( sentry );
    }

    if ( offsetbytefld_ )
    {
	const SEGY::HdrEntry oentry = *hdef[SEGY::TrcHeader::EntryOffset()];
	offsetbytefld_->setHdrEntry( oentry );
    }
}


void uiSEGYReadStartInfo::parChanged( bool revchg )
{
    if ( parsbeingset_ )
	return;

    if ( revchg && revfld_->currentItem()>0 )
	setRev1Values();

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

    clearTable();

    if ( doupd )
	parChanged( false );
}


void uiSEGYReadStartInfo::clearInfo()
{
    xinfotxt_.setEmpty(); yinfotxt_.setEmpty();
    inlinfotxt_.setEmpty(); crlinfotxt_.setEmpty();
    trcnrinfotxt_.setEmpty(); refnrinfotxt_.setEmpty();
    offsetinfotxt_.setEmpty(); azimuthinfotxt_.setEmpty();
    for ( int irow=0; irow<mKey1Row; irow++ )
	setCellTxt( mQSResCol, irow, uiString::empty() );

    updateCellTexts();
}


void uiSEGYReadStartInfo::setScanInfo( const SEGY::ScanInfoSet& sis )
{
    const int nrfiles = sis.size();
    uiString scanlabel = sis.isFull() ? tr("Full scan result")
				      : tr("Quick scan result");
    tbl_->setColumnLabel( mQSResCol,
	    nrfiles <=1 ? scanlabel : tr("%1 (from 1st file)").arg(scanlabel) );

    tbl_->setColumnLabel( mUseCol, nrfiles <= 1 ? tr("Actually use")
					: tr("Actually use (for all files)") );

    uiString txt = nrfiles <= 1 ? uiString::empty()
				: tr( "%1 files selected" ).arg( nrfiles );
    tbl_->setColumnLabel( mItemCol, txt );

    if ( nrfiles < 1 )
    {
	clearInfo();
	return;
    }

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
	txt.arg( uiString::empty()  );
    setCellTxt( mQSResCol, mDataFormatRow, txt );

    txt.set( "%1 (%2 %3)" );
    const int nrtrcs = sis.nrTraces();
    txt.arg( bi.ns_ ).arg( nrtrcs )
	.arg( nrtrcs==1 ? tr("trace") : tr("traces") );
    setCellTxt( mQSResCol, mNrSamplesRow, txt );

    if ( mIsUdf(bi.sampling_.step) )
	txt = uiString::empty();
    else
    {
	txt.set( "%1 - %2 - %3 (s or %4)" );
	const float endz = bi.sampling_.start
			 + (bi.ns_-1) * bi.sampling_.step;
	txt.arg( bi.sampling_.start ).arg( endz )
	   .arg( bi.sampling_.step )
	   .arg( sis.inFeet() ? tr("ft") : tr("m") );
    }
    setCellTxt( mQSResCol, mZRangeRow, txt );

    const SEGY::ScanRangeInfo& rgs = sis.ranges();
    const char* rgstr = "%1 - %2";
    inlinfotxt_.set( rgstr ).arg( rgs.inls_.start ).arg( rgs.inls_.stop );
    crlinfotxt_.set( rgstr ).arg( rgs.crls_.start ).arg( rgs.crls_.stop );
    trcnrinfotxt_.set( rgstr ).arg( rgs.trcnrs_.start ).arg( rgs.trcnrs_.stop );
    const int nrdec = SI().nrXYDecimals();
    xinfotxt_.set( rgstr ).arg( rgs.xrg_.start, nrdec )
			  .arg( rgs.xrg_.stop, nrdec );
    yinfotxt_.set( rgstr ).arg( rgs.yrg_.start, nrdec )
			  .arg( rgs.yrg_.stop, nrdec );
    offsetinfotxt_.set( rgstr ).arg( rgs.offs_.start ).arg( rgs.offs_.stop );
    azimuthinfotxt_.set( rgstr ).arg( rgs.azims_.start*360/M_PI )
				.arg( rgs.azims_.stop*360/M_PI );
    if ( mIsUdf(rgs.refnrs_.start) )
	refnrinfotxt_ =  tr( "<no data>" );
    else
	refnrinfotxt_.set(rgstr).arg( rgs.refnrs_.start )
				.arg( rgs.refnrs_.stop );
}


void uiSEGYReadStartInfo::setLoadDefCache( const SEGY::LoadDef& ld )
{
    *hp_loaddefcache.getParam(this) = ld;
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

    nrsampsrcfld_->setCurrentItem( loaddef_.usenrsampsinfile_ ? 0 : 1 );
    showNrSamplesSetting( !loaddef_.usenrsampsinfile_ );

    zsampsrcfld_->setCurrentItem( loaddef_.usezsamplinginfile_ ? 0 : 1 );
    showZSamplingSetting( !loaddef_.usezsamplinginfile_ );

    NotifyStopper ns( nsfld_->valueChanging );
    nsfld_->setValue( loaddef_.ns_ );
    zstartfld_->setValue( loaddef_.sampling_.start );
    srfld_->setValue( loaddef_.sampling_.step );
    if ( imptype_.isVSP() )
	{ parsbeingset_ = false; return; }

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
    {
	trcnrsrcfld_->setCurrentItem( loaddef_.havetrcnrs_ ? 0 : 1 );
	refnrbytefld_->setHdrEntry( loaddef_.hdrdef_->refnr_ );
	if ( trcnrbytefld_ )
	    trcnrbytefld_->setHdrEntry( loaddef_.hdrdef_->trnr_ );
	if ( trcnrgengrp_ )
	{
	    trcnrgenstartfld_->setValue( loaddef_.trcnrdef_.start );
	    trcnrgenstepfld_->setValue( loaddef_.trcnrdef_.step );
	}
    }

    if ( xcoordbytefld_ )
    {
	const bool isic = loaddef_.icvsxytype_==SEGY::FileReadOpts::ICOnly;
	xcoordbytefld_->setSensitive( !isic );
	ycoordbytefld_->setSensitive( !isic );
	xcoordbytefld_->setHdrEntry( loaddef_.hdrdef_->xcoord_ );
	ycoordbytefld_->setHdrEntry( loaddef_.hdrdef_->ycoord_ );
    }

    if ( inlbytefld_ )
    {
	const bool isxy = loaddef_.icvsxytype_==SEGY::FileReadOpts::XYOnly;
	inlbytefld_->setSensitive( !isxy );
	crlbytefld_->setSensitive( !isxy );
	inlbytefld_->setHdrEntry( loaddef_.hdrdef_->inl_ );
	crlbytefld_->setHdrEntry( loaddef_.hdrdef_->crl_ );
    }

    if ( Seis::isPS(gt) )
    {
	psoffsrcfld_->setCurrentItem( (int)loaddef_.psoffssrc_ );
	offsetbytefld_->setHdrEntry( loaddef_.hdrdef_->offs_ );
	if ( offsgenstartfld_ )
	{
	    offsgenstartfld_->setValue( loaddef_.psoffsdef_.start );
	    offsgenstepfld_->setValue( loaddef_.psoffsdef_.step );
	}

	azimuthbytefld_->setHdrEntry( loaddef_.hdrdef_->azim_ );
    }

    parsbeingset_ = false;
}


void uiSEGYReadStartInfo::fillLoadDef()
{
    loaddef_.revision_ = revfld_->currentItem();

    const int fmtidx = fmtfld_->currentItem();
    loaddef_.dataswapped_ = fmtidx > nrunswappedfmts_;
    loaddef_.useformatinfile_ = fmtidx == 0;
    loaddef_.format_ = fmtidx==0 ? 0 : short(*fmtfld_->text() - '0');
    loaddef_.usenrsampsinfile_ = nrsampsrcfld_->currentItem() == 0;
    loaddef_.usezsamplinginfile_ = zsampsrcfld_->currentItem() == 0;

    const int newns = nsfld_->getIntValue();
    if ( newns > SEGY::cMaxReasonableNrSamples() )
    {
	// TODO: Add message with donot show again
    }

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

    if ( xcoordbytefld_ )
    {
	loaddef_.hdrdef_->xcoord_ = xcoordbytefld_->hdrEntry();
	loaddef_.hdrdef_->ycoord_ = ycoordbytefld_->hdrEntry();
    }

    const Seis::GeomType gt = imptype_.geomType();
    if ( Seis::is2D(gt) )
    {
	loaddef_.hdrdef_->refnr_ = refnrbytefld_->hdrEntry();
	if ( trcnrbytefld_ )
	    loaddef_.hdrdef_->trnr_ = trcnrbytefld_->hdrEntry();
	loaddef_.havetrcnrs_ = trcnrsrcfld_->currentItem() == 0;
	if ( trcnrgengrp_ )
	{
	    SamplingData<int>& def = loaddef_.trcnrdef_;
	    def.start = trcnrgenstartfld_->getIntValue();
	    if ( mIsUdf(def.start) )
		def.start = 1;

	    def.step = trcnrgenstepfld_->getIntValue();
	    if ( mIsUdf(def.step) || def.step == 0 )
		def.step = 1;
	}
    }
    else if ( inlbytefld_ )
    {
	loaddef_.hdrdef_->inl_ = inlbytefld_->hdrEntry();
	loaddef_.hdrdef_->crl_ = crlbytefld_->hdrEntry();
    }

    if ( Seis::isPS(gt) )
    {
	loaddef_.psoffssrc_ = (SEGY::FileReadOpts::PSDefType)
				    psoffsrcfld_->currentItem();
	loaddef_.hdrdef_->offs_ = offsetbytefld_->hdrEntry();
	if ( offsgenstartfld_ )
	{
	    SamplingData<float>& def = loaddef_.psoffsdef_;
	    def.start = offsgenstartfld_->getFValue();
	    if ( mIsUdf(def.start) ) def.start = 0.f;
	    def.step = offsgenstepfld_->getFValue();
	    if ( mIsUdf(def.step) ) def.step = 1.f;
	}

	loaddef_.hdrdef_->azim_ = azimuthbytefld_->hdrEntry();
    }
}
