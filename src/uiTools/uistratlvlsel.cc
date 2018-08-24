/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/

#include "uistratlvlsel.h"
#include "uicombobox.h"

static const char* sNoLevelTxt			= "---";
const uiString uiStratLevelSel::sTiedToTxt()	{ return tr("Tied to Level"); }


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool withudf,
				  const uiString& lbl )
    : uiGroup(p)
    , fld_(0)
    , haveudf_(withudf)
    , selChange(this)
{
    if ( !lbl.isEmpty() )
	fld_ = (new uiLabeledComboBox(this, lbl))->box();
    else
    {
	fld_ = new uiComboBox( this, toString(sTiedToTxt()) );
	fld_->setStretch( 2, 2 );
    }
    fill();
    fld_->selectionChanged.notify( mCB(this,uiStratLevelSel,selCB) );

    mAttachCB( Strat::eLVLS().objectChanged(), uiStratLevelSel::extChgCB );
    setHAlignObj( fld_ );
}


uiStratLevelSel::~uiStratLevelSel()
{
    detachAllNotifiers();
}


Strat::Level uiStratLevelSel::selected() const
{
    if ( haveudf_ && fld_->currentItem() == 0 )
	return Strat::Level::undef();

    return Strat::LVLS().getByName( fld_->text() );
}


BufferString uiStratLevelSel::getLevelName() const
{
    const Strat::Level lvl( selected() );
    return lvl.id().isValid() ? lvl.name() : BufferString::empty();
}


Color uiStratLevelSel::getColor() const
{
    const Strat::Level lvl = selected();
    return lvl.id().isValid() ? lvl.color() : Color::NoColor();
}


uiStratLevelSel::LevelID uiStratLevelSel::getID() const
{
    const Strat::Level lvl = selected();
    return lvl.id();
}


void uiStratLevelSel::setID( Strat::Level::ID lvlid )
{
    if ( !lvlid.isInvalid() )
	fld_->setCurrentItem( Strat::LVLS().get(lvlid).name() );
    else if ( haveudf_ )
	fld_->setCurrentItem( ((int)0) );
}


void uiStratLevelSel::setName( const char* nm )
{
    setID( Strat::LVLS().getIDByName(nm) );
}


void uiStratLevelSel::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSel::addItem( const char* nm, const Color& col )
{
    fld_->addItem( toUiString(nm) );
    fld_->setColorIcon( fld_->size()-1, col );
}


void uiStratLevelSel::fill()
{
    const auto& lvls = Strat::LVLS();
    MonitorLock ml( lvls );
    for ( int ilvl=-1; ilvl<lvls.size(); ilvl++ )
    {
	if ( ilvl == -1 )
	{
	    if ( haveudf_ )
		addItem( sNoLevelTxt, Color::NoColor() );
	}
	else
	{
	    const Strat::Level lvl = lvls.getByIdx( ilvl );
	    addItem( lvl.name(), lvl.color() );
	}
    }
}


void uiStratLevelSel::extChgCB( CallBacker* )
{
    const Strat::Level cursel = selected();
    fld_->setEmpty();
    fill();
    if ( !cursel.id().isInvalid() )
	fld_->setCurrentItem( cursel.name() );
    else if ( haveudf_ )
	fld_->setCurrentItem( ((int)0) );
}
