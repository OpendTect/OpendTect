/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		March 2015
________________________________________________________________________

-*/
#include "uibasemapfaultitem.h"

static const char* rcsID mUsedVar = "$Id: $";


#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"


// uiBasemapFaultParentTreeItem
bool uiBasemapFaultParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sAdd(true)), 0 );
    const int mnuid = mnu.exec();
    if ( mnuid==0 )
    {
	uiMSG().message( "Not implemented yet" );
    }

    return true;
}


const char* uiBasemapFaultParentTreeItem::iconName() const
{ return "basemap-fault"; }



// uiBasemapFaultItem
int uiBasemapFaultItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapFaultItem::createGroup( uiParent*, bool )
{ return 0; }

uiBasemapParentTreeItem* uiBasemapFaultItem::createParentTreeItem()
{ return new uiBasemapFaultParentTreeItem(ID()); }

uiBasemapTreeItem* uiBasemapFaultItem::createTreeItem( const char* nm )
{ return new uiBasemapFaultTreeItem( nm ); }



// uiBasemapFaultTreeItem
uiBasemapFaultTreeItem::uiBasemapFaultTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapFaultTreeItem::~uiBasemapFaultTreeItem()
{
}


bool uiBasemapFaultTreeItem::usePar( const IOPar& par )
{
    return true;
}


bool uiBasemapFaultTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapFaultTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapFaultTreeItem::parentType() const
{
    return typeid(uiBasemapFaultParentTreeItem).name();
}

