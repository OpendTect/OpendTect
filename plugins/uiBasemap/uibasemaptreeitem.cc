/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uibasemaptreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uistrings.h"
#include "uitreeview.h"

#include "basemapwell.h"

mImplFactory( uiBasemapTreeItem, uiBasemapTreeItem::factory )


// uiBasemapTreeTop
uiBasemapTreeTop::uiBasemapTreeTop( uiTreeView* tv, BaseMap& bm )
    : uiTreeTopItem(tv)
    , basemap_(bm)
{
}


uiBasemapTreeTop::~uiBasemapTreeTop()
{}


void uiBasemapTreeTop::add( BaseMapObject& bmo )
{ basemap_.addObject( &bmo ); }


// uiBasemapTreeItem
uiBasemapTreeItem::~uiBasemapTreeItem()
{
}

int uiBasemapTreeItem::uiTreeViewItemType() const
{ return uiTreeViewItem::CheckBox; }

uiODApplMgr& uiBasemapTreeItem::applMgr()
{ return ODMainWin()->applMgr(); }

void uiBasemapTreeItem::fillPar(IOPar &) const
{}

bool uiBasemapTreeItem::usePar(const IOPar &)
{ return true; }


// uiBasemapWellTreeItem
bool uiBasemapWellTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sAdd(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapWellTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	ObjectSet<MultiID> wellids;
	applMgr().selectWells( wellids );
	if ( wellids.isEmpty() )
	    return false;

	for ( int idx=0; idx<wellids.size(); idx++ )
	{
	    Basemap::WellObject* obj = new Basemap::WellObject( *wellids[idx] );
	    mDynamicCastGet(uiBasemapTreeTop*,tt,parent_);
	    tt->add( *obj );
	    obj->updateGeometry();
	}
    }
    else
	return false;

    return true;
}

