/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene / Bruno
 Date:          July 2007 / Sept 2010
________________________________________________________________________

-*/

#include "uistratlvllist.h"

#include "bufstringset.h"
#include "randcolor.h"
#include "stratlevel.h"

#include "uibuttongroup.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistratutildlgs.h"
#include "uitoolbutton.h"


uiStratLvlList::uiStratLvlList( uiParent* p )
    : uiListBox(p,Setup(OD::ChooseOnlyOne,tr("Regional markers"),
			uiListBox::AboveMid))
    , islocked_(false)
    , anychange_(false)
{
    setStretch( 2, 2 );
    setFieldWidth( 15 );
    doubleClicked.notify( mCB(this,uiStratLvlList,editCB) );

    uiButtonGroup* grp = new uiButtonGroup( this, "Tools", OD::Vertical );
    grp->attach( rightTo, box() );
    new uiToolButton( grp, "addnew", uiStrings::phrCreate(uiStrings::sNew()),
		      mCB(this,uiStratLvlList,addCB) );
    new uiToolButton( grp, "edit", uiStrings::sEdit(),
                      mCB(this,uiStratLvlList,editCB) );
    new uiToolButton( grp, "remove", uiStrings::sRemove(),
		      mCB(this,uiStratLvlList,removeCB) );
    new uiToolButton( grp, "clear", uiStrings::phrRemove(uiStrings::sAll()),
		      mCB(this,uiStratLvlList,removeAllCB) );

    setLevels();
    setHAlignObj( box() );
    setHCenterObj( box() );
}


void uiStratLvlList::setLevels()
{
    Strat::LevelSet& levelset = Strat::eLVLS();
    levelset.levelChanged.notifyIfNotNotified( mCB(this,uiStratLvlList,fill) );
    levelset.levelAdded.notifyIfNotNotified( mCB(this,uiStratLvlList,fill) );
    levelset.levelToBeRemoved.notifyIfNotNotified(
	    mCB(this,uiStratLvlList,removeLvl) );

    fill(0);
}


uiStratLvlList::~uiStratLvlList()
{
    Strat::LevelSet& levelset = Strat::eLVLS();
    levelset.levelChanged.remove( mCB(this,uiStratLvlList,fill) );
    levelset.levelAdded.remove( mCB(this,uiStratLvlList,fill) );
    levelset.levelToBeRemoved.remove( mCB(this,uiStratLvlList,removeLvl) );
}


#define mCheckLocked            if ( checkLocked() ) return;

#define mCheckEmptyList \
    if ( isPresent(sNoLevelTxt()) ) \
	return;


bool uiStratLvlList::checkLocked() const
{
    if ( islocked_ )
    {
	uiMSG().error( tr("Cannot change Stratigraphy because it is locked") );
	return true;
    }

    return false;
}


void uiStratLvlList::editCB( CallBacker* )
{ mCheckLocked; mCheckEmptyList; editLevel( false );  }

void uiStratLvlList::addCB( CallBacker* )
{ mCheckLocked; editLevel( true ); }


void uiStratLvlList::removeCB( CallBacker* )
{
    mCheckLocked; mCheckEmptyList;
    uiString msg = tr("This will remove the selected marker.");
    if ( !uiMSG().askRemove(msg) ) return;

    Strat::LevelSet& levelset = Strat::eLVLS();
    const char* lvlnm = getText();
    if ( !levelset.isPresent(lvlnm) ) return;

    const Strat::Level& lvl = *levelset.get( lvlnm );
    levelset.remove( lvl.id() ) ;
    anychange_ = true;
}


void uiStratLvlList::removeAllCB( CallBacker* )
{
    mCheckLocked; mCheckEmptyList;
    uiString msg = tr("This will remove all the markers present in the list,"
		      " do you want to continue ?");
    if ( !uiMSG().askRemove(msg) ) return;

    Strat::LevelSet& levelset = Strat::eLVLS();
    for ( int idx=levelset.size()-1; idx>=0; idx-- )
    {
	const Strat::Level* lvl = levelset.levels()[idx];
	if ( lvl->id() >= 0 )
	{
	    levelset.remove( lvl->id() );
	    anychange_ = true;
	}
    }
}


void uiStratLvlList::removeLvl( CallBacker* cb )
{
    mDynamicCastGet(Strat::LevelSet*,lvlset,cb)
    if ( !lvlset )
	{ pErrMsg( "cb null or not a LevelSet" ); return; }
    const int lvlidx = lvlset->notifLvlIdx();
    if ( lvlset->levels().validIdx( lvlidx ) )
    {
	const Strat::Level* lvl = lvlset->levels()[lvlidx];
	if ( isPresent( lvl->name() ) )
	    removeItem( indexOf( lvl->name() ) );
    }
    if ( isEmpty() )
	addItem( toUiString("--- %1 ---").arg(uiStrings::sNone()) );
}


void uiStratLvlList::fill( CallBacker* )
{
    setEmpty();
    BufferStringSet lvlnms;
    TypeSet<Color> lvlcolors;

    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
    {
	const Strat::Level& lvl = *lvls.levels()[idx];
	lvlnms.add( lvl.name() );
	lvlcolors += lvl.color();
    }
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	addItem( toUiString(lvlnms[idx]->buf()), lvlcolors[idx] );

    if ( isEmpty() )
	addItem( toUiString("--- %1 ---").arg(uiStrings::sNone()) );
}


void uiStratLvlList::editLevel( bool create )
{
    Strat::LevelSet& lvls = Strat::eLVLS();
    BufferString oldnm = create ? "" : getText();
    uiStratLevelDlg newlvldlg( this );
    newlvldlg.setCaption( create ? tr("Create level") : tr("Edit level") );
    Strat::Level* lvl = create ? 0 : lvls.get( oldnm );
    if ( lvl ) newlvldlg.setLvlInfo( oldnm, lvl->color() );
    if ( newlvldlg.go() )
    {
	BufferString nm; Color col;
	newlvldlg.getLvlInfo( nm, col );
	if ( !nm.isEmpty() && oldnm!=nm && lvls.isPresent( nm ) )
	    { uiMSG().error(tr("Level name is empty or already exists"));
              return; }
	if ( create )
	    lvl = lvls.add( nm.buf(), col );
	else if ( lvl )
	{
	    lvl->setName( nm.buf() );
	    lvl->setColor( col );
	}

	anychange_ = true;
    }
}
