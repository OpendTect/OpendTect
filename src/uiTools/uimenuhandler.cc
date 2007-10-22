/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: uimenuhandler.cc,v 1.9 2007-10-22 05:30:41 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uimenuhandler.h"
#include "uicursor.h"
#include "uimenu.h"

const int uiMenuHandler::fromTree = 1;
const int uiMenuHandler::fromScene = 0;


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
    uiCursor::setOverride( uiCursor::Wait );
    createnotifier.trigger();
    uiCursor::restoreOverride();

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
    if ( subitms.isEmpty() )
	return 0;

    uiPopupMenu* menu = item ? new uiPopupMenu( uiparent_, item->text )
			     : new uiPopupMenu( uiparent_ );

    BoolTypeSet handled( subitms.size(), false );

    while ( true )
    {
	int lowest;
	int lowestitem = -1;
	for ( int idx=0; idx<subitms.size(); idx++ )
	{
	    if ( lowestitem==-1 || lowest<subitms[idx]->placement )
	    {
		if ( handled[idx] ) continue;

		lowest = subitms[idx]->placement;
		lowestitem = idx;
	    }
	}

	if ( lowestitem==-1 )
	    break;

	const MenuItem& subitm = *subitms[lowestitem];

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

