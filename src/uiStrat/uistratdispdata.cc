/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.1 2010-04-13 12:55:16 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratmgr.h"

#include "bufstringset.h"
#include "color.h"

#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uistrattreewin.h"


#define mAskStratWinNotif(nm)\
    StratTWin().nm.notify( mCB(this,uiStratDisp,triggerDataChange) );
uiStratDisp::uiStratDisp()
    : CallBacker(CallBacker::CallBacker()) 	
    , uistratmgr_(StratTWin().man()) 
    , dataChanged(this)  
{
    mAskStratWinNotif(unitCreated)
    mAskStratWinNotif(unitChanged)
    mAskStratWinNotif(unitRemoved)
    mAskStratWinNotif(levelCreated)
    mAskStratWinNotif(levelChanged)
    mAskStratWinNotif(levelRemoved)
    mAskStratWinNotif(newUnitSelected)
    mAskStratWinNotif(newLevelSelected)

    gatherInfo();
}


uiStratDisp::~uiStratDisp()
{
    deepErase( levels_ );
    deepErase( units_ );
}


void uiStratDisp::triggerDataChange( CallBacker* )
{
    dataChanged.trigger();
}


void uiStratDisp::gatherInfo()
{
    deepErase( levels_ );
    deepErase( units_ );
    addLevels();
    addUnits( *uistratmgr_.getCurTree(), 0 );
}


void uiStratDisp::addLevels()
{
    BufferStringSet lvlnms; TypeSet<Color> lvlcols;
    uistratmgr_.getLvlsTxtAndCol( lvlnms, lvlcols );
    for ( int idx=0; idx<lvlnms.size(); idx++ )
    {
	Color col; Interval<float> timerg;
	uistratmgr_.getLvlPars( lvlnms.get(idx), timerg, col );
	uiStratDisp::Level* lv = new uiStratDisp::Level( lvlnms.get( idx ), 
							 timerg.start );
	lv->col_ = col;
	levels_ += lv;
    }
}


void uiStratDisp::addUnits( const Strat::NodeUnitRef& nur, int order ) 
{
    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    addUnit( nur, order-1 );
	    mDynamicCastGet(const Strat::LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    addUnit( *lur, order );
	}
	else
	{
	    mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
	    { order++; addUnits( *chldnur, order ); }
	}
    }
}


void uiStratDisp::addUnit( const Strat::UnitRef& uref, int order )
{
    Color tcol, bcol; 
    Interval<float> ttimerg, btimerg;
    BufferString& tnm = *new BufferString(); 
    BufferString& bnm = *new BufferString();
    uistratmgr_.getNmsTBLvls( &uref, tnm, bnm );
    if ( tnm.isEmpty() || bnm.isEmpty() ) return;
    uistratmgr_.getLvlPars( tnm, ttimerg, tcol );
    uistratmgr_.getLvlPars( bnm, btimerg, bcol );

    uiStratDisp::Unit* unit = new uiStratDisp::Unit( uref.code(), 
						     ttimerg.start, 
						     btimerg.start );
    unit->col_ = tcol;
    unit->order_ = order;
    unit->toplvlnm_ = tnm;
    unit->botlvlnm_ = bnm;
    units_ += unit;
}
