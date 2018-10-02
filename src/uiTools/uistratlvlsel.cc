/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/

#include "uistratlvlsel.h"
#include "uicombobox.h"
#include "uilistbox.h"

static const char* sNoLevelTxt			= "---";
uiString uiStratLevelSel::sTiedToTxt()		{ return tr("Tied to Level"); }


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool withudf,
				  const uiString& lbl )
    : uiGroup(p)
    , haveudf_(withudf)
    , selChange(this)
{
    if ( !lbl.isEmpty() )
	fld_ = (new uiLabeledComboBox(this, lbl))->box();
    else
    {
	fld_ = new uiComboBox( this, toString(sTiedToTxt()) );
	fld_->setStretch( 1, 1 );
    }
    fill();
    fld_->setHSzPol( uiObject::MedVar );
    fld_->selectionChanged.notify( mCB(this,uiStratLevelSel,selCB) );

    mAttachCB( Strat::eLVLS().objectChanged(), uiStratLevelSel::extChgCB );
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
    return selected().id();
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


uiStratLevelSelDlg::uiStratLevelSelDlg( uiParent* p, const uiString& lbl,
					OD::ChoiceMode cm )
    : uiDialog(p,uiDialog::Setup(tr("Stratigraphic Level"),
				mNoDlgTitle,mTODOHelpKey))
    , selChange(this)
{
    uiListBox::Setup lbsu( cm, lbl );
    fld_ = new uiListBox( this, lbsu );
    fill();

    fld_->selectionChanged.notify( mCB(this,uiStratLevelSelDlg,selCB) );
    mAttachCB( Strat::eLVLS().objectChanged(), uiStratLevelSelDlg::extChgCB );
}


uiStratLevelSelDlg::~uiStratLevelSelDlg()
{
    detachAllNotifiers();
}


Strat::Level uiStratLevelSelDlg::selected() const
{
    return Strat::LVLS().getByName( fld_->getText() );
}


BufferString uiStratLevelSelDlg::getLevelName() const
{
    return BufferString( fld_->getText() );
}


Color uiStratLevelSelDlg::getColor() const
{
    const Strat::Level lvl = selected();
    return lvl.id().isValid() ? lvl.color() : Color::NoColor();
}


uiStratLevelSelDlg::LevelID uiStratLevelSelDlg::getID() const
{
    return selected().id();
}


void uiStratLevelSelDlg::setID( Strat::Level::ID lvlid )
{
    if ( lvlid.isValid() )
	fld_->setCurrentItem( Strat::LVLS().get(lvlid).name() );
}


void uiStratLevelSelDlg::setName( const char* nm )
{
    fld_->setCurrentItem( nm );
}


void uiStratLevelSelDlg::selCB( CallBacker* )
{
    selChange.trigger();
}


void uiStratLevelSelDlg::addItem( const char* nm, const Color& col )
{
    fld_->addItem( toUiString(nm) );
    fld_->setColorIcon( fld_->size()-1, col );
}


void uiStratLevelSelDlg::fill()
{
    const auto& lvls = Strat::LVLS();
    MonitorLock ml( lvls );
    for ( int ilvl=0; ilvl<lvls.size(); ilvl++ )
    {
	const Strat::Level lvl = lvls.getByIdx( ilvl );
	addItem( lvl.name(), lvl.color() );
    }
}


void uiStratLevelSelDlg::extChgCB( CallBacker* )
{
    const Strat::Level cursel = selected();
    fld_->setEmpty();
    fill();
    if ( !cursel.id().isInvalid() )
	fld_->setCurrentItem( cursel.name() );
}
