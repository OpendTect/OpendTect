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
    , graphitem_(*new uiGraphicsItem)
    , changed_(false)
    , transform_(0)
{
    if ( bmobject_ )
    {
	bmobject_->changed.notify( mCB(this,uiBaseMapObject,changedCB) );
	bmobject_->stylechanged.notify(
		    mCB(this,uiBaseMapObject,changedStyleCB) );
	graphitem_.setZValue( bmobject_->getDepth() );
    }
    graphitem_.setAcceptHoverEvents( true );
}


uiBaseMapObject::~uiBaseMapObject()
{
    if ( bmobject_ )
    {
	bmobject_->changed.remove( mCB(this,uiBaseMapObject,changedCB) );
	bmobject_->stylechanged.remove(
		    mCB(this,uiBaseMapObject,changedStyleCB) );
    }

    delete &graphitem_;
}


BaseMapObject* uiBaseMapObject::getObject()
{ return bmobject_; }


const char* uiBaseMapObject::name() const
{ return bmobject_ ? bmobject_->name().buf() : 0; }

void uiBaseMapObject::show( bool yn )
{ graphitem_.setVisible( yn ); }

bool uiBaseMapObject::isShown() const
{ return graphitem_.isVisible(); }


void uiBaseMapObject::leftClickCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const MouseEvent&,ev,cb);
    if ( bmobject_ )
	bmobject_->leftClicked.trigger( ev );
}

void uiBaseMapObject::rightClickCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const MouseEvent&,ev,cb);
    if ( bmobject_ )
	bmobject_->rightClicked.trigger( ev );
}


void uiBaseMapObject::changedCB( CallBacker* )
{
    changed_ = true;
    update();
}


void uiBaseMapObject::changedStyleCB( CallBacker* )
{
    changed_ = true;
    updateStyle();
}


void uiBaseMapObject::setTransform( const uiWorld2Ui* w2ui )
{ transform_ = w2ui; }


void uiBaseMapObject::addToGraphItem( uiGraphicsItem& itm )
{
    graphitem_.addChild( &itm );
    itm.leftClicked.notify( mCB(this,uiBaseMapObject,leftClickCB) );
    itm.rightClicked.notify( mCB(this,uiBaseMapObject,rightClickCB) );
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

	TypeSet<uiWorldPoint> worldpts;
	for ( int cdx=0; cdx<crds.size(); cdx++ )
	    worldpts += crds[cdx];

	if ( bmobject_->getLineStyle(idx) &&
	     bmobject_->getLineStyle(idx)->type_!=LineStyle::None )
	{
	    if ( !bmobject_->close(idx) )
	    {
		while ( graphitem_.nrChildren()>itemnr )
		{
		    mDynamicCastGet(uiPolyLineItem*,itm,
				    graphitem_.getChild(itemnr));
		    if ( !itm )
			graphitem_.removeChild( graphitem_.getChild(itemnr),
						true );
		    else break;
		}

		if ( graphitem_.nrChildren()<=itemnr )
		{
		    uiPolyLineItem* itm = new uiPolyLineItem();
		    if ( !itm ) return;
		    addToGraphItem( *itm );
		}

		mDynamicCastGet(uiPolyLineItem*,itm,graphitem_.getChild(itemnr))
		if ( !itm ) return;

		itm->setPenStyle( *bmobject_->getLineStyle(idx) );
		itm->setPolyLine( worldpts );
		itm->setAcceptHoverEvents( bmobject_->allowHoverEvent(idx) );
		itemnr++;
	    }
	    else
	    {
		while ( graphitem_.nrChildren()>itemnr )
		{
		    mDynamicCastGet(uiPolygonItem*,itm,
				    graphitem_.getChild(itemnr));
		    if ( !itm )
			graphitem_.removeChild( graphitem_.getChild(itemnr),
						true );
		    else break;
		}

		if ( graphitem_.nrChildren()<=itemnr )
		{
		    uiPolygonItem* itm = new uiPolygonItem();
		    if ( !itm ) return;
		    addToGraphItem( *itm );
		}

		mDynamicCastGet(uiPolygonItem*,itm,graphitem_.getChild(itemnr))
		if ( !itm ) return;

		itm->setPenStyle( *bmobject_->getLineStyle(idx) );
		itm->setPolygon( worldpts );
		itm->setFillColor( bmobject_->getFillColor(idx), true );
		itm->fill();
		itm->setAcceptHoverEvents( bmobject_->allowHoverEvent(idx) );
		itemnr++;
	    }
	}

	const BufferString imgfnm = bmobject_->getImageFileName( idx );
	if ( !imgfnm.isEmpty() )
	{
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
	    {
		while ( graphitem_.nrChildren()>itemnr )
		{
		    mDynamicCastGet(uiPixmapItem*,itm,
				    graphitem_.getChild(itemnr));
		    if ( !itm )
			graphitem_.removeChild( graphitem_.getChild(itemnr),
						true );
		    else break;
		}

		if ( graphitem_.nrChildren()<=itemnr )
		{
		    uiPixmapItem* itm =	new uiPixmapItem( uiPixmap(imgfnm) );
		    itm->setPaintInCenter( true );
		    graphitem_.addChild( itm );
		}

		mDynamicCastGet(uiPixmapItem*,itm,graphitem_.getChild(itemnr));
		if ( !itm ) return;
		itm->setPixmap( uiPixmap(imgfnm) );
		itm->setPos( crds[ptidx] );
		const int scale = bmobject_->getScale(idx);
		itm->setScale( mCast(float,scale), mCast(float,scale) );
		itemnr++;
	    }
	}

	const MarkerStyle2D* ms2d = bmobject_->getMarkerStyle( idx );
	if ( ms2d && ms2d->type_!=MarkerStyle2D::None )
	{
	    for ( int ptidx=0; ptidx<crds.size(); ptidx++ )
	    {
		while ( graphitem_.nrChildren()>itemnr )
		{
		    mDynamicCastGet(uiMarkerItem*,itm,
				    graphitem_.getChild(itemnr));
		    if ( !itm )
			graphitem_.removeChild( graphitem_.getChild(itemnr),
						true );
		    else break;
		}

		if ( graphitem_.nrChildren()<=itemnr )
		{
		    uiMarkerItem* itm = new uiMarkerItem();
		    if ( !itm ) return;
		    addToGraphItem( *itm );
		}

		mDynamicCastGet(uiMarkerItem*,itm,graphitem_.getChild(itemnr));
		itm->setMarkerStyle( *ms2d );
		itm->setPenColor( ms2d->color_ );
		itm->setFillColor( ms2d->color_ );
		itm->setPos( crds[ptidx] );
		itm->setAcceptHoverEvents( bmobject_->allowHoverEvent(idx) );
		itemnr++;
	    }
	}

	const char* shapenm = bmobject_->getShapeName( idx );
	if ( shapenm && !crds.isEmpty() )
	{
	    while ( graphitem_.nrChildren()>itemnr )
	    {
		mDynamicCastGet(uiTextItem*,itm,
				graphitem_.getChild(itemnr));
		if ( !itm )
		    graphitem_.removeChild( graphitem_.getChild(itemnr), true );
		else break;
	    }

	    if ( graphitem_.nrChildren()<=itemnr )
	    {
		uiTextItem* itm = new uiTextItem();
		if ( !itm ) return;
		addToGraphItem( *itm );
	    }

	    mDynamicCastGet(uiTextItem*,itm,graphitem_.getChild(itemnr));
	    if ( !itm ) return;
	    itm->setText( mToUiStringTodo(shapenm) );
	    itm->setPos( crds[0] );
	    const Alignment al = bmobject_->getAlignment( idx );
	    itm->setAlignment( al );

	    const float angle = Math::toDegrees( bmobject_->getTextRotation() );
	    itm->setRotation( angle );

	    itemnr++;
	}
    }

    while ( graphitem_.nrChildren()>itemnr )
	graphitem_.removeChild( graphitem_.getChild(itemnr), true );
}


void uiBaseMapObject::updateStyle()
{
    if ( !bmobject_ ) return;

    Threads::Locker( bmobject_->lock_ );

    int itemnr = 0;
    for ( int idx=0; idx<bmobject_->nrShapes(); idx++ )
    {
	if ( bmobject_->getLineStyle(idx) &&
	     bmobject_->getLineStyle(idx)->type_!=LineStyle::None )
	{
	    if ( !bmobject_->close(idx) )
	    {
		mDynamicCastGet(uiPolyLineItem*,li,graphitem_.getChild(itemnr))
		if ( !li ) return;

		li->setPenStyle( *bmobject_->getLineStyle(idx) );
		itemnr++;
	    }
	    else
	    {
		mDynamicCastGet(uiPolygonItem*,itm,graphitem_.getChild(itemnr))
		if ( !itm ) return;

		itm->setPenStyle( *bmobject_->getLineStyle(idx) );
		itm->setFillColor( bmobject_->getFillColor(idx), true );
		itm->fill();
		itemnr++;
	    }
	}

	if ( bmobject_->fill(idx) )
	{
	    mDynamicCastGet(uiPolygonItem*,itm,graphitem_.getChild(itemnr))
	    if ( !itm ) return;

	    itm->fill();
	    itemnr++;
	}

	const char* shapenm = bmobject_->getShapeName( idx );
	if ( shapenm )
	    itemnr++;
    }
}



// uiBaseMap
uiBaseMap::uiBaseMap( uiParent* p )
    : uiGroup(p,"Basemap")
    , view_(*new uiGraphicsView(this,"Basemap"))
    , w2ui_(*new uiWorld2Ui)
    , worlditem_(*new uiGraphicsItem())
    , changed_(false)
    , objectAdded(this)
    , objectRemoved(this)
    , centerworlditem_(false)
{
    view_.scene().addItem( &worlditem_ );
    view_.reSize.notify( mCB(this,uiBaseMap,reSizeCB) );
}


uiBaseMap::~uiBaseMap()
{
    deepErase( objects_ );
    view_.scene().removeItem( &worlditem_ );
    delete &view_;
    delete &w2ui_;
}


void uiBaseMap::reSizeCB( CallBacker* )
{
    updateTransform();
}


void uiBaseMap::setView( const uiWorldRect& wr )
{
    wr_ = wr;
    updateTransform();
}


void uiBaseMap::updateTransform()
{
    const uiRect viewrect( 0, 0, (int)view_.scene().width(),
				 (int)view_.scene().height() );

    const double wrwidth = wr_.width();
    const double wrheight = wr_.bottom() - wr_.top();
    if ( mIsZero(wrwidth,mDefEps) || mIsZero(wrheight,mDefEps) )
	return;

    double xscale = viewrect.width() / wrwidth;
    double yscale = -xscale;
    if ( yscale*wrheight > viewrect.height() )
    {
	yscale = viewrect.height() / wrheight;
	xscale = -yscale;
    }


    const int pixwidth = mNINT32( xscale * wrwidth );
    const int pixheight = mNINT32( yscale * wrheight );

    const double xshift =
	centerworlditem_ ? (viewrect.width()-pixwidth) / 2. : 0;
    const double yshift =
	centerworlditem_ ? (viewrect.height()-pixheight) / 2. : 0;

    const uiRect newviewrect( 0, 0, pixwidth, pixheight );
    w2ui_.set( newviewrect, wr_ );

    const double xpos = viewrect.left() - xscale*wr_.left() + xshift;
    const double ypos = viewrect.top() - yscale*wr_.top() + yshift;

    worlditem_.setPos( uiWorldPoint(xpos,ypos) );
    worlditem_.setScale( (float)xscale, (float)yscale );
    reDraw();
}


void uiBaseMap::addObject( BaseMapObject* obj )
{
    const int index = indexOf( obj );
    if ( index==-1 )
    {
	uiBaseMapObject* uiobj = new uiBaseMapObject( obj );
	addObject( uiobj );
    }
    else
	objects_[index]->update();
}


BaseMapObject* uiBaseMap::getObject( int id )
{
    if ( id<0 ) return 0;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	BaseMapObject* bmo = objects_[idx]->getObject();
	if ( bmo && bmo->ID()==id )
	    return bmo;
    }

    return 0;
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
    if ( !uiobj ) return;

    worlditem_.addChild( &uiobj->graphItem() );
    objects_ += uiobj;
    changed_ = true;
    if ( uiobj->getObject() )
	objectAdded.trigger( uiobj->getObject()->ID() );
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
    const int index = obj ? indexOf( obj ) : -1;
    if ( index==-1 )
	return;

    delete objects_.removeSingle( index );
    changed_ = true;
    objectRemoved.trigger( obj->ID() );
}


void uiBaseMap::reDraw( bool )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->update();
}


const char* uiBaseMap::nameOfItemAt( const Geom::Point2D<float>& pt )  const
{
    const uiGraphicsItem* itm = view_.scene().itemAt( pt );
    if ( !itm ) return 0;

    mDynamicCastGet(const uiTextItem*,txtitm,itm)
    if ( txtitm ) return 0;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	const uiGraphicsItem& bmitm = objects_[idx]->graphItem();
	if ( !bmitm.isPresent(*itm) )
	    continue;

	return objects_[idx]->name();
    }

    return 0;
}


uiGraphicsScene& uiBaseMap::scene()
{ return view_.scene(); }


void uiBaseMap::centerWorldItem( bool yn )
{ centerworlditem_ = yn; }

