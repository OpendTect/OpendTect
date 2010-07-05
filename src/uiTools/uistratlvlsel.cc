/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvlsel.cc,v 1.18 2010-07-05 16:08:07 cvsbruno Exp $";

#include "uistratlvlsel.h"

#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrattreewin.h"

static const char* sNoLevelTxt	= "--Undefined--";
static const char* sTiedToTxt	= "Tied to Level";

static void getLvlNms( BufferStringSet& bss, TypeSet<int>& ids )
{
    BufferStringSet tmplvlnmset;
    Strat::RT().getUnitIDs( ids );
    for ( int idx=ids.size()-1; idx>=0; idx-- )
    {
	BufferString lvlnm( Strat::RT().getUnitLvlName( ids[idx] ) );
	if ( !lvlnm.isEmpty() ) 
	    tmplvlnmset.add( lvlnm );
	else 
	    ids.remove( idx );
    }
    bss.add( sNoLevelTxt );
    for ( int idx=tmplvlnmset.size()-1; idx>=0; idx-- )
	bss.add( tmplvlnmset.get( idx ) );
}


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool wlbl )
    : uiGroup(p)
    , selChange(this)
{
    BufferStringSet bss; getLvlNms( bss, ids_ );

    uiObject* attachobj = 0;
    if ( wlbl )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, bss, sTiedToTxt,
							sTiedToTxt );
	selfld_ = lcb->box();
	attachobj = lcb->attachObj();
    }
    else
    {
	selfld_ = new uiComboBox( this, bss, sTiedToTxt );
	selfld_->setStretch( 2, 2 );
	attachobj = selfld_;
    }
    selfld_->selectionChanged.notify( mCB(this,uiStratLevelSel,selCB) );

    StratTWin().unitCreated.notify( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().unitChanged.notify( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().unitRemoved.notify( mCB(this,uiStratLevelSel,chgCB) );

    setHAlignObj( selfld_ );
}


uiStratLevelSel::~uiStratLevelSel()
{
    StratTWin().unitCreated.remove( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().unitChanged.remove( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().unitRemoved.remove( mCB(this,uiStratLevelSel,chgCB) );
}


int uiStratLevelSel::selected() const
{
    return selfld_->currentItem();
}


int uiStratLevelSel::getID() const
{
    int selidx = selected();
    if ( selidx <= 0 ) return -1;
    return ids_[selidx-1];
}


void uiStratLevelSel::setID( int id )
{
    if ( id < 0 )
	selfld_->setCurrentItem( (int)0 );
    else
	selfld_->setCurrentItem( Strat::RT().getUnitLvlName( id ) );
}


Color uiStratLevelSel::getColor() const
{
    return Strat::RT().getUnitColor( getID() );
}


const char* uiStratLevelSel::getName() const
{
    return Strat::RT().getUnitLvlName( getID() );
}


void uiStratLevelSel::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSel::chgCB( CallBacker* )
{
    int cursel = getID();
    selfld_->empty();
    ids_.erase();
    BufferStringSet bss; getLvlNms( bss, ids_ );
    BufferString nm( getName() );
    selfld_->addItems( bss );
    if ( cursel < 0 )  nm = sNoLevelTxt;
    selfld_->setCurrentItem( nm );
}
