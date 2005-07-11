/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: uimenuhandler.cc,v 1.2 2005-07-11 21:20:19 cvskris Exp $
________________________________________________________________________

-*/


#include "uimenuhandler.h"
#include "uicursor.h"
#include "uimenu.h"

const int uiMenuHandler::fromTree = 1;
const int uiMenuHandler::fromScene = 0;


uiMenuHandler::uiMenuHandler( uiParent* parent_, int ni )
    : MenuHandler( ni )
    , parent( parent_ )
    , positionxyz( Coord3::udf() )
{ }


bool uiMenuHandler::executeMenu( int menutype_, const TypeSet<int>* path_ )
{
    //makes sure that object is not removed during a cb
    RefMan<uiMenuHandler> reffer(this);
    
    freeid = 0;
    menutype = menutype_;
    path = path_;

    items.erase();
    uiCursor::setOverride( uiCursor::Wait );
    createnotifier.trigger();
    uiCursor::restoreOverride();

    PtrMan<uiPopupMenu> menu = createMenu( items );
    if ( !menu ) return true;

    const int selection = menu->exec();

    if ( selection==-1 )
    { return true; }

    ishandled = false;
    handlenotifier.trigger( selection, *this );

    return true;
}


uiPopupMenu* uiMenuHandler::createMenu( const ObjectSet<MenuItem>& subitms,
					const MenuItem* item )
{
    if ( !subitms.size() )
	return 0;

    uiPopupMenu* menu = item ? new uiPopupMenu( parent, item->text )
			     : new uiPopupMenu( parent );
    if ( item )
    {
	menu->setEnabled( item->enabled );
	menu->setChecked( item->checked );
    }

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

	const MenuItem& item = *subitms[lowestitem];

	if ( item.nrItems() )
	{
	    uiPopupMenu* submenu = createMenu( item.getItems(), &item );
	    if ( submenu ) menu->insertItem( submenu, item.id );
	}
	else
	{
	    uiMenuItem* mnuitem = item.cb.cbObj() && item.cb.cbFn() 
		? new uiMenuItem(item.text,item.cb)
		: new uiMenuItem(item.text);

	    menu->insertItem( mnuitem, item.id );
	    mnuitem->setEnabled( item.enabled );
	    mnuitem->setChecked( item.checked );
	}

	handled[lowestitem] = true;
    }

    return menu;
}

