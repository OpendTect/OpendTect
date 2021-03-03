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
    , changed_(false)
    , transform_(0)
{
    if ( bmobject_ )
    {
	mAttachCB( bmobject_->changed, uiBaseMapObject::changedCB );
	mAttachCB( bmobject_->styleChanged, uiBaseMapObject::changedStyleCB );
	mAttachCB( bmobject_->zvalueChanged, uiBaseMapObject::changedZValueCB );
	changedZValueCB( 0 );
    }
    graphitem_.setAcceptHoverEvents( true );
}


uiBaseMapObject::~uiBaseMapObject()
{
    detachAllNotifiers();
    delete &graphitem_;
    delete &labelitem_;
}

const char* uiBaseMapObject::type() const
{ return bmobject_ ? bmobject_->type() : 0; }

const char* uiBaseMapObject::name() const
{ return bmobject_ ? bmobject_->name().buf() : 0; }

void uiBaseMapObject::show( bool yn )
{
    graphitem_.setVisible( yn );
    if ( showlabels_ )
	labelitem_.setVisible( yn );
}

bool uiBaseMapObject::isShown() const
{ return graphitem_.isVisible(); }


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
    graphitem_.setZValue( bmobject_->depth() );
    labelitem_.setZValue( bmobject_->depth()-1 );
}


void uiBaseMapObject::setTransform( const uiWorld2Ui* w2ui )
{ transform_ = w2ui; }


void uiBaseMapObject::add( uiGraphicsItem& itm )
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

	const BaseMapObject::LineStyle* linestyle = bmobject_->lineStyle( idx );
	if ( linestyle && linestyle->type_!=OD::LineStyle::None )
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
		    add( *new uiPolyLineItem() );
		}

		mDynamicCastGet(uiPolyLineItem*,itm,graphitem_.getChild(itemnr))
		if ( !itm ) return;

		itm->setPenStyle( *linestyle );
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
		    add( *new uiPolygonItem() );
		}

		mDynamicCastGet(uiPolygonItem*,itm,graphitem_.getChild(itemnr))
		if ( !itm ) return;

		itm->setPenStyle( *linestyle );
		itm->setPolygon( worldpts );
		itm->setFillColor( bmobject_->fillColor(idx), true );
		itm->fill();
		itm->setAcceptHoverEvents( bmobject_->allowHoverEvent(idx) );
		itemnr++;
	    }
	}

	const BufferString imgfnm = bmobject_->imageFileName( idx );
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

	const BaseMapObject::MarkerStyle* markerstyle
			    = bmobject_->markerStyle( idx );
	if ( markerstyle && markerstyle->type_!=OD::MarkerStyle2D::None )
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
		    add( *itm );
		}

		mDynamicCastGet(uiMarkerItem*,itm,graphitem_.getChild(itemnr));
		itm->setMarkerStyle( *markerstyle );
		itm->setPenColor( markerstyle->color_ );
		itm->setFillColor( markerstyle->color_ );
		itm->setPos( crds[ptidx] );
		itm->setAcceptHoverEvents( bmobject_->allowHoverEvent(idx) );
		itemnr++;
	    }
	}

	const char* shapenm = bmobject_->shapeName( idx );
	if ( shapenm && !crds.isEmpty() )
	{
	    if ( labelitem_.nrChildren()<=labelitemnr )
	    {
		addLabel( *new uiTextItem() );
	    }

	    mDynamicCastGet(uiTextItem*,itm,labelitem_.getChild(labelitemnr));
	    if ( !itm ) return;

	    itm->setText( toUiString(shapenm) );
	    for( int crdidx=0; crdidx<crds.size(); crdidx++ )
	    {
		if( !mIsUdf(crds[crdidx]) )
		{
		    itm->setPos( crds[crdidx] );
		    break;
		}
	    }

	    itm->setAlignment( bmobject_->alignment(idx) );
	    const float angle = Math::toDegrees( bmobject_->textRotation() );
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
	const BaseMapObject::LineStyle* linestyle = bmobject_->lineStyle( idx );
	if ( linestyle && linestyle->type_!=OD::LineStyle::None )
	{
	    if ( !bmobject_->close(idx) )
	    {
		mDynamicCastGet(uiPolyLineItem*,li,graphitem_.getChild(itemnr))
		if ( !li ) return;

		li->setPenStyle( *linestyle );
		itemnr++;
	    }
	    else
	    {
		mDynamicCastGet(uiPolygonItem*,itm,graphitem_.getChild(itemnr))
		if ( !itm ) return;

		itm->setPenStyle( *linestyle );
		itm->setFillColor( bmobject_->fillColor(idx), true );
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
    }
}



// uiBaseMap
uiBaseMap::uiBaseMap( uiParent* p )
    : w2ui_(*new uiWorld2Ui)
    , worlditem_(*new uiGraphicsItem())
    , changed_(false)
    , objectAdded(this)
    , objectToBeRemoved(this)
    , centerworlditem_(false)
{
    bmgroup_ = new uiGroup( p, "Basemap" );
    view_ = new uiGraphicsView( bmgroup_,"Basemap" );
    view_->scene().addItem( &worlditem_ );
    mAttachCB( view_->reSize, uiBaseMap::reSizeCB );
}


uiBaseMap::~uiBaseMap()
{
    detachAllNotifiers();
    deepErase( objects_ );
    view_->scene().removeItem( &worlditem_ );
    delete view_;
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
    const uiRect viewrect( 0, 0, view_->scene().nrPixX(),
				 view_->scene().nrPixY() );

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


BaseMapObject* uiBaseMap::bmObject( int id )
{
    if ( id<0 ) return nullptr;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	BaseMapObject* bmo = objects_[idx]->bmObject();
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
	BaseMapObject* bmo = objects_[idx]->bmObject();
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
    if ( uiobj->bmObject() )
	objectAdded.trigger( uiobj->bmObject()->ID() );
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

    objectToBeRemoved.trigger( obj->ID() );
    delete objects_.removeSingle( index );
    changed_ = true;
}


void uiBaseMap::reDraw( bool )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->update();
}


const uiBaseMapObject*
	uiBaseMap::uiObjectAt( const Geom::Point2D<float>& pt ) const
{
    const uiGraphicsItem* itm = view_->scene().itemAt( pt );
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
    nm.setEmpty(); info.setEmpty(); tk.setUdf(); val = mUdf(float);

    const MouseEvent& ev = view_->getMouseEventHandler().event();
    crd3.setXY( getWorld2Ui().toWorldX(ev.x()),
		getWorld2Ui().toWorldY(ev.y()) );
    crd3.z_ = mUdf(double);

    Geom::Point2D<float> pt; pt.setFrom( ev.pos() );
    const uiBaseMapObject* uibmobj = uiObjectAt( pt );
    if ( uibmobj )
    {
	nm = uibmobj->name();
	uibmobj->getMousePosInfo( crd3, tk, val, info );
    }
}


uiGraphicsScene& uiBaseMap::scene()
{ return view_->scene(); }


void uiBaseMap::centerWorldItem( bool yn )
{ centerworlditem_ = yn; }
