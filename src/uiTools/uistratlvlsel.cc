/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uistratlvlsel.h"
#include "stratlevel.h"
#include "uicombobox.h"

static const char* sNoLevelTxt			= "---";
const char* uiStratLevelSel::sTiedToTxt()	{ return "Tied to Level"; }


static void getLvlNms( BufferStringSet& bss, bool withudf )
{
    if ( withudf )
	bss.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::LVLS().levels().size(); idx++ )
	bss.add( Strat::LVLS().levels()[idx]->name() );
}


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool withudf, const char* lbl )
    : uiGroup(p)
    , haveudf_(withudf)
    , selChange(this)
{
    BufferStringSet bss; getLvlNms( bss, haveudf_ );

    if ( lbl && *lbl )
	fld_ = (new uiLabeledComboBox( this, bss, lbl ))->box();
    else
    {
	fld_ = new uiComboBox( this, bss, sTiedToTxt() );
	fld_->setStretch( 2, 2 );
    }
    fld_->selectionChanged.notify( mCB(this,uiStratLevelSel,selCB) );

    Strat::eLVLS().levelChanged.notify( mCB(this,uiStratLevelSel,extChgCB) );
    setHAlignObj( fld_ );
}


uiStratLevelSel::~uiStratLevelSel()
{
    Strat::eLVLS().levelChanged.remove( mCB(this,uiStratLevelSel,extChgCB) );
}


const Strat::Level* uiStratLevelSel::selected() const
{
    const char* seltxt = fld_->text();
    if ( !seltxt || !*seltxt || (haveudf_ && !strcmp(seltxt,sNoLevelTxt)) )
	return 0;

    return Strat::LVLS().get( seltxt );
}


const char* uiStratLevelSel::getName() const
{
    const Strat::Level* lvl = selected();
    return lvl && lvl->id() >= 0 ? lvl->name().buf() : "";
}


Color uiStratLevelSel::getColor() const
{
    const Strat::Level* lvl = selected();
    return lvl && lvl->id() >= 0 ? lvl->color() : Color::NoColor();
}


int uiStratLevelSel::getID() const
{
    const Strat::Level* lvl = selected();
    return lvl ? lvl->id() : -1;
}


void uiStratLevelSel::setSelected( const Strat::Level* lvl )
{
    if ( !lvl )
	fld_->setCurrentItem( ((int)0) );
    else
	fld_->setCurrentItem( lvl->name() );
}


void uiStratLevelSel::setName( const char* nm )
{
    setSelected( Strat::LVLS().get(nm) );
}


void uiStratLevelSel::setID( int id )
{
    const Strat::Level* lvl = id < 0 ? 0 : Strat::LVLS().get( id );
    setSelected( lvl );
}


void uiStratLevelSel::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSel::extChgCB( CallBacker* )
{
    const Strat::Level* cursel = selected();
    fld_->setEmpty();

    BufferStringSet bss; getLvlNms( bss, haveudf_ );
    fld_->addItems( bss );

    if ( cursel )
	fld_->setCurrentItem( cursel->name().buf() );
    else if ( haveudf_ )
	fld_->setCurrentItem( ((int)0) );
}
