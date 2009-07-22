/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimenuhandler.cc,v 1.14 2009-07-22 16:01:42 cvsbert Exp $";


#include "uimenuhandler.h"
#include "mousecursor.h"
#include "uimenu.h"

const int uiMenuHandler::fromTree()	{ return 1; }
const int uiMenuHandler::fromScene()	{ return 0; }


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
    
    freeid_ = 0;
    removeItems();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    createnotifier.trigger();
    MouseCursorManager::restoreOverride();

    PtrMan<uiPopupMenu> menu = createMenu( getItems() );
    if ( !menu ) return true;

    const int selection = menu->exec();

    if ( selection==-1 )
    { return true; }

    ishandled_ = false;
    handlenotifier.trigger( selection, *this );

    return true;
}

    
bool uiMenuHandler::executeMenu( int menutype, const TypeSet<int>* path )
{
    menutype_ = menutype; path_ = path;
    return executeMenuInternal();
}


uiPopupMenu* uiMenuHandler::createMenu( const ObjectSet<MenuItem>& subitms,
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

    uiPopupMenu* menu = item ? new uiPopupMenu( uiparent_, item->text )
			     : new uiPopupMenu( uiparent_ );

    BoolTypeSet handled( validsubitms.size(), false );

    while ( true )
    {
	int lowest;
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
	    uiPopupMenu* submenu = createMenu( subitm.getItems(), &subitm );
	    if ( submenu )
	    {
		menu->insertItem( submenu, subitm.id );
		submenu->setEnabled( subitm.enabled );
		submenu->setCheckable( subitm.checkable );
		submenu->setChecked( subitm.checked );
	    }
	}
	else
	{
	    uiMenuItem* mnuitem = new uiMenuItem(subitm.text);
	    menu->insertItem( mnuitem, subitm.id );
	    mnuitem->setEnabled( subitm.enabled );
	    mnuitem->setCheckable( subitm.checkable );
	    mnuitem->setChecked( subitm.checked );
	}

	handled[lowestitem] = true;
    }

    return menu;
}

