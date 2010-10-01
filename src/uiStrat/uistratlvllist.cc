/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene / Bruno
 Date:          July 2007 / Sept 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvllist.cc,v 1.6 2010-10-01 15:04:20 cvsbruno Exp $";

#include "uistratlvllist.h"

#include "bufstringset.h"
#include "uimenu.h"
#include "uimsg.h"
#include "randcolor.h"
#include "stratlevel.h"
#include "uistratutildlgs.h"

static const char* sNoLevelTxt      = "--- Empty ---";

uiStratLvlList::uiStratLvlList( uiParent* p )
    : uiLabeledListBox(p,"Regional markers",false,uiLabeledListBox::AboveMid)
    , levelset_(Strat::eLVLS())
{
    box()->setStretch( 2, 2 );
    box()->setFieldWidth( 10 );
    box()->rightButtonClicked.notify( mCB(this,uiStratLvlList,rClickLvlCB));
    levelset_.levelChanged.notify( mCB(this,uiStratLvlList,fill) );
    levelset_.levelAdded.notify( mCB(this,uiStratLvlList,fill) );
    levelset_.levelToBeRemoved.notify( mCB(this,uiStratLvlList,fill) );

    fill(0);
}


uiStratLvlList::~uiStratLvlList()
{
    levelset_.levelChanged.remove( mCB(this,uiStratLvlList,fill) );
    levelset_.levelAdded.remove( mCB(this,uiStratLvlList,fill) );
    levelset_.levelToBeRemoved.remove( mCB(this,uiStratLvlList,fill) );
}


void uiStratLvlList::rClickLvlCB( CallBacker* )
{
    int curit = box()->currentItem();
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
	if ( !levelset_.isPresent( lvlnm ) ) return;
	const Strat::Level& lvl = *levelset_.get( lvlnm );
	levelset_.remove( lvl.id() );
    }
    else if ( mnuid == 3 )
    {
	BufferString msg ( "This will remove all the markers " ); 
	msg += "present in the list";
	msg += ", do you want to continue ?";
	if ( uiMSG().askGoOn(msg) )
	    levelset_.setEmpty();
    }
}


void uiStratLvlList::fill( CallBacker* )
{
    box()->empty();
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

    if ( !lvlnms.size() )
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

