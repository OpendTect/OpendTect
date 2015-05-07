/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemappolygonitem.h"

#include "uiaction.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uistrings.h"

#include "basemappolygon.h"
#include "oduicommon.h"
#include "picksettr.h"


static IOObjContext getIOObjContext()
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    ctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );
    return ctxt;
}


// uiBasemapPolygonGroup
uiBasemapPolygonGroup::uiBasemapPolygonGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,getIOObjContext(),isadd)
{
}


uiBasemapPolygonGroup::~uiBasemapPolygonGroup()
{
}


bool uiBasemapPolygonGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapPolygonGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );
    return res;
}


bool uiBasemapPolygonGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );
    return res;
}



// uiBasemapPolygonParentTreeItem
const char* uiBasemapPolygonParentTreeItem::iconName() const
{ return "basemap-polygon"; }



// uiBasemapPolygonItem
int uiBasemapPolygonItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapPolygonItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapPolygonGroup( p, isadd ); }

uiBasemapParentTreeItem* uiBasemapPolygonItem::createParentTreeItem()
{ return new uiBasemapPolygonParentTreeItem(ID()); }

uiBasemapTreeItem* uiBasemapPolygonItem::createTreeItem( const char* nm )
{ return new uiBasemapPolygonTreeItem( nm ); }



// uiBasemapPolygonTreeItem
uiBasemapPolygonTreeItem::uiBasemapPolygonTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapPolygonTreeItem::~uiBasemapPolygonTreeItem()
{}


bool uiBasemapPolygonTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    int nrpolygons = 1;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrpolygons );

    while ( nrpolygons < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrpolygons; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::PolygonObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::PolygonObject* obj = new Basemap::PolygonObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapPolygonTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction("Show in 3D"), 1 );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapPolygonTreeItem::handleSubMenu( int mnuid )
{
    if ( uiBasemapTreeItem::handleSubMenu(mnuid) )
	return true;

    bool handled = true;
    if ( mnuid==1 )
    {
	const int nrobjs = basemapobjs_.size();
	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    mDynamicCastGet(Basemap::PolygonObject*,obj,basemapobjs_[idx])
	    if ( !obj ) continue;

	    ODMainWin()->sceneMgr().addPickSetItem( obj->getMultiID() );
	}
    }
    else
	handled = false;

    return handled;
}


const char* uiBasemapPolygonTreeItem::parentType() const
{
    return typeid(uiBasemapPolygonParentTreeItem).name();
}
