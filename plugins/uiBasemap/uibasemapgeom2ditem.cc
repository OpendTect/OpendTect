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
#include "uisellinest.h"
#include "uistrings.h"

#include "basemapgeom2d.h"
#include "oduicommon.h"
#include "survgeometrytransl.h"


static IOObjContext getIOObjContext()
{
    IOObjContext ctxt( mIOObjContext(SurvGeom2D) );
    return ctxt;
}


// uiBasemapGeom2DGroup
uiBasemapGeom2DGroup::uiBasemapGeom2DGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,getIOObjContext(),isadd)
{
    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst;
    lsfld_ = new uiSelLineStyle( this, lst, stu );
    lsfld_->attach( alignedBelow, uiBasemapIOObjGroup::lastObject() );

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


bool uiBasemapGeom2DGroup::fillItemPar( int idx, IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillItemPar( idx, par );

    BufferString lsstr;
    lsfld_->getStyle().toString( lsstr );
    par.set( sKey::LineStyle(), lsstr );

    return res;
}


bool uiBasemapGeom2DGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls; ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    return res;
}


uiObject* uiBasemapGeom2DGroup::lastObject()
{ return lsfld_->attachObj(); }



// uiBasemapGeom2DItem
int uiBasemapGeom2DItem::defaultZValue() const
{ return 100; }

const char* uiBasemapGeom2DItem::iconName() const
{ return "basemap-geom2d"; }

uiBasemapGroup* uiBasemapGeom2DItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapGeom2DGroup( p, isadd ); }

uiBasemapParentTreeItem* uiBasemapGeom2DItem::createParentTreeItem()
{ return new uiBasemapGeom2DParentTreeItem(ID()); }

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
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    int nrobjs = 0;
    par.get( uiBasemapGroup::sKeyNrObjs(), nrobjs );

    while ( nrobjs < basemapobjs_.size() )
	delete removeBasemapObject( *basemapobjs_[0] );

    for ( int idx=0; idx<nrobjs; idx++ )
    {
	MultiID mid;
	if ( !par.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    continue;

	if ( !basemapobjs_.validIdx(idx) )
	{
	    Basemap::Geom2DObject* obj = new Basemap::Geom2DObject();
	    addBasemapObject( *obj );
	}

	mDynamicCastGet(Basemap::Geom2DObject*,obj,basemapobjs_[idx])
	if ( !obj ) return false;

	if ( hasParChanged(prevpar,par,uiBasemapGroup::sKeyNrObjs()) )
	{
	    // if the number of objects is different, everything needs to be
	    // redraw So...
	    obj->setLineStyle( 0, ls );
	    obj->setMultiID( mid );
	    obj->updateGeometry();
	    continue;
	}

	if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
	    obj->setLineStyle( 0, ls );

	if ( hasSubParChanged(prevpar,par,sKey::ID()) )
	{
	    obj->setMultiID( mid );
	    obj->updateGeometry();
	}
    }

    return true;
}


bool uiBasemapGeom2DTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction("Show in 3D"), 1 );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapGeom2DTreeItem::handleSubMenu( int mnuid )
{
    if ( uiBasemapTreeItem::handleSubMenu(mnuid) )
	return true;

    bool handled = true;
    if ( mnuid==1 )
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
	handled = false;

    return handled;
}


const char* uiBasemapGeom2DTreeItem::parentType() const
{
    return typeid(uiBasemapGeom2DParentTreeItem).name();
}
