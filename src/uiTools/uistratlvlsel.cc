/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvlsel.cc,v 1.22 2010-09-27 11:05:19 cvsbruno Exp $";

#include "uistratlvlsel.h"

#include "stratlevel.h"
#include "stratreftree.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrattreewin.h"

static const char* sNoLevelTxt	= "--Undefined--";
static const char* sTiedToTxt	= "Tied to Level";

static void getLvlNms( BufferStringSet& bss )
{
    bss.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::LVLS().levels().size(); idx++ )
	bss.add( Strat::LVLS().levels()[idx]->name() );
}


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool wlbl )
    : uiGroup(p)
    , selChange(this)
{
    BufferStringSet bss; getLvlNms( bss );

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

    Strat::eRT().unitAdded.notify( mCB(this,uiStratLevelSel,chgCB) );
    Strat::eRT().unitChanged.notify( mCB(this,uiStratLevelSel,chgCB) );
    Strat::eRT().unitToBeDeleted.notify( mCB(this,uiStratLevelSel,chgCB) );
    Strat::eLVLS().levelChanged.notify( mCB(this,uiStratLevelSel,chgCB) );

    setHAlignObj( selfld_ );
}


uiStratLevelSel::~uiStratLevelSel()
{
    Strat::eRT().unitAdded.remove( mCB(this,uiStratLevelSel,chgCB) );
    Strat::eRT().unitChanged.remove( mCB(this,uiStratLevelSel,chgCB) );
    Strat::eRT().unitToBeDeleted.remove( mCB(this,uiStratLevelSel,chgCB) );
    Strat::eLVLS().levelChanged.remove( mCB(this,uiStratLevelSel,chgCB) );
}


const Strat::Level* uiStratLevelSel::selected() const
{
    const int selidx =selfld_->currentItem();
    return selidx == 0 ? 0 : Strat::LVLS().get( selfld_->text() );
}


int uiStratLevelSel::getID() const
{
    const Strat::Level* lvl = selected();
    return lvl ? lvl->id() : -1;
}


void uiStratLevelSel::setID( int id )
{
    const Strat::Level* lvl = Strat::LVLS().get( id );
    if ( !lvl )
	selfld_->setCurrentItem( (int)0 );
    else
	selfld_->setCurrentItem( lvl->name() );
}


Color uiStratLevelSel::getColor() const
{
    const Strat::Level* lvl = selected();
    return lvl && lvl->id() >= 0 ? lvl->color() : Color::NoColor();
}


const char* uiStratLevelSel::getName() const
{
    const Strat::Level* lvl = selected();
    return lvl && lvl->id() >= 0 ? lvl->name().buf() : "";
}


void uiStratLevelSel::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSel::chgCB( CallBacker* )
{
    const Strat::Level* cursel = selected();
    selfld_->empty();
    BufferStringSet bss; getLvlNms( bss );

    selfld_->addItems( bss );
    selfld_->setCurrentItem( cursel ? cursel->name().buf() : sNoLevelTxt );
}
