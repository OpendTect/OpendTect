/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2003
 RCS:           $Id: uimenuhandler.cc,v 1.1 2005-06-28 15:58:12 cvskris Exp $
________________________________________________________________________

-*/


#include "uimenuhandler.h"
#include "uicursor.h"
#include "uimenu.h"

const int uiMenuHandler::fromTree = 1;
const int uiMenuHandler::fromScene = 0;


uiMenuHandler::uiMenuHandler( uiParent* parent_, int ni )
    : parent( parent_ )
    , id_( ni )
    , createnotifier( this )
    , handlenotifier( this )
    , positionxyz( Coord3::udf() )
{
    mRefCountConstructor;
}


uiMenuHandler::~uiMenuHandler()
{}


bool uiMenuHandler::isHandled() const
{ return ishandled; }


void uiMenuHandler::setIsHandled( bool yn )
{ ishandled = yn; }


bool uiMenuHandler::executeMenu( int menutype_, const TypeSet<int>* path_ )
{
    ref();		//makes sure that object is not removed during a cb
    freeid = 0;
    menutype = menutype_;
    path = path_;

    uiPopupMenu menu( parent );

    uiCursor::setOverride( uiCursor::Wait );
    createnotifier.trigger();
    uiCursor::restoreOverride();


    if ( !menus.size() && !items.size() )
	return true;

    while ( menus.size() || items.size() )
    {
	int lowest;
	bool found = false;
	for ( int idx=0; idx<menus.size(); idx++ )
	{
	    if ( !found || lowest<menuplacement[idx] )
	    {
		lowest = menuplacement[idx];
		found = true; 
	    }
	}
	    
	for ( int idx=0; idx<items.size(); idx++ )
	{
	    if ( !found || lowest<itemplacement[idx] )
	    {
		lowest = itemplacement[idx];
		found = true; 
	    }
	}


	for ( int idx=0; idx<menus.size(); idx++ )
	{
	    if ( lowest==menuplacement[idx] )
	    {
		menu.insertItem( menus[idx], -1 );
		menus.remove(idx);
		menuplacement.remove(idx);
	    }
	}
	   

	for ( int idx=0; idx<items.size(); idx++ )
	{
	    if ( lowest==itemplacement[idx] )
	    {
		menu.insertItem( items[idx], itemids[idx] );
		items.remove(idx);
		itemids.remove(idx);
		itemplacement.remove(idx);
	    }
	}
    }
	    
    const int selection = menu.exec();

    if ( selection==-1 )
    {
	unRef();
	return true;
    }

    ishandled = false;
    handlenotifier.trigger( selection, *this );
    unRef();
    return true;
}


int uiMenuHandler::addItem( uiMenuItem* itm, int placementidx)
{
    items += itm;
    itemids += getFreeID();
    itemplacement += placementidx;
    return freeid-1;
}


void  uiMenuHandler::addSubMenu( uiPopupMenu* itm, int placementidx)
{
    menus += itm;
    menuplacement += placementidx;
}


uiPopupMenu* uiMenuHandler::getMenu( const char* name )
{
    for ( int idx=0; idx<menus.size(); idx++ )
    {
	if ( !strcmp(menus[idx]->name(), name ) )
	    return menus[idx];
    }

    return 0;
}
