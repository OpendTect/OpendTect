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


static void getLvlNms( BufferStringSet& bss, bool withudf )
{
    BufferStringSet realnms;
    Strat::LVLS().getNames( realnms );
    if ( withudf )
	bss.add( sNoLevelTxt );
    bss.append( realnms );
}


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool withudf,
						const uiString& lbl )
    : uiGroup(p)
    , fld_(0)
    , haveudf_(withudf)
    , selChange(this)
{
    BufferStringSet bss; getLvlNms( bss, haveudf_ );

    if ( !lbl.isEmpty() )
	fld_ = (new uiLabeledComboBox(this, bss, lbl))->box();
    else
    {
	fld_ = new uiComboBox( this, bss, mFromUiStringTodo(sTiedToTxt()) );
	fld_->setStretch( 2, 2 );
    }
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
    const BufferString seltxt( fld_->text() );
    if ( seltxt.isEmpty() || (haveudf_ && seltxt == sNoLevelTxt) )
	return Strat::Level::undef();

    return Strat::LVLS().getByName( seltxt );
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


void uiStratLevelSel::extChgCB( CallBacker* )
{
    const Strat::Level cursel = selected();
    fld_->setEmpty();

    BufferStringSet bss; getLvlNms( bss, haveudf_ );
    fld_->addItems( bss );

    if ( !cursel.id().isInvalid() )
	fld_->setCurrentItem( cursel.name() );
    else if ( haveudf_ )
	fld_->setCurrentItem( ((int)0) );
}
