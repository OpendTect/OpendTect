/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemaprandomlineitem.h"

#include "uiaction.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uisellinest.h"
#include "uistrings.h"

#include "basemaprandomline.h"
#include "oduicommon.h"
#include "randomlinetr.h"


// uiBasemapRandomLineGroup
uiBasemapRandomLineGroup::uiBasemapRandomLineGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,mIOObjContext(RandomLineSet),isadd)
{
    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst;
    lsfld_ = new uiSelLineStyle( this, lst, stu );
    if ( uiBasemapIOObjGroup::lastObject() )
	lsfld_->attach( alignedBelow, uiBasemapIOObjGroup::lastObject() );
}


uiBasemapRandomLineGroup::~uiBasemapRandomLineGroup()
{
}


bool uiBasemapRandomLineGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


bool uiBasemapRandomLineGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );

    for ( int idx=0; idx<nrItems(); idx++ )
    {
	IOPar ipar;
	BufferString lsstr;
	lsfld_->getStyle().toString( lsstr );
	ipar.set( sKey::LineStyle(), lsstr );

	par.mergeComp( ipar, IOPar::compKey(sKeyItem(),idx) );
    }

    return res;
}


bool uiBasemapRandomLineGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls; ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    return res;
}


uiObject* uiBasemapRandomLineGroup::lastObject()
{ return lsfld_->attachObj(); }



// uiBasemapRandomLineParentTreeItem
const char* uiBasemapRandomLineParentTreeItem::iconName() const
{ return "basemap-randomline"; }



// uiBasemapRandomLineItem
int uiBasemapRandomLineItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapRandomLineItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapRandomLineGroup( p, isadd ); }

uiBasemapParentTreeItem* uiBasemapRandomLineItem::createParentTreeItem()
{ return new uiBasemapRandomLineParentTreeItem( ID() ); }

uiBasemapTreeItem* uiBasemapRandomLineItem::createTreeItem( const char* nm )
{ return new uiBasemapRandomLineTreeItem( nm ); }



// uiBasemapRandomLineTreeItem
uiBasemapRandomLineTreeItem::uiBasemapRandomLineTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{}


uiBasemapRandomLineTreeItem::~uiBasemapRandomLineTreeItem()
{}


bool uiBasemapRandomLineTreeItem::usePar( const IOPar& par )
{
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    MultiID mid;
    if ( !par.get(sKey::ID(),mid) )
	return false;

    if ( basemapobjs_.isEmpty() )
	addBasemapObject( *new Basemap::RandomLineObject() );

    mDynamicCastGet(Basemap::RandomLineObject*,obj,basemapobjs_[0])
    if ( !obj ) return false;

    if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
	obj->setLineStyle( 0, ls );

    if ( hasSubParChanged(prevpar,par,sKey::ID()) )
    {
	obj->setMultiID( mid );
	obj->updateGeometry();
    }

    return true;
}


bool uiBasemapRandomLineTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapRandomLineTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapRandomLineTreeItem::parentType() const
{ return typeid(uiBasemapRandomLineParentTreeItem).name(); }
