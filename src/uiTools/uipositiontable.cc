/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2010
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipositiontable.cc,v 1.4 2010/06/22 10:49:26 cvsbert Exp $";

#include "uipositiontable.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "pixmap.h"
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

    uiLabel* pmlvl =  new uiLabel( this, "" );
    ioPixmap pm( 20, 20 ); pm.fill( Color(200,0,0) );
    pmlvl->setPixmap( pm );
    pmlvl->attach( rightTo, lbl );

    uiLabel* collbl =  new uiLabel( this, "Node outside Survey" );
    collbl->attach( rightTo, pmlvl );

    table_ = new uiTable( this, uiTable::Setup().rowdesc("Node")
	    					.rowgrow(true)
						.defrowlbl(true), "Pos Table" );
    table_->attach( alignedBelow, lbl );
    table_->valueChanged.notify( mCB(this,uiPositionTable,posChgCB) );
    table_->setNrCols( withxy_ && withic_ ? 4 : 2 );
    table_->setNrRows( 5 );
    attachObj()->setMinimumWidth( withxy_ && withic_ ? 400 : 200 );

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
	zfld_->setStretch( 0, 0 );
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
    BinID bid;
    if ( rc.col==0 || rc.col==1 )
    {
	Coord coord( table_->getdValue(RowCol(rc.row,0)),
		     table_->getdValue(RowCol(rc.row,1)) );
	bid = SI().transform( coord );
	if ( withic_ )
	{
	    table_->setValue( RowCol(rc.row,2), bid.inl );
	    table_->setValue( RowCol(rc.row,3), bid.crl );
	}
    }
    else if ( rc.col==2 || rc.col==3 )
    {
	bid = BinID( table_->getIntValue(RowCol(rc.row,2)),
		     table_->getIntValue(RowCol(rc.row,3)) );
	Coord coord = SI().transform( bid );
	if ( withxy_ )
	{
	    table_->setValue( RowCol(rc.row,0), coord.x );
	    table_->setValue( RowCol(rc.row,1), coord.y );
	}
    }

    setRowColor( rc.row, SI().includes(bid,SI().zRange(true).start,true) );
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

	const BinID bid = SI().transform( crd );

	if ( withic_ )
	{
	    table_->setValue( RowCol(idx,getICol()), bid.inl );
	    table_->setValue( RowCol(idx,getCCol()), bid.crl );
	}

	setRowColor( idx, SI().includes(bid,SI().zRange(true).start,true) );
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

	setRowColor( idx, SI().includes(bid,SI().zRange(true).start,true) );
    }

}


void uiPositionTable::getBinIDs( TypeSet<BinID>& binids ) const
{
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	if ( withic_ )
	{
	    if ( mIsUdf(table_->getfValue(RowCol(idx,2))) ||
		 mIsUdf(table_->getfValue(RowCol(idx,3))) )
		continue;

	    binids += BinID( table_->getIntValue(RowCol(idx,2)),
			     table_->getIntValue(RowCol(idx,3)) );
	}
	else
	{
	    if ( mIsUdf(table_->getdValue(RowCol(idx,0))) ||
		 mIsUdf(table_->getdValue(RowCol(idx,1))) )
		continue;

	    Coord coord( table_->getdValue(RowCol(idx,0)),
		    	 table_->getdValue(RowCol(idx,1)) );
	    binids += SI().transform( coord );
	}
    }
}


void uiPositionTable::setZRange( const Interval<float>& zrg )
{ zfld_->setValue( zrg ); }


void uiPositionTable::getZRange( Interval<float>& zrg ) const
{
    zrg.setFrom( withz_ ? zfld_->getFInterval() 
	    		: (Interval<float>)SI().zRange(false) );
}

void uiPositionTable::setRowColor( int rid, bool includes )
{
    Color col = !includes ? Color(200,0,0) : Color::White();
    for ( int colid=0; colid<table_->nrCols(); colid++ )
	table_->setColor( RowCol(rid,colid), col );
}
