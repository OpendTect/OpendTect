/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvlsel.cc,v 1.14 2009-07-22 16:01:42 cvsbert Exp $";

#include "uistratlvlsel.h"

#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrattreewin.h"


static const char* sNoLevelTxt	= "--Undefined--";
static const char* sTiedToTxt	= "Tied to Level";

static void getLvlNms( BufferStringSet& bss )
{
    bss.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::RT().nrLevels(); idx++ )
	bss.add( Strat::RT().levelFromIdx(idx)->name() );
}


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool wlbl, bool wdefine )
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

    if ( wdefine )
    {
	uiPushButton* deflvlbut = new uiPushButton( this, "&Define Level",
			mCB(this,uiStratLevelSel,defCB), true );
	deflvlbut->setHSzPol( uiPushButton::Medium );
	if ( attachobj )
	    deflvlbut->attach( rightOf, attachobj );
    }

    StratTWin().levelCreated.notify( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().levelChanged.notify( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().levelRemoved.notify( mCB(this,uiStratLevelSel,chgCB) );

    setHAlignObj( selfld_ );
}


uiStratLevelSel::~uiStratLevelSel()
{
    StratTWin().levelCreated.remove( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().levelChanged.remove( mCB(this,uiStratLevelSel,chgCB) );
    StratTWin().levelRemoved.remove( mCB(this,uiStratLevelSel,chgCB) );
}


const Strat::Level* uiStratLevelSel::selected() const
{
    const int selidx = selfld_->currentItem();
    if ( selidx == 0 ) return 0;
    const Strat::Level* lvl = Strat::RT().getLevel( selfld_->text() );
    return lvl;
}


int uiStratLevelSel::getID() const
{
    const Strat::Level* lvl = selected();
    return lvl ? lvl->id_ : -1;
}


void uiStratLevelSel::setID( int id )
{
    const Strat::Level* lvl = Strat::RT().levelFromID( id );
    if ( !lvl )
	selfld_->setCurrentItem( (int)0 );
    else
	selfld_->setCurrentItem( lvl->name() );
}


Color uiStratLevelSel::getColor() const
{
    const Strat::Level* lvl = selected();
    return lvl && lvl->id_ >= 0 ? lvl->color_ : Color::NoColor();
}


const char* uiStratLevelSel::getName() const
{
    const Strat::Level* lvl = selected();
    return lvl && lvl->id_ >= 0 ? lvl->name() : "";
}


void uiStratLevelSel::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSel::defCB( CallBacker* )
{
    StratTWin().popUp();
}


void uiStratLevelSel::chgCB( CallBacker* )
{
    const Strat::Level* cursel = selected();
    selfld_->empty();
    BufferStringSet bss; getLvlNms( bss );
    selfld_->addItems( bss );
    selfld_->setCurrentItem( cursel ? cursel->name().buf() : sNoLevelTxt );
}
