/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2010
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipositiontable.cc,v 1.1 2010-03-18 03:38:57 cvsnanne Exp $";

#include "uipositiontable.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "ranges.h"
#include "survinfo.h"


uiPositionTable::uiPositionTable( uiParent* p, bool withxy, bool withic,
				  bool withz )
    : uiGroup(p,"Position Table")
    , withxy_(withxy)
    , withic_(withic)
    , withz_(withz)
{
    if ( !withxy_ && !withic_ )
	withic_ = true;

    BufferString infotxt( "Enter " );
    if ( withxy_ ) infotxt += "X/Y";
    if ( withxy_ && withic_ ) infotxt += " or ";
    if ( withic_ ) infotxt += "Inl/Crl";
    infotxt += " positions";
    uiLabel* lbl = new uiLabel( this, infotxt );
    table_ = new uiTable( this, uiTable::Setup().rowdesc("Node")
	    					.rowgrow(true)
						.defrowlbl(true), "Pos Table" );
    table_->attach( alignedBelow, lbl );
    table_->valueChanged.notify( mCB(this,uiPositionTable,posChgCB) );
    table_->setNrCols( withxy_ && withic_ ? 4 : 2 );
    table_->setNrRows( 5 );

    table_->setColumnLabel( 0, withxy_ ? "X" : "Inline" );
    table_->setColumnLabel( 1, withxy_ ? "Y" : "Crossline" );
    if ( withxy_ && withic_ )
    {
	table_->setColumnLabel( 2, "Inline" );
	table_->setColumnLabel( 3, "Crossline" );
    }

    if ( withz_ )
    {
	uiSeparator* hsep = new uiSeparator( this, "Separator" );
	hsep->attach( stretchedBelow, table_ );

	BufferString zlbl = "Z range "; zlbl += SI().getZUnitString();
	zfld_ = new uiGenInput( this, zlbl,
	    FloatInpIntervalSpec().setName("Z start",0).setName("Z stop",1) );
	zfld_->attach( leftAlignedBelow, table_ );
	zfld_->attach( ensureBelow, hsep );
    }
}


int uiPositionTable::getXCol() const	{ return withxy_ ? 0 : -1; }
int uiPositionTable::getYCol() const	{ return withxy_ ? 1 : -1; }

int uiPositionTable::getICol() const
{ return withxy_ && withic_ ? 2 : (withic_ ? 0 : -1); }
int uiPositionTable::getCCol() const
{ return withxy_ && withic_ ? 3 : (withic_ ? 1 : -1); }


void uiPositionTable::posChgCB( CallBacker* )
{
    if ( !withxy_ || !withic_ )
	return;

    NotifyStopper ns( table_->valueChanged );
    const RowCol& rc = table_->notifiedCell();
    if ( rc.col==0 || rc.col==1 )
    {
    }
    else if ( rc.col==2 || rc.col==3 )
    {
    }
}


void uiPositionTable::setCoords( const TypeSet<Coord>& coords )
{
    const int sz = coords.size();
    table_->setNrRows( sz+5 );
    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord& crd = coords[idx];
	if ( withxy_ )
	{
	    table_->setValue( RowCol(idx,getXCol()), crd.x );
	    table_->setValue( RowCol(idx,getYCol()), crd.y );
	}

	if ( withic_ )
	{
	    const BinID bid = SI().transform( crd );
	    table_->setValue( RowCol(idx,getICol()), bid.inl );
	    table_->setValue( RowCol(idx,getCCol()), bid.crl );
	}
    }
}


void uiPositionTable::getCoords( TypeSet<Coord>& coords ) const
{
}


void uiPositionTable::setBinIDs( const TypeSet<BinID>& binids )
{
    const int sz = binids.size();
    table_->setNrRows( sz+5 );
    for ( int idx=0; idx<sz; idx++ )
    {
	const BinID& bid = binids[idx];
	if ( withxy_ )
	{
	    const Coord crd = SI().transform( bid );
	    table_->setValue( RowCol(idx,getXCol()), crd.x );
	    table_->setValue( RowCol(idx,getYCol()), crd.y );
	}

	if ( withic_ )
	{
	    table_->setValue( RowCol(idx,getICol()), bid.inl );
	    table_->setValue( RowCol(idx,getCCol()), bid.crl );
	}
    }

}


void uiPositionTable::getBinIDs( TypeSet<BinID>& binids ) const
{
}


void uiPositionTable::setZRange( const Interval<float>& zrg )
{ zfld_->setValue( zrg ); }


void uiPositionTable::getZRange( Interval<float>& zrg ) const
{
    zrg.setFrom( withz_ ? zfld_->getFInterval() 
	    		: (Interval<float>)SI().zRange(false) );
}
