/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uibasemap.cc,v 1.2 2010-07-29 10:57:03 cvsraman Exp $";

#include "uibasemap.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiworld2ui.h"


uiBaseMapObject::uiBaseMapObject( const char* nm )
    : BaseMapObject(nm)
    , itemgrp_(new uiGraphicsItemGroup(true))
    , transform_(0)
{}


void uiBaseMapObject::setTransform( const uiWorld2Ui* w2ui )
{
    transform_ = w2ui;
}


uiBaseMap::uiBaseMap( uiParent* p )
    : uiGroup(p,"Basemap")
    , view_(*new uiGraphicsView(this,"Basemap"))
    , w2ui_(*new uiWorld2Ui)
{
    view_.reSize.notify( mCB(this,uiBaseMap,reSizeCB) );
}


uiBaseMap::~uiBaseMap()
{
    delete &view_;
    deepErase( objects_ );
}


void uiBaseMap::addObject( BaseMapObject* obj )
{
    mDynamicCastGet(uiBaseMapObject*,uiobj,obj)
    if ( !uiobj )
	return;

    view_.scene().addItem( uiobj->itemGrp() );
    objects_ += uiobj;
    uiobj->setTransform( &w2ui_ );
}


void uiBaseMap::removeObject( const BaseMapObject* obj )
{
    mDynamicCastGet(const uiBaseMapObject*,uiobj,obj)
    if ( !uiobj )
	return;

    const int index = objects_.indexOf( uiobj );
    if ( index >= 0 )
	delete objects_.remove( index );
}


void uiBaseMap::reDraw()
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->updateGeometry();
}
