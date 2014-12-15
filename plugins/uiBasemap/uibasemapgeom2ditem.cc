/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapgeom2ditem.h"

#include "uiaction.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"

#include "basemapgeom2d.h"
#include "oduicommon.h"
#include "survgeometrytransl.h"


static IOObjContext getIOObjContext()
{
    IOObjContext ctxt( mIOObjContext(Survey::SurvGeom) );
    ctxt.fixTranslator( Survey::dgb2DSurvGeomTranslator::translKey() );
    return ctxt;
}


// uiBasemapGeom2DGroup
uiBasemapGeom2DGroup::uiBasemapGeom2DGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,getIOObjContext(),isadd)
{
    addNameField();
}


uiBasemapGeom2DGroup::~uiBasemapGeom2DGroup()
{
}


bool uiBasemapGeom2DGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapGeom2DGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );
    return res;
}


bool uiBasemapGeom2DGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );
    return res;
}


// uiBasemapGeom2DItem
const char* uiBasemapGeom2DItem::iconName() const
{ return "geom2d"; }

uiBasemapGroup* uiBasemapGeom2DItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapGeom2DGroup( p, isadd ); }

uiBasemapTreeItem* uiBasemapGeom2DItem::createTreeItem( const char* nm )
{ return new uiBasemapGeom2DTreeItem( nm ); }



// uiBasemapGeom2DTreeItem
uiBasemapGeom2DTreeItem::uiBasemapGeom2DTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapGeom2DTreeItem::~uiBasemapGeom2DTreeItem()
{}


bool uiBasemapGeom2DTreeItem::usePar( const IOPar& par )
{
    uiBasemapTreeItem::usePar( par );

    int nrobjs = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrobjs );

    while ( nrobjs < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrobjs; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( basemapobjs_.validIdx(idx) )
	{
	    mDynamicCastGet(Basemap::Geom2DObject*,obj,basemapobjs_[idx])
	    if ( obj ) obj->setMultiID( mid );
	}
	else
	{
	    Basemap::Geom2DObject* obj =
		new Basemap::Geom2DObject( mid );
	    addBasemapObject( *obj );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapGeom2DTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    mnu.insertItem( new uiAction("Show in 3D"), 1 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapGeom2DTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
    {
	BMM().edit( getFamilyID(), ID() );
    }
    else if ( mnuid==1 )
    {
	const int nrobjs = basemapobjs_.size();
	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    mDynamicCastGet(Basemap::Geom2DObject*,obj,basemapobjs_[idx])
	    if ( !obj ) continue;

	    ODMainWin()->sceneMgr().add2DLineItem( obj->getMultiID() );
	}
    }
    else
	return false;

    return true;
}
