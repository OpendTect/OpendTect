/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/


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
    , labelitem_(*new uiGraphicsItem)
    , showlabels_(true)
    , changed_(false)
    , transform_(nullptr)
{
    if ( bmobject_ )
    {
	mAttachCB( bmobject_->changed, uiBaseMapObject::changedCB );
	mAttachCB( bmobject_->stylechanged, uiBaseMapObject::changedStyleCB );
	mAttachCB( bmobject_->zvalueChanged, uiBaseMapObject::changedZValueCB );

	graphitem_.setZValue( bmobject_->getDepth() );
	labelitem_.setZValue( bmobject_->getDepth()-1 );
    }

    graphitem_.setAcceptHoverEvents( true );
}


uiBaseMapObject::~uiBaseMapObject()
{
    detachAllNotifiers();
    delete &graphitem_;
    delete &labelitem_;
}


BaseMapObject* uiBaseMapObject::getObject()
{ return bmobject_; }


const char* uiBaseMapObject::name() const
{
    return bmobject_ ? bmobject_->name().buf() : nullptr;
}


void uiBaseMapObject::show( bool yn )
{
    graphitem_.setVisible( yn );
    if ( showlabels_ )
	labelitem_.setVisible( yn );
}


bool uiBaseMapObject::isShown() const
{
    return graphitem_.isVisible();
}


void uiBaseMapObject::showLabels( bool yn )
{
    showlabels_ = yn;
    if ( isShown() )
	labelitem_.setVisible( yn );
}


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


void uiBaseMapObject::changedZValueCB( CallBacker* )
{
    changed_ = true;
    graphitem_.setZValue( bmobject_->getDepth() );
    labelitem_.setZValue( bmobject_->getDepth()-1 );
}


void uiBaseMapObject::setTransform( const uiWorld2Ui* w2ui )
{
    transform_ = w2ui;
}


void uiBaseMapObject::addToGraphItem( uiGraphicsItem& itm )
{
    graphitem_.addChild( &itm );
    mAttachCB( itm.leftClicked, uiBaseMapObject::leftClickCB );
    mAttachCB( itm.rightClicked, uiBaseMapObject::rightClickCB );
}


void uiBaseMapObject::addLabel( uiGraphicsItem& itm )
{
    labelitem_.addChild( &itm );
}


void uiBaseMapObject::getMousePosInfo( Coord3& crd, TrcKey& tk, float& val,
				       BufferString& info ) const
{
    if ( bmobject_ )
	bmobject_->getMousePosInfo( crd, tk, val, info );
}


void uiBaseMapObject::update()
{
    if ( !bmobject_ ) return;

    Threads::Locker locker( bmobject_->lock_ );

    int itemnr = 0;
    int labelitemnr = 0;
    for ( int idx=0; idx<bmobject_->nrShapes(); idx++ )
    {
	TypeSet<Coord> crds;
	bmobject_->getPoints( idx, crds );

	TypeSet<uiWorldPoint> worldpts( crds.size(), Coord::udf() );
	for ( int cidx=0; cidx<crds.size(); cidx++ )
	    worldpts[cidx] = crds[cidx];

	if ( bmobject_->getLineStyle(idx) &&
	     bmobject_->getLineStyle(idx)->type_!=OD::LineStyle::None )
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
		    addToGraphItem( *new uiPolyLineItem() );
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
		    addToGraphItem( *new uiPolygonItem() );
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
		    auto* itm =	new uiPixmapItem( uiPixmap(imgfnm) );
		    itm->setPaintInCenter( true );
		    graphitem_.addChild( itm );
		}

		mDynamicCastGet(uiPixmapItem*,itm,graphitem_.getChild(itemnr));
		if ( !itm ) return;
		itm->setPixmap( uiPixmap(imgfnm) );
		itm->setPos( crds[ptidx] );

		float scalex=1.f, scaley=1.f;
		bmobject_->getXYScale( idx, scalex, scaley );
		itm->setScale( scalex, scaley );
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
		    addToGraphItem( *new uiMarkerItem() );
		}

		mDynamicCastGet(uiMarkerItem*,itm,graphitem_.getChild(itemnr));
		itm->setMarkerStyle( *ms2d );
		itm->setPenColor( OD::Color::Black() );
		itm->setFillColor( ms2d->color_ );
		itm->setPos( crds[ptidx] );
		itm->setAcceptHoverEvents( bmobject_->allowHoverEvent(idx) );
		itemnr++;
	    }
	}

	const char* shapenm = bmobject_->getShapeName( idx );
	if ( shapenm && !crds.isEmpty() )
	{
	    if ( labelitem_.nrChildren()<=labelitemnr )
	    {
		addLabel( *new uiTextItem() );
	    }

	    mDynamicCastGet(uiTextItem*,itm,labelitem_.getChild(labelitemnr));
	    if ( !itm ) return;

	    itm->setText( toUiString(shapenm) );
	    Coord txtpos = bmobject_->getTextPos( idx );
	    if ( txtpos.isUdf() )
	    {
		for( int crdidx=0; crdidx<crds.size(); crdidx++ )
		{
		    if( !mIsUdf(crds[crdidx]) )
		    {
			txtpos = crds[crdidx];
			break;
		    }
		}
	    }

	    itm->setPos( txtpos );
	    const Alignment al = bmobject_->getAlignment( idx );
	    itm->setAlignment( al );

	    const float angle = Math::toDegrees( bmobject_->getTextRotation() );
	    itm->setRotation( angle );

	    labelitemnr++;
	}
    }

    while ( graphitem_.nrChildren()>itemnr )
	graphitem_.removeChild( graphitem_.getChild(itemnr), true );

    while ( labelitem_.nrChildren()>labelitemnr )
	labelitem_.removeChild( labelitem_.getChild(labelitemnr), true );
}


void uiBaseMapObject::updateStyle()
{
    if ( !bmobject_ ) return;

    Threads::Locker locker( bmobject_->lock_ );

    int itemnr = 0;
    for ( int idx=0; idx<bmobject_->nrShapes(); idx++ )
    {
	if ( bmobject_->getLineStyle(idx) &&
	     bmobject_->getLineStyle(idx)->type_!=OD::LineStyle::None )
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
    mAttachCB( view_.reSize, uiBaseMap::reSizeCB );
}


uiBaseMap::~uiBaseMap()
{
    detachAllNotifiers();
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

    double wrwidth = wr_.width();
    double wrheight = wr_.bottom() - wr_.top();
    if ( mIsZero(wrwidth,mDefEps) || mIsZero(wrheight,mDefEps) )
	return;

    bool fitwidth = true;
    double xscale = viewrect.width() / wrwidth;
    double yscale = -xscale;
    if ( yscale*wrheight > viewrect.height() )
    {
	fitwidth = false;
	yscale = viewrect.height() / wrheight;
	xscale = -yscale;
    }

    uiWorldRect newwr( wr_ );
    uiRect newviewrect( viewrect );
    if ( centerworlditem_ ) // Adjust wr_ to cover viewrect
    {
	if ( fitwidth )
	{
	    const double newwrheight = viewrect.height() / yscale;
	    const double halfdiff = (newwrheight - wrheight) / 2;
	    newwr.setTop( newwr.top() - halfdiff );
	    newwr.setBottom( newwr.bottom() + halfdiff );
	    wrheight = newwrheight;
	}
	else
	{
	    const double newwrwidth = viewrect.width() / xscale;
	    const double halfdiff = (newwrwidth - wrwidth) / 2;
	    newwr.setLeft( newwr.left() - halfdiff );
	    newwr.setRight( newwr.right() + halfdiff );
	    wrwidth = newwrwidth;
	}
    }
    else // Adjust viewrect to cover wr_
    {
	newviewrect.setRight( mNINT32(xscale*wrwidth) );
	newviewrect.setBottom( mNINT32(yscale*wrheight) );
    }

    w2ui_.set( newviewrect, newwr );

    const double xpos = newviewrect.left() - xscale*newwr.left();
    const double ypos = newviewrect.top() - yscale*newwr.top();

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
    if ( id<0 ) return nullptr;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	BaseMapObject* bmo = objects_[idx]->getObject();
	if ( bmo && bmo->ID()==id )
	    return bmo;
    }

    return nullptr;
}


uiBaseMapObject* uiBaseMap::getUiObject( int id )
{
    if ( id<0 ) return nullptr;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	BaseMapObject* bmo = objects_[idx]->getObject();
	if ( bmo && bmo->ID()==id )
	    return objects_[idx];
    }

    return nullptr;
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
    worlditem_.addChild( &uiobj->labelItem() );
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


void uiBaseMap::showLabels( bool yn )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->showLabels( yn );
}


bool uiBaseMap::labelsShown() const
{ return !objects_.isEmpty() && objects_[0]->labelsShown(); }


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


const uiBaseMapObject*
	uiBaseMap::uiObjectAt( const Geom::Point2D<float>& pt ) const
{
    const uiGraphicsItem* itm = view_.scene().itemAt( pt );
    if ( !itm ) return nullptr;

    mDynamicCastGet(const uiTextItem*,txtitm,itm)
    if ( txtitm ) return nullptr;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	const uiGraphicsItem& bmitm = objects_[idx]->graphItem();
	if ( !bmitm.isPresent(*itm) )
	    continue;

	return objects_[idx];
    }

    return nullptr;
}


const char* uiBaseMap::nameOfItemAt( const Geom::Point2D<float>& pt )  const
{
    const uiBaseMapObject* uibmobj = uiObjectAt( pt );
    return uibmobj ? uibmobj->name() : nullptr;
}


void uiBaseMap::getMousePosInfo( BufferString& nm, Coord3& crd3, TrcKey& tk,
				 float& val, BufferString& info ) const
{
    nm.setEmpty();
    tk = TrcKey::udf();
    val = mUdf(float);
    info.setEmpty();

    const MouseEvent& ev = view_.getMouseEventHandler().event();
    const Geom::Point2D<double> pt = ev.getDPos();
    crd3.setFrom( getWorld2Ui().transform<double,double>(pt,true) );
    crd3.z = mUdf(double);

    const uiBaseMapObject* uibmobj = uiObjectAt( ev.getFPos() );
    if ( uibmobj )
    {
	nm = uibmobj->name();
	uibmobj->getMousePosInfo( crd3, tk, val, info );
    }
}


uiGraphicsScene& uiBaseMap::scene()
{ return view_.scene(); }


void uiBaseMap::centerWorldItem( bool yn )
{ centerworlditem_ = yn; }
