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
uiString uiStratLevelSel::sTiedToTxt()	{ return tr("Tied to Level"); }


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool withudf,
				  const uiString& lbl )
    : uiGroup(p)
    , haveudf_(withudf)
    , selChange(this)
{
    if ( lbl.isEmpty() )
    {
	fld_ = new uiComboBox( this, toString(sTiedToTxt()) );
	fld_->setStretch( 1, 1 );
    }
    else
	fld_ = (new uiLabeledComboBox(this, lbl))->box();

    fill();
    fld_->setHSzPol( uiObject::MedVar );

    mAttachCB( fld_->selectionChanged, uiStratLevelSel::selCB );
    mAttachCB( Strat::lvlSetMgr().curChanged, uiStratLevelSel::curSetChgCB );
    mAttachCB( Strat::eLVLS().changed, uiStratLevelSel::extChgCB );
    mAttachCB( Strat::eLVLS().levelAdded, uiStratLevelSel::extChgCB );
    mAttachCB( Strat::eLVLS().levelToBeRemoved, uiStratLevelSel::extChgCB );
    setStretch( 1, 0 );
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

    const BufferString lvltxt = fld_->text();
    if ( Strat::LVLS().isPresent(lvltxt) )
	return Strat::LVLS().getByName( lvltxt );

    return Strat::lvlSetMgr().unpushedLVLS().getByName( lvltxt );
}


BufferString uiStratLevelSel::getLevelName() const
{
    const Strat::Level lvl( selected() );
    return lvl.id().isValid() ? lvl.name() : BufferString::empty();
}


OD::Color uiStratLevelSel::getColor() const
{
    const Strat::Level lvl = selected();
    return lvl.id().isValid() ? lvl.color() : OD::Color::NoColor();
}


Strat::LevelID uiStratLevelSel::getID() const
{
    return selected().id();
}


void uiStratLevelSel::setID( Strat::LevelID lvlid )
{
    if ( lvlid.isValid() )
	fld_->setCurrentItem( Strat::LVLS().get(lvlid).name() );
    else if ( haveudf_ )
	fld_->setCurrentItem( ((int)0) );
}


void uiStratLevelSel::setName( const char* nm )
{
    fld_->setCurrentItem( nm );
}


void uiStratLevelSel::setToolTip( const uiString& tt )
{
    fld_->setToolTip( tt );
}


void uiStratLevelSel::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSel::addItem( const char* nm, const OD::Color& col )
{
    fld_->addItem( toUiString(nm) );
    fld_->setColorIcon( fld_->size()-1, col );
}


void uiStratLevelSel::fill()
{
    const Strat::LevelSet& lvls = Strat::LVLS();
    if ( haveudf_ )
	addItem( sNoLevelTxt, OD::Color::NoColor() );

    for ( int ilvl=0; ilvl<lvls.size(); ilvl++ )
    {
	const Strat::Level lvl = lvls.getByIdx( ilvl );
	addItem( lvl.name(), lvl.color() );
    }
}


void uiStratLevelSel::curSetChgCB( CallBacker* cb )
{
    mAttachCBIfNotAttached( Strat::eLVLS().changed, uiStratLevelSel::extChgCB );
    mAttachCBIfNotAttached( Strat::eLVLS().levelAdded,
			    uiStratLevelSel::extChgCB );
    mAttachCBIfNotAttached( Strat::eLVLS().levelToBeRemoved,
			    uiStratLevelSel::extChgCB );
    extChgCB( cb );
}


void uiStratLevelSel::extChgCB( CallBacker* )
{
    const Strat::Level cursel = selected();
    fld_->setEmpty();
    fill();
    if ( cursel.id().isValid() )
	fld_->setCurrentItem( cursel.name() );
    else if ( haveudf_ )
	fld_->setCurrentItem( ((int)0) );
}
