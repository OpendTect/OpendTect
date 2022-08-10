/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2003
________________________________________________________________________

-*/


#include "uimenuhandler.h"

#include "uimenu.h"
#include "uitoolbar.h"

#include "mousecursor.h"

int uiMenuHandler::fromTree()	{ return 1; }
int uiMenuHandler::fromScene()	{ return 0; }


uiMenuHandler::uiMenuHandler( uiParent* uiparent, int ni )
    : MenuHandler( ni )
    , uiparent_( uiparent )
    , positionxyz_( Coord3::udf() )
{ }


bool uiMenuHandler::executeMenu()
{
    menutype_ = -1; path_ = 0;
    return executeMenuInternal();
}


bool uiMenuHandler::executeMenuInternal()
{
    //makes sure that object is not removed during a cb
    RefMan<uiMenuHandler> reffer(this);

    removeItems();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    initnotifier.trigger();
    createnotifier.trigger();
    MouseCursorManager::restoreOverride();

    PtrMan<uiMenu> menu = createMenu( getItems() );
    if ( !menu ) return true;

    const int selection = menu->exec();

    if ( selection==-1 )
    { return true; }

    ishandled_ = false;
    handlenotifier.trigger( selection, *this );

    executeQueue();
    return true;
}


bool uiMenuHandler::executeMenu( int menutype, const TypeSet<int>* path )
{
    menutype_ = menutype;
    path_ = path;
    return executeMenuInternal();
}


uiMenu* uiMenuHandler::createMenu( const ObjectSet<MenuItem>& subitms,
					const MenuItem* item )
{
    ObjectSet<const MenuItem> validsubitms;
    for ( int idx=0; idx<subitms.size(); idx++ )
    {
	if ( subitms[idx]->id >= 0 )
	    validsubitms += subitms[idx];
    }

    if ( validsubitms.isEmpty() )
	return 0;

    uiMenu* menu = item ? new uiMenu( uiparent_, item->text )
			: new uiMenu( uiparent_ );

    BoolTypeSet handled( validsubitms.size(), false );

    while ( true )
    {
	int lowest = mUdf(int);
	int lowestitem = -1;
	for ( int idx=0; idx<validsubitms.size(); idx++ )
	{
	    if ( lowestitem==-1 || lowest<validsubitms[idx]->placement )
	    {
		if ( handled[idx] ) continue;

		lowest = validsubitms[idx]->placement;
		lowestitem = idx;
	    }
	}

	if ( lowestitem==-1 )
	    break;

	const MenuItem& subitm = *validsubitms[lowestitem];

	if ( subitm.nrItems() )
	{
	    uiMenu* submenu = createMenu( subitm.getItems(), &subitm );
	    if ( submenu )
	    {
		menu->addMenu( submenu );
		submenu->setEnabled( subitm.enabled );
		submenu->setCheckable( subitm.checkable );
		submenu->setChecked( subitm.checked );
	    }
	}
	else
	{
	    uiAction* mnuitem = new uiAction(subitm.text);
	    menu->insertAction( mnuitem, subitm.id );
	    mnuitem->setEnabled( subitm.enabled );
	    mnuitem->setCheckable( subitm.checkable );
	    mnuitem->setChecked( subitm.checked );
	    if ( !subitm.iconfnm.isEmpty() )
		mnuitem->setIcon( subitm.iconfnm );
	}

	handled[lowestitem] = true;
    }

    return menu;
}



uiTreeItemTBHandler::uiTreeItemTBHandler( uiParent* uiparent )
    : MenuHandler(-1)
    , uiparent_(uiparent)
{
    tb_ = new uiToolBar( uiparent_, uiStrings::phrJoinStrings(tr("Item"),
			 uiStrings::sTools()), uiToolBar::Top, true );
    tb_->buttonClicked.notify( mCB(this,uiTreeItemTBHandler,butClickCB) );
    handleEmpty();
}


void uiTreeItemTBHandler::handleEmpty()
{
    if ( nrItems() > 0 )
	return;

    tb_->addButton( "base_icon", tr("No tools available"), CallBack() );
}


void uiTreeItemTBHandler::addButtons()
{
    removeItems();
    tb_->clear();
    createnotifier.trigger();
    for ( int idx=0; idx<nrItems(); idx++ )
	tb_->addButton( *getItem(idx) );

    handleEmpty();
}


void uiTreeItemTBHandler::butClickCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,butid,cb);
    if ( butid == -1 ) return;

    ishandled_ = false;
    handlenotifier.trigger( butid, this );
    executeQueue();
}
