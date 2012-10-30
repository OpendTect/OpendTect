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

    Threads::MutexLocker( bmobject_->lock_ );

    TypeSet<Coord> crds;
    int itemnr = 0;
    for ( int idx=0; idx<bmobject_->nrShapes(); idx++ )
    {
	bmobject_->getPoints( idx, crds );

	if ( bmobject_->getLineStyle(idx) &&
	     bmobject_->getLineStyle(idx)->type_!=LineStyle::None )
	{
	    while ( itemgrp_->size()>itemnr )
	    {
		mDynamicCastGet(uiPolyLineItem*,itm,itemgrp_->getUiItem(itemnr));
		if ( !itm ) itemgrp_->remove( itemgrp_->getUiItem(itemnr), true );
		else break;
	    }

	    if ( itemgrp_->size()<=itemnr )
		itemgrp_->add( new uiPolyLineItem() );

	    mDynamicCastGet(uiPolyLineItem*,li,itemgrp_->getUiItem(itemnr))
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
	    while ( itemgrp_->size()>itemnr )
	    {
		mDynamicCastGet(uiPolygonItem*,itm,itemgrp_->getUiItem(itemnr));
		if ( !itm ) itemgrp_->remove( itemgrp_->getUiItem(itemnr), true );
		else break;
	    }

	    if ( itemgrp_->size()<=itemnr )
		itemgrp_->add( new uiPolygonItem() );

	    mDynamicCastGet(uiPolygonItem*,itm,itemgrp_->getUiItem(itemnr))
	    if ( !itm ) return;

	    TypeSet<uiPoint> pts;
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
		pts += transform_->transform( uiWorldPoint(crds[ptidx]) );
	    itm->setPolygon( pts );
	    itm->fill();
	    itemnr++;
	}

	if ( bmobject_->getMarkerStyle(idx) &&
	     bmobject_->getMarkerStyle(idx)->type_!=MarkerStyle2D::None )
	{
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
	    {
		while ( itemgrp_->size()>itemnr )
		{
		    mDynamicCastGet(uiMarkerItem*,itm,itemgrp_->getUiItem(itemnr));
		    if ( !itm )
			itemgrp_->remove( itemgrp_->getUiItem(itemnr), true );
		    else break;
		}

		if ( itemgrp_->size()<=itemnr )
		    itemgrp_->add( new uiMarkerItem() );

		mDynamicCastGet(uiMarkerItem*,itm,itemgrp_->getUiItem(itemnr));
		itm->setMarkerStyle( *bmobject_->getMarkerStyle(idx) );
		itm->setPos( transform_->transform( uiWorldPoint(crds[ptidx]) ) );
		itemnr++;
	    }
	}
    }

    while ( itemgrp_->size()>itemnr )
    {
	itemgrp_->remove( itemgrp_->getUiItem(itemnr), true );
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
    const int index = indexOf( obj );
    if ( index==-1 )
    {
	uiBaseMapObject* uiobj = new uiBaseMapObject( obj );
	if ( !uiobj )
	    return;

	addObject( uiobj );
    }
    else
    {
	objects_[index]->update();
    }
}


void uiBaseMap::addObject( uiBaseMapObject* uiobj )
{
    view_.scene().addItem( uiobj->itemGrp() );
    objects_ += uiobj;
    uiobj->setTransform( &w2ui_ );
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

    view_.scene().removeItem( objects_[index]->itemGrp() );
    delete objects_.removeSingle( index );
}


void uiBaseMap::reDraw( bool )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->update();
}
