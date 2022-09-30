/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipositiontable.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipixmap.h"
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

    uiString infotxt;
    if ( withxy_ ) infotxt = tr("Enter X/Y positions");
    if ( withxy_ && withic_ ) infotxt = tr("Enter X/y or Inl/Crl positions");
    if ( withic_ ) infotxt = tr("Enter Inl/Crl positions");

    uiLabel* lbl = new uiLabel( this, infotxt );

    uiLabel* pmlvl =  new uiLabel( this, uiStrings::sEmptyString() );
    uiPixmap pm( 20, 20 ); pm.fill( OD::Color(200,0,0) );
    pmlvl->setPixmap( pm );
    pmlvl->attach( rightTo, lbl );

    uiLabel* collbl =  new uiLabel( this, tr("Node outside Survey") );
    collbl->attach( rightTo, pmlvl );

    table_ = new uiTable( this, uiTable::Setup().rowdesc("Node")
	    					.rowgrow(true)
						.selmode(uiTable::Multi)
						.defrowlbl(true),
			  "Pos Table" );
    table_->attach( alignedBelow, lbl );
    table_->valueChanged.notify( mCB(this,uiPositionTable,posChgCB) );
    table_->setNrCols( withxy_ && withic_ ? 4 : 2 );
    table_->setNrRows( 5 );
    attachObj()->setMinimumWidth( withxy_ && withic_ ? 400 : 200 );

    table_->setColumnLabel(0, withxy_ ? uiStrings::sX() : uiStrings::sInline());
    table_->setColumnLabel(1, withxy_ ? uiStrings::sY()
				      : uiStrings::sCrossline());
    if ( withxy_ && withic_ )
    {
	table_->setColumnLabel( 2, uiStrings::sInline() );
	table_->setColumnLabel( 3, uiStrings::sCrossline() );
    }

    if ( withz_ )
    {
	uiSeparator* hsep = new uiSeparator( this, "Separator" );
	hsep->attach( stretchedBelow, table_ );

	uiString zlbl = uiStrings::phrJoinStrings(uiStrings::sZRange(),
			SI().getUiZUnitString());
	zfld_ = new uiGenInput( this, zlbl,
	    FloatInpIntervalSpec().setName("Z start",0).setName("Z stop",1) );
	zfld_->setStretch( 0, 0 );
	zfld_->attach( leftAlignedBelow, table_ );
	zfld_->attach( ensureBelow, hsep );
    }
}


uiPositionTable::~uiPositionTable()
{}


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
    if ( rc.col()==0 || rc.col()==1 )
    {
	Coord coord( table_->getDValue(RowCol(rc.row(),0)),
		     table_->getDValue(RowCol(rc.row(),1)) );
	bid = SI().transform( coord );
	if ( withic_ )
	{
	    table_->setValue( RowCol(rc.row(),2), bid.inl() );
	    table_->setValue( RowCol(rc.row(),3), bid.crl() );
	}
    }
    else if ( rc.col()==2 || rc.col()==3 )
    {
	bid = BinID( table_->getIntValue(RowCol(rc.row(),2)),
		     table_->getIntValue(RowCol(rc.row(),3)) );
	Coord coord = SI().transform( bid );
	const int nrdec = SI().nrXYDecimals();
	if ( withxy_ )
	{
	    table_->setValue( RowCol(rc.row(),0), coord.x, nrdec );
	    table_->setValue( RowCol(rc.row(),1), coord.y, nrdec );
	}
    }

    setRowColor( rc.row(), SI().includes(bid,SI().zRange(true).center(),true) );
}


void uiPositionTable::setCoords( const TypeSet<Coord>& coords )
{
    const int sz = coords.size();
    table_->setNrRows( sz+5 );
    const int nrdec = SI().nrXYDecimals();
    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord& crd = coords[idx];
	if ( withxy_ )
	{
	    table_->setValue( RowCol(idx,getXCol()), crd.x, nrdec );
	    table_->setValue( RowCol(idx,getYCol()), crd.y, nrdec );
	}

	const BinID bid = SI().transform( crd );

	if ( withic_ )
	{
	    table_->setValue( RowCol(idx,getICol()), bid.inl() );
	    table_->setValue( RowCol(idx,getCCol()), bid.crl() );
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
    const int nrdec = SI().nrXYDecimals();
    for ( int idx=0; idx<sz; idx++ )
    {
	const BinID& bid = binids[idx];
	if ( withxy_ )
	{
	    const Coord crd = SI().transform( bid );
	    table_->setValue( RowCol(idx,getXCol()), crd.x, nrdec );
	    table_->setValue( RowCol(idx,getYCol()), crd.y, nrdec );
	}

	if ( withic_ )
	{
	    table_->setValue( RowCol(idx,getICol()), bid.inl() );
	    table_->setValue( RowCol(idx,getCCol()), bid.crl() );
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
	    if ( mIsUdf(table_->getFValue(RowCol(idx,2))) ||
		 mIsUdf(table_->getFValue(RowCol(idx,3))) )
		continue;

	    binids += BinID( table_->getIntValue(RowCol(idx,2)),
			     table_->getIntValue(RowCol(idx,3)) );
	}
	else
	{
	    if ( mIsUdf(table_->getDValue(RowCol(idx,0))) ||
		 mIsUdf(table_->getDValue(RowCol(idx,1))) )
		continue;

	    Coord coord( table_->getDValue(RowCol(idx,0)),
		    	 table_->getDValue(RowCol(idx,1)) );
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
    OD::Color col = !includes ? OD::Color(200,0,0) : OD::Color::White();
    for ( int colid=0; colid<table_->nrCols(); colid++ )
	table_->setColor( RowCol(rid,colid), col );
}
