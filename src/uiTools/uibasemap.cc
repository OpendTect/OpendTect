/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemap.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uipixmap.h"
#include "uirgbarray.h"
#include "uiworld2ui.h"

#include "survinfo.h"


uiBaseMapObject::uiBaseMapObject( BaseMapObject* bmo )
    : bmobject_( bmo )
    , itemgrp_(*new uiGraphicsItemGroup(true))
    , transform_(0)
    , changed_(false)
{
    if ( bmobject_ )
	bmobject_->changed.notify( mCB(this,uiBaseMapObject,changedCB) );
}


uiBaseMapObject::~uiBaseMapObject()
{
    delete &itemgrp_;
}


const char* uiBaseMapObject::name() const
{ return bmobject_ ? bmobject_->name().buf() : 0; }


void uiBaseMapObject::show( bool yn )
{ yn ? itemgrp_.show() : itemgrp_.hide(); }


void uiBaseMapObject::changedCB( CallBacker* )
{
    changed_ = true;
    update();
}


void uiBaseMapObject::setTransform( const uiWorld2Ui* w2ui )
{
    transform_ = w2ui;
}


void uiBaseMapObject::update()
{
    if ( !bmobject_ ) return;

    Threads::Locker( bmobject_->lock_ );

    int itemnr = 0;
    for ( int idx=0; idx<bmobject_->nrShapes(); idx++ )
    {
	TypeSet<Coord> crds;
	bmobject_->getPoints( idx, crds );

	if ( bmobject_->getLineStyle(idx) &&
	     bmobject_->getLineStyle(idx)->type_!=LineStyle::None )
	{
	    while ( itemgrp_.size()>itemnr )
	    {
		mDynamicCastGet(uiPolyLineItem*,itm,
				itemgrp_.getUiItem(itemnr));
		if ( !itm )
		    itemgrp_.remove( itemgrp_.getUiItem(itemnr), true );
		else break;
	    }

	    if ( itemgrp_.size()<=itemnr )
		itemgrp_.add( new uiPolyLineItem() );

	    mDynamicCastGet(uiPolyLineItem*,li,itemgrp_.getUiItem(itemnr))
	    if ( !li ) return;

	    TypeSet<uiPoint> uipts;
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
		uipts += transform_->transform( crds[ptidx] );

	    if ( bmobject_->close(idx) )
		uipts += transform_->transform( crds[0] );

	    li->setPenStyle( *bmobject_->getLineStyle(idx) );
	    li->setPolyLine( uipts );
	    itemnr++;
	}

	if ( bmobject_->fill(idx) )
	{
	    while ( itemgrp_.size()>itemnr )
	    {
		mDynamicCastGet(uiPolygonItem*,itm,itemgrp_.getUiItem(itemnr));
		if ( !itm )
		    itemgrp_.remove( itemgrp_.getUiItem(itemnr), true );
		else break;
	    }

	    if ( itemgrp_.size()<=itemnr )
		itemgrp_.add( new uiPolygonItem() );

	    mDynamicCastGet(uiPolygonItem*,itm,itemgrp_.getUiItem(itemnr))
	    if ( !itm ) return;

	    TypeSet<uiPoint> pts;
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
		pts += transform_->transform( uiWorldPoint(crds[ptidx]) );
	    itm->setPolygon( pts );
	    itm->fill();
	    itemnr++;
	}

	const MarkerStyle2D* ms2d = bmobject_->getMarkerStyle( idx );
	if ( ms2d && ms2d->type_!=MarkerStyle2D::None )
	{
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
	    {
		while ( itemgrp_.size()>itemnr )
		{
		    mDynamicCastGet(uiMarkerItem*,itm,
				    itemgrp_.getUiItem(itemnr));
		    if ( !itm )
			itemgrp_.remove( itemgrp_.getUiItem(itemnr), true );
		    else break;
		}

		if ( itemgrp_.size()<=itemnr )
		    itemgrp_.add( new uiMarkerItem() );

		mDynamicCastGet(uiMarkerItem*,itm,itemgrp_.getUiItem(itemnr));
		itm->setMarkerStyle( *ms2d );
		itm->setPenColor( ms2d->color_ );
		itm->setFillColor( ms2d->color_ );
		itm->setPos( transform_->transform(uiWorldPoint(crds[ptidx])) );
		itemnr++;
	    }
	}

	if ( bmobject_->getImage( idx ) )
	{
	    while ( itemgrp_.size()<itemnr )
	    {
		mDynamicCastGet(uiPixmapItem*,itm,itemgrp_.getUiItem(itemnr));
		if ( !itm )
		    itemgrp_.remove( itemgrp_.getUiItem(itemnr), true );
		else break;
	    }

	    if ( itemgrp_.size()<=itemnr )
		itemgrp_.add( new uiPixmapItem() );

	    mDynamicCastGet(uiPixmapItem*,itm,itemgrp_.getUiItem(itemnr));

	    mDynamicCastGet(const uiRGBArray*,rgbptr,bmobject_->getImage(idx));
	    uiPixmap pixmap( *rgbptr );
	    const int uixstart = transform_->toUiX( SI().minCoord(false).x );
	    const int uiystart = transform_->toUiY( SI().maxCoord(false).y );
	    itm->setOffset( uixstart, uiystart );
	    itm->setPixmap( pixmap );

	    itemnr++;
	}

	const char* shapenm = bmobject_->getShapeName( idx );
	if ( shapenm && !crds.isEmpty() )
	{
	    while ( itemgrp_.size()>itemnr )
	    {
		mDynamicCastGet(uiTextItem*,itm,
				itemgrp_.getUiItem(itemnr));
		if ( !itm )
		    itemgrp_.remove( itemgrp_.getUiItem(itemnr), true );
		else break;
	    }

	    if ( itemgrp_.size()<=itemnr )
		itemgrp_.add( new uiTextItem() );

	    mDynamicCastGet(uiTextItem*,itm,itemgrp_.getUiItem(itemnr));
	    itm->setText( shapenm );
	    itm->setPos( transform_->transform(uiWorldPoint(crds[0])) +
			 uiPoint(0,-3) );
	    Alignment al = bmobject_->getAlignment( idx );
	    itm->setAlignment( al );
	    itemnr++;
	}
    }

    while ( itemgrp_.size()>itemnr )
	itemgrp_.remove( itemgrp_.getUiItem(itemnr), true );
}


uiBaseMap::uiBaseMap( uiParent* p )
    : uiGroup(p,"Basemap")
    , view_(*new uiGraphicsView(this,"Basemap"))
    , w2ui_(*new uiWorld2Ui)
    , changed_(false)
{
    view_.reSize.notify( mCB(this,uiBaseMap,reSizeCB) );
}


uiBaseMap::~uiBaseMap()
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	view_.scene().removeItem( &objects_[idx]->itemGrp() );

    deepErase( objects_ );
    delete &view_;
    delete &w2ui_;
}


void uiBaseMap::addObject( BaseMapObject* obj )
{
    const int index = indexOf( obj );
    if ( index==-1 )
    {
	uiBaseMapObject* uiobj = new uiBaseMapObject( obj );
	if ( !uiobj )
	    return;

	addObject( uiobj ); // it already has the line 'changed = true'
    }
    else
    {
	objects_[index]->update();
    }
    // Does update() change the object? If yes, line 'changed = true' should go
    // inside it
}


bool uiBaseMap::hasChanged()
{
    if ( changed_ ) return true;

    for ( int idx=0; idx<objects_.size(); idx++ )
	if ( objects_[idx]->hasChanged() ) return true;

    return false;
}


void uiBaseMap::resetChangeFlag()
{
    changed_ = false;

    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->resetChangeFlag();
}


void uiBaseMap::addObject( uiBaseMapObject* uiobj )
{
    view_.scene().addItem( &uiobj->itemGrp() );
    objects_ += uiobj;
    uiobj->setTransform( &w2ui_ );
    changed_ = true;
}


void uiBaseMap::show( const BaseMapObject& obj, bool yn )
{
    const int objidx = indexOf( &obj );
    if ( !objects_.validIdx(objidx) ) return;

    objects_[objidx]->show( yn );
}


int uiBaseMap::indexOf( const BaseMapObject* obj ) const
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->bmobject_==obj )
	{
	    return idx;
	}
    }

    return -1;
}


void uiBaseMap::removeObject( const BaseMapObject* obj )
{
    const int index = indexOf( obj );
    if ( index==-1 )
    {
	pErrMsg( "Base map object not found" );
    }

    view_.scene().removeItem( &objects_[index]->itemGrp() );
    delete objects_.removeSingle( index );
    changed_ = true;
}


void uiBaseMap::reDraw( bool )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->update();
}


const char* uiBaseMap::nameOfItemAt( const Geom::Point2D<int>& pt )  const
{
    const uiGraphicsItem* itm = view_.scene().itemAt( pt );
    if ( !itm ) return 0;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	const uiGraphicsItemGroup& itmgrp = objects_[idx]->itemGrp();
	if ( !itmgrp.isPresent(*itm) )
	    continue;

	return objects_[idx]->name();
    }

    return 0;
}


