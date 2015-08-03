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
#include "uitable.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uisegyimptype.h"
#include "uisegydef.h"

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


uiSEGYReadStartInfo::uiSEGYReadStartInfo( uiParent* p, SEGY::uiScanDef& scd )
    : uiGroup(p,"SEGY read start info")
    , scandef_(scd)
    , scandefChanged(this)
    , parsbeingset_(false)
{
    tbl_ = new uiTable( this, uiTable::Setup(mNrInfoRows,3)
				  .manualresize(true), "Info table" );
    tbl_->setColumnLabel( 0, "" );
    tbl_->setColumnLabel( 1, "Quick scan result" );
    tbl_->setColumnLabel( 2, "Use" );
    tbl_->setColumnStretchable( 0, false );
    tbl_->setColumnStretchable( 1, true );
    tbl_->setPrefWidthInChar( 80 );
    tbl_->setPrefHeightInRows( mNrInfoRows );
    tbl_->setTableReadOnly( true );
    tbl_->setLeftHeaderHidden( true );
    setCellTxt( 0, mRevRow, "SEG-Y Revision" );
    setCellTxt( 0, mDataFormatRow, "Data format" );
    setCellTxt( 0, mNrSamplesRow, "Number of samples" );
    setCellTxt( 0, mZRangeRow, "Z Range" );

    const CallBack cb( mCB(this,uiSEGYReadStartInfo,parChg) );

    uiGroup* grp = new uiGroup( 0, "rev group" );
    const char* revstrs[] = { "0", "1", 0 };
    revfld_ = new uiComboBox( grp, revstrs, "Revision" );
    revfld_->selectionChanged.notify( cb );
    revfld_->setStretch( 2, 1 );
    tbl_->setCellGroup( RowCol(mRevRow,2), grp );

    useScanDef();
}


void uiSEGYReadStartInfo::setCellTxt( int col, int row, const char* txt )
{
    tbl_->setText( RowCol(row,col), txt );
    if ( col == 0 )
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
    const char* k1str; const char* k2str; const char* psstr;
    const char* xstr; const char* ystr;
    k1str = k2str = psstr = xstr = ystr = "";

    if ( !imptype_.isVSP() )
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
	.add( " (s or " ).add( sd.infeet_ ? "ft)" : "m)" );
    setCellTxt( 1, mZRangeRow, txt );

    if ( imptype_.isVSP() )
	return;

    const Seis::GeomType gt = imptype_.geomType();
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


void uiSEGYReadStartInfo::useScanDef()
{
    parsbeingset_ = true;
    revfld_->setCurrentItem( scandef_.revision_ );
    parsbeingset_ = false;
}


void uiSEGYReadStartInfo::fillScanDef()
{
    scandef_.revision_ = revfld_->currentItem();
}
