/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene / Bruno
 Date:          July 2007 / Sept 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistratlvllist.cc,v 1.14 2012-07-04 10:36:06 cvsbruno Exp $";

#include "uistratlvllist.h"

#include "bufstringset.h"
#include "uimenu.h"
#include "uimsg.h"
#include "randcolor.h"
#include "stratlevel.h"
#include "uistratutildlgs.h"

static const char* sNoLevelTxt      = "--- None ---";

uiStratLvlList::uiStratLvlList( uiParent* p )
    : uiLabeledListBox(p,"Regional markers",false,uiLabeledListBox::AboveMid)
    , islocked_(false)
{
    box()->setStretch( 2, 2 );
    box()->setFieldWidth( 10 );
    box()->rightButtonClicked.notify( mCB(this,uiStratLvlList,rClickLvlCB));

    setLevels();
}


void uiStratLvlList::setLevels()
{
    Strat::LevelSet& levelset = Strat::eLVLS();

    levelset.levelChanged.notify( mCB(this,uiStratLvlList,fill) );
    levelset.levelAdded.notify( mCB(this,uiStratLvlList,fill) );
    levelset.levelToBeRemoved.notify( mCB(this,uiStratLvlList,removeLvl) );

    fill(0);
}


uiStratLvlList::~uiStratLvlList()
{
    Strat::LevelSet& levelset = Strat::eLVLS();
    levelset.levelChanged.remove( mCB(this,uiStratLvlList,fill) );
    levelset.levelAdded.remove( mCB(this,uiStratLvlList,fill) );
    levelset.levelToBeRemoved.remove( mCB(this,uiStratLvlList,removeLvl) );
}


void uiStratLvlList::rClickLvlCB( CallBacker* )
{
    if ( islocked_ ) return;

    int curit = box()->currentItem();
    Strat::LevelSet& levelset = Strat::eLVLS();
    uiPopupMenu mnu( this, "Action" );
    mnu.insertItem( new uiMenuItem("Create &New ..."), 0 );
    if ( curit>-1 && !box()->isPresent( sNoLevelTxt ) )
    {
	mnu.insertItem( new uiMenuItem("&Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("&Remove"), 2 );
	mnu.insertItem( new uiMenuItem("&Remove All"), 3 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid<0 || mnuid>3 ) return;
    if ( mnuid != 2 && mnuid != 3 )
	editLevel( mnuid ? false : true );
    else if ( mnuid == 2 )
    {
	const char* lvlnm = box()->getText();
	if ( !levelset.isPresent( lvlnm ) ) return;
	const Strat::Level& lvl = *levelset.get( lvlnm );
	levelset.remove( lvl.id() );
    }
    else if ( mnuid == 3 )
    {
	BufferString msg ( "This will remove all the markers " ); 
	msg += "present in the list";
	msg += ", do you want to continue ?";
	if ( uiMSG().askGoOn(msg) )
	{
	    for ( int idx=levelset.size()-1; idx>=0; idx-- )
	    {
		const Strat::Level* lvl = levelset.levels()[idx];
		if ( lvl->id() >= 0 )
		    levelset.remove( lvl->id() );
	    }
	}
    }
}


void uiStratLvlList::removeLvl( CallBacker* cb )
{
    mDynamicCastGet(Strat::LevelSet*,lvlset,cb)
    if ( !lvlset ) pErrMsg( "Can't find levelSet" );
    const int lvlidx = lvlset->notifLvlIdx();
    if ( lvlset->levels().validIdx( lvlidx ) )
    {
	const Strat::Level* lvl = lvlset->levels()[lvlidx];
	if ( box()->isPresent( lvl->name() ) )
	    box()->removeItem( box()->indexOf( lvl->name() ) );
    }
    if ( box()->isEmpty() )
	box()->addItem( sNoLevelTxt );
}


void uiStratLvlList::fill( CallBacker* )
{
    box()->setEmpty();
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
	box()->addItem( lvlnms[idx]->buf(), lvlcolors[idx] );

    if ( box()->isEmpty() )
	box()->addItem( sNoLevelTxt );
}


void uiStratLvlList::editLevel( bool create )
{
    Strat::LevelSet& lvls = Strat::eLVLS();
    BufferString oldnm = create ? "" : box()->getText();
    uiStratLevelDlg newlvldlg( this );
    Strat::Level* lvl = create ? 0 : lvls.get( oldnm ); 
    if ( lvl ) newlvldlg.setLvlInfo( oldnm, lvl->color() );
    if ( newlvldlg.go() )
    {
	BufferString nm; Color col;
	newlvldlg.getLvlInfo( nm, col );
	if ( !nm.isEmpty() && strcmp(oldnm,nm) && lvls.isPresent( nm ) )
	    { uiMSG().error("Level name is empty or already exists"); return; }
	if ( create )
	    lvl = lvls.add( nm.buf(), col );
	else if ( lvl )
	{
	    lvl->setName( nm.buf() );
	    lvl->setColor( col );
	}
	lvls.store( Repos::Survey );
    }
}

