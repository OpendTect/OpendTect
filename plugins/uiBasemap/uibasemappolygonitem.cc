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
#include "uiodapplmgr.h"
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
uiBasemapPolygonGroup::uiBasemapPolygonGroup( uiParent* p )
    : uiBasemapIOObjGroup(p,getIOObjContext())
{
    addNameField();
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


// uiBasemapPolygonItem
const char* uiBasemapPolygonItem::iconName() const
{ return "polygon"; }

uiBasemapGroup* uiBasemapPolygonItem::createGroup( uiParent* p )
{ return new uiBasemapPolygonGroup( p ); }

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

    int nrpolygons = 0;
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
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapPolygonTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	BMM().edit( getFamilyID(), ID() );
    }
    else
	return false;

    return true;
}

