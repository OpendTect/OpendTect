/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uibasemap.cc,v 1.9 2011-04-27 10:13:19 cvsbert Exp $";

#include "uibasemap.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiworld2ui.h"


uiBaseMapObject::uiBaseMapObject( BaseMapObject* bmo )
    : bmobject_( bmo )
    , itemgrp_(new uiGraphicsItemGroup(true))
    , transform_(0)
{
    if ( bmobject_ )
	bmobject_->changed.notify( mCB(this,uiBaseMapObject,changedCB) );
}


uiBaseMapObject::~uiBaseMapObject()
{
    delete itemgrp_;
}


void uiBaseMapObject::changedCB( CallBacker* )
{
    update();
}


void uiBaseMapObject::setTransform( const uiWorld2Ui* w2ui )
{
    transform_ = w2ui;
}


void uiBaseMapObject::update()
{
    if ( !bmobject_ ) return;

    TypeSet<Coord> crds;
    bmobject_->getPoints( -1, crds );
    
    if ( bmobject_->connectPoints(-1) == BaseMapObject::cConnect() )
    {
	if ( itemgrp_->isEmpty() )
	    itemgrp_->add( new uiLineItem() );

	mDynamicCastGet(uiLineItem*,li,itemgrp_->getUiItem(0))
	if ( !li ) return;

	uiPoint pt1 = transform_->transform( uiWorldPoint(crds[0]) );
	uiPoint pt2 = transform_->transform( uiWorldPoint(crds[1]) );
	li->setLine( pt1, pt2 );
    }
    else if ( bmobject_->connectPoints(-1) == BaseMapObject::cPolygon() )
    {
	if ( itemgrp_->isEmpty() )
	    itemgrp_->add( new uiPolygonItem() );

	mDynamicCastGet(uiPolygonItem*,itm,itemgrp_->getUiItem(0))
	if ( !itm ) return;

	TypeSet<uiPoint> pts;
	for ( int idx=0; idx<crds.size(); idx++ )
	    pts += transform_->transform( uiWorldPoint(crds[idx]) );
	itm->setPolygon( pts );
    }
    else if ( bmobject_->connectPoints(-1) == BaseMapObject::cDontConnect() )
    {
	if ( itemgrp_->isEmpty() )
	    itemgrp_->add( new uiMarkerItem(true) );

	mDynamicCastGet(uiMarkerItem*,itm,itemgrp_->getUiItem(0))
	if ( !itm ) return;

	const Color* col = bmobject_->getColor(-1);
	if ( col ) itm->setFillColor( *col );
	if ( crds.size() > 0 )
	    itm->setPos( transform_->transform( uiWorldPoint(crds[0]) ) );
    }
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
    for ( int idx=0; idx<objects_.size(); idx++ )
	view_.scene().removeItem( objects_[idx]->itemGrp() );

    deepErase( objects_ );
    delete &view_;
    delete &w2ui_;
}


void uiBaseMap::addObject( BaseMapObject* obj )
{
    uiBaseMapObject* uiobj = new uiBaseMapObject( obj );
    if ( !uiobj )
	return;

    addObject( uiobj );
}


void uiBaseMap::addObject( uiBaseMapObject* uiobj )
{
    view_.scene().addItem( uiobj->itemGrp() );
    objects_ += uiobj;
    uiobj->setTransform( &w2ui_ );
}


void uiBaseMap::removeObject( const BaseMapObject* obj )
{
    int index = -1;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->bmobject_==obj )
	{
	    index = idx;
	    break;
	}
    }

    if ( index==-1 )
    {
	pErrMsg( "Base map object not found" );
    }

    const uiBaseMapObject* uiobj = objects_[index];

    view_.scene().removeItem( objects_[index]->itemGrp() );
    delete objects_.remove( index );
}


void uiBaseMap::reDraw( bool )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->update();
}
