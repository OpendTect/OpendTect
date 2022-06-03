/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/


#include "uigraphicsitem.h"
#include "uigraphicsscene.h"

#include "uicursor.h"
#include "uimain.h"
#include "uistring.h"
#include "bufstringset.h"

#include "draw.h"

#include "odgraphicsitem.h"

#include "q_uiimpl.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsScene>
#include <QPen>
#include <QTransform>


mUseQtnamespace

class ODGraphicsItem : public QGraphicsItem
{
public:
ODGraphicsItem()
    : QGraphicsItem()
{}

QRectF boundingRect() const
{ return QRectF(); }

void paint( QPainter* painter, const QStyleOptionGraphicsItem* option,
	    QWidget* widget )
{
}

void hoverEnterEvent( QGraphicsSceneHoverEvent* event )
{
    QList<QGraphicsItem*> itms = childItems();
    for ( int idx=0; idx<itms.size(); idx++ )
    {
	mDynamicCastGet(ODGraphicsHighlightItem*,hlitm,itms[idx])
	if ( !hlitm ) continue;

	hlitm->highlight();
    }

    QGraphicsItem::hoverEnterEvent( event );
}

void hoverLeaveEvent( QGraphicsSceneHoverEvent* event )
{
    QList<QGraphicsItem*> itms = childItems();
    for ( int idx=0; idx<itms.size(); idx++ )
    {
	mDynamicCastGet(ODGraphicsHighlightItem*,hlitm,itms[idx])
	if ( !hlitm ) continue;

	hlitm->unHighlight();
    }

    QGraphicsItem::hoverLeaveEvent( event );
}

};


uiGraphicsItem::uiGraphicsItem()
    : qgraphicsitem_(new ODGraphicsItem)
    , leftClicked(this)
    , rightClicked(this)
    , scene_(0)
    , id_(getNewID())
    , selected_(false)
    , translation_( 0, 0 )
    , scale_( 1, 1 )
    , angle_( 0 )
    , parent_( 0 )
{
}


uiGraphicsItem::uiGraphicsItem( QGraphicsItem* itm )
    : qgraphicsitem_(itm)
    , leftClicked(this)
    , rightClicked(this)
    , scene_(0)
    , id_(getNewID())
    , selected_(false)
    , translation_( 0, 0 )
    , scale_( 1, 1 )
    , angle_( 0 )
    , parent_( 0 )
{
}


uiGraphicsItem::~uiGraphicsItem()
{
    removeAll( true );
    if ( parent_ )
	parent_->removeChild( this, false );
    if ( scene_ )
    {
	scene_->removeItem( this );
	delete qgraphicsitem_;
    }
}


void uiGraphicsItem::setScene( uiGraphicsScene* scene )
{ scene_ = scene; }


int uiGraphicsItem::getNewID()
{
    mDefineStaticLocalObject( Threads::Mutex, mutex, );
    Threads::MutexLocker lock( mutex );
    mDefineStaticLocalObject( int, curid, = 1 );
    return curid++;
}


void uiGraphicsItem::show()	{ qgraphicsitem_->show(); }
void uiGraphicsItem::hide()	{ qgraphicsitem_->hide(); }


void uiGraphicsItem::setAcceptHoverEvents( bool yn )
{
    qgraphicsitem_->setAcceptHoverEvents( yn );
}


void uiGraphicsItem::setAcceptedMouseButtons( OD::ButtonState bs )
{
    switch ( bs )
    {
	case OD::NoButton:
	    qgraphicsitem_->setAcceptedMouseButtons( Qt::NoButton );
	    break;
	case OD::LeftButton:
	    qgraphicsitem_->setAcceptedMouseButtons( Qt::LeftButton );
	    break;
	case OD::RightButton:
	    qgraphicsitem_->setAcceptedMouseButtons( Qt::RightButton );
	    break;
	case OD::MidButton:
	    qgraphicsitem_->setAcceptedMouseButtons( Qt::MiddleButton );
	    break;
	default:
	    qgraphicsitem_->setAcceptedMouseButtons( Qt::NoButton );
    }
}


void uiGraphicsItem::setFiltersChildEvents( bool yn )
{ qgraphicsitem_->setFiltersChildEvents( yn ); }


void uiGraphicsItem::setMovable( bool yn )
{ qgraphicsitem_->setFlag( QGraphicsItem::ItemIsMovable, yn ); }


void uiGraphicsItem::setVisible( bool yn )
{ qgraphicsitem_->setVisible( yn ); }


OD::ButtonState uiGraphicsItem::acceptedMouseButtonsEnabled() const
{
    Qt::MouseButtons qmb = qgraphicsitem_->acceptedMouseButtons();
    switch ( qmb )
    {
	case Qt::NoButton:
	    return OD::NoButton;
	case Qt::LeftButton:
	    return OD::LeftButton;
	case Qt::RightButton:
	    return OD::RightButton;
	case Qt::MiddleButton:
	    return OD::MidButton;
	default:
	    break;
    }

    return OD::NoButton;
}


bool uiGraphicsItem::isFiltersChildEventsEnabled() const
{ return qgraphicsitem_->filtersChildEvents(); }


bool uiGraphicsItem::isHoverEventsAccepted() const
{ return qgraphicsitem_->acceptHoverEvents(); }


bool uiGraphicsItem::isMovable() const
{ return qgraphicsitem_->flags().testFlag( QGraphicsItem::ItemIsMovable ); }


bool uiGraphicsItem::isVisible() const
{ return qgraphicsitem_->isVisible(); }


Geom::Point2D<float> uiGraphicsItem::getPos() const
{
    return Geom::Point2D<float>( mCast(float,qgraphicsitem_->pos().x()),
				 mCast(float,qgraphicsitem_->pos().y()) );
}


uiRect uiGraphicsItem::boundingRect() const
{
    QRectF qpr( qgraphicsitem_->sceneBoundingRect() );
    QRectF qcr( qgraphicsitem_->mapRectToScene(
		    qgraphicsitem_->childrenBoundingRect()) );
    QRectF qr = qpr | qcr;
    return uiRect( mNINT32(qr.left()), mNINT32(qr.top()),
		   mNINT32(qr.right()), mNINT32(qr.bottom()) );
}

void uiGraphicsItem::setPos( float x, float y )
{ stPos( x, y ); }


void uiGraphicsItem::setPos( const uiPoint& p )
{ stPos( p.x, p.y ); }


void uiGraphicsItem::setPos( const uiWorldPoint& p )
{ stPos( p.x, p.y ); }


void uiGraphicsItem::setPos( const Geom::Point2D<float>& p )
{ stPos(p.x, p.y); }



void uiGraphicsItem::stPos( float x, float y )
{
    translation_.x = x;
    translation_.y = y;

    updateTransform();
}


void uiGraphicsItem::moveBy( float x, float y )
{
    translation_.x += x;
    translation_.y += y;
    updateTransform();
}


float uiGraphicsItem::getRotation()
{
    return qgraphicsitem_->rotation();
}


void uiGraphicsItem::getScale( float &sx, float &sy )
{
    sx = scale_.x;
    sy = scale_.y;
}


void uiGraphicsItem::setRotation( float angle )
{
    angle_ = angle;
    updateTransform();
}


void uiGraphicsItem::setScale( float sx, float sy )
{
    scale_.x = sx;
    scale_.y = sy;

    updateTransform();
}


void uiGraphicsItem::updateTransform()
{
    if ( !isItemIgnoresTransformationsEnabled() )
    {
	QTransform transform;
	transform.translate( translation_.x, translation_.y );
	transform.scale( scale_.x, scale_.y );
	transform.rotate( angle_ );
	qgraphicsitem_->setTransform( transform );
	qgraphicsitem_->update();
    }
    else
	qgraphicsitem_->setPos( translation_.x, translation_.y );
}

/*
 * void uiGraphicsItem::scaleAroundXY( float sx, float sy, int x, int y )
{
    qgraphicsitem_->setTransform( QTransform().translate(x,y)
				  .scale(sx,sy).translate(-x,-y) );
}
*/

void uiGraphicsItem::setZValue( int zval )
{ qgraphicsitem_->setZValue( zval ); }


uiGraphicsItem* uiGraphicsItem::getChild( int idx )
{ return children_.validIdx(idx) ? children_[idx] : 0; }


bool uiGraphicsItem::isPresent( const uiGraphicsItem& itm ) const
{ return children_.isPresent( &itm ); }


int uiGraphicsItem::nrChildren() const
{ return children_.size(); }


void uiGraphicsItem::removeChild( uiGraphicsItem* itm, bool withdelete )
{
    if ( !itm ) return;

    itm->parent_ = 0;
    children_ -= itm;
    itm->qGraphicsItem()->setParentItem( 0 );

    if ( withdelete )
    {
	itm->setVisible( false );
	delete itm;
    }
}


void uiGraphicsItem::removeAll( bool withdelete )
{
    while ( !children_.isEmpty() )
	removeChild( children_[0], withdelete );
}


void uiGraphicsItem::addChild( uiGraphicsItem* itm )
{
    if ( !itm || children_.isPresent(itm) )
	return;

    itm->parent_ = this;
    children_ += itm;
    itm->qGraphicsItem()->setParentItem( qGraphicsItem() );
}


void uiGraphicsItem::addChildSet( uiGraphicsItemSet& set )
{
    for ( int idx=0; idx<set.size(); idx++ )
	addChild( set.get(idx) );
}


void uiGraphicsItem::removeChildSet( uiGraphicsItemSet& set )
{
    for ( int idx=0; idx<set.size(); idx++ )
	removeChild( set.get(idx), false );
}


uiPoint uiGraphicsItem::transformToScenePos( const uiPoint& pt ) const
{
    QPointF qpt = qgraphicsitem_->mapToScene( pt.x, pt.y );
    return uiPoint( mNINT32(qpt.x()),  mNINT32( qpt.y()) );
}


void uiGraphicsItem::setItemIgnoresTransformations( bool yn )
{
    qgraphicsitem_->setFlag(QGraphicsItem::ItemIgnoresTransformations, yn );
}


bool uiGraphicsItem::isItemIgnoresTransformationsEnabled() const
{
    return qgraphicsitem_->flags().testFlag(
				    QGraphicsItem::ItemIgnoresTransformations );
}


void uiGraphicsItem::setPenColor( const Color& col, bool usetransparency )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor(QRgb(col.rgb()));
    if ( usetransparency ) color.setAlpha( 255-col.t() );

    QPen qpen( color );
    qpen.setCosmetic( true );
    agsitm->setPen( qpen );
}


bool uiGraphicsItem::isSelectable() const
{
    return qgraphicsitem_->flags().testFlag( QGraphicsItem::ItemIsSelectable );
}


void uiGraphicsItem::setSelected( bool yn )
{
    selected_ = yn;
}


void uiGraphicsItem::setSelectable( bool yn )
{
    qgraphicsitem_->setFlag( QGraphicsItem::ItemIsSelectable, yn );
}


void uiGraphicsItem::setPenStyle( const OD::LineStyle& ls, bool usetransparency)
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor( QRgb(ls.color_.rgb()) );
    if ( usetransparency ) color.setAlpha( 255-ls.color_.t() );

    QBrush qbrush( color );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qpen.setCosmetic( true );
    agsitm->setPen( qpen );
    mDynamicCastGet(ODGraphicsHighlightItem*,hlitm,qgraphicsitem_)
    if ( hlitm ) hlitm->setQPen( qpen );
}


void uiGraphicsItem::setFillColor( const Color& col, bool usetransparency )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = Qt::transparent;
    if ( col != Color::NoColor() )
    {
	color = QColor( QRgb(col.rgb()) );
	if ( usetransparency ) color.setAlpha( 255-col.t() );
    }

    QBrush qbrush = agsitm->brush();
    if ( qbrush.style() == Qt::NoBrush )
	qbrush.setStyle( Qt::SolidPattern );
    qbrush.setColor( color );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setGradientFill( int xstart, int ystart,
				  int xstop, int ystop,
				  const TypeSet<float>& stops,
				  const TypeSet<Color>& colors )
{
    if ( colors.size() != stops.size() )
	return;

    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QLinearGradient qgrad( xstart, ystart, xstop, ystop );
    qgrad.setCoordinateMode( QGradient::ObjectBoundingMode );
    for ( int idx=0; idx<colors.size(); idx++ )
	qgrad.setColorAt( stops[idx], QColor( QRgb(colors[idx].rgb()) ) );

    QBrush qbrush( qgrad );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setFillPattern( const FillPattern& inpfp )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QBrush qbrush = agsitm->brush();
    Qt::BrushStyle qbs = Qt::NoBrush;
    FillPattern fp = inpfp;

    // Beware, this is a duplication of what is in draw.cc
    // Did this to get full freedom to solve extensions and changes in .cc only
    const int cDotsFillPatternType = 1;
    const int cLinesFillPatternType = 2;
    if ( fp.type_ == cDotsFillPatternType )
    {
	if ( fp.opt_ < 0 || fp.opt_ > 7 ) fp.opt_ = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::SolidPattern)+fp.opt_);
    }
    else if ( fp.type_ == cLinesFillPatternType )
    {
	if ( fp.opt_ < 0 || fp.opt_ > 8 ) fp.opt_ = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::HorPattern)+fp.opt_);
    }

    qbrush.setStyle( qbs );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setTransparency( float transparency )
{
    qgraphicsitem_->setOpacity( double(1.0f - transparency) );
}


float uiGraphicsItem::getTransparency() const
{
    return float(1.-qgraphicsitem_->opacity());
}


void uiGraphicsItem::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qgraphicsitem_->setCursor( qcursor );
}


void uiGraphicsItem::setToolTip( const uiString& tt )
{
    tooltip_ = tt;
    qgraphicsitem_->setToolTip( toQString(tt) );
}


void uiGraphicsItemSet::setZValue( int zval )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx] )
	    (*this)[idx]->setZValue( zval );
    }
}


void uiGraphicsItemSet::setVisible( bool yn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx] )
	    (*this)[idx]->setVisible( yn );
    }
}


int uiGraphicsItem::getZValue() const
{
    return mNINT32(qgraphicsitem_->zValue());
}


void uiGraphicsItem::translateText()
{
    qgraphicsitem_->setToolTip( toQString(tooltip_) );
}


uiGraphicsItem* uiGraphicsItem::findItem( QGraphicsItem* qitm )
{
    if ( qitm == qgraphicsitem_ )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	uiGraphicsItem* itm = children_[idx]->findItem( qitm );
	if ( itm ) return itm;
    }

    return 0;
}



// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup( bool owner )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(owner)
{}


void uiGraphicsItemGroup::setScene( uiGraphicsScene* scene )
{
    uiGraphicsItem::setScene( scene );
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setScene( scene );

}


uiGraphicsItemGroup::uiGraphicsItemGroup( const ObjectSet<uiGraphicsItem>& grp )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(false)
{
    ObjectSet<uiGraphicsItem>& itms =
				const_cast<ObjectSet<uiGraphicsItem>&>(grp);
    for ( int idx=0; idx<itms.size(); idx++ )
	add( itms[idx] );
}


uiGraphicsItemGroup::~uiGraphicsItemGroup()
{
    removeAll( owner_ );
    deepErase( items2bdel_ );
}


QGraphicsItem* uiGraphicsItemGroup::mkQtObj()
{
    qgraphicsitemgrp_ = new ODGraphicsItemGroup;
    return qgraphicsitemgrp_;
}


void uiGraphicsItemGroup::add( uiGraphicsItem* itm )
{
    if ( !itm ) return;

    if ( isMainThreadCurrent() )
    {
	items_ += itm;
	itm->setScene( scene_ );
	itm->setZValue( getZValue() );
	itm->qGraphicsItem()->setParentItem( qGraphicsItem() );
	qgraphicsitemgrp_->addToGroup( itm->qGraphicsItem() );
    }
    else
    {
	scene_->addUpdateToQueue(
		new uiGraphicsSceneChanger(*this,*itm,false) );
    }
}


void uiGraphicsItemGroup::remove( uiGraphicsItem* itm, bool withdelete )
{
    if ( !itm ) return;

    items_ -= itm;
    itm->setScene( 0 );
    itm->qGraphicsItem()->setParentItem( 0 );

    QGraphicsItem* qitm = itm->qGraphicsItem();
    if ( withdelete )
    {
	if ( qitm && qitm->scene() )
	    qitm->scene()->removeItem( qitm );
	delete itm;
	return;
    }

    qgraphicsitemgrp_->removeFromGroup( qitm );
}


void uiGraphicsItemGroup::removeAll( bool withdelete )
{
    while ( !items_.isEmpty() )
	remove( items_[0], withdelete );
}


void uiGraphicsItemGroup::setVisible( bool yn )
{
    isvisible_ = yn;
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setVisible( yn );
}


bool uiGraphicsItemGroup::isVisible() const
{
    return isvisible_;
}


uiRect uiGraphicsItemGroup::boundingRect() const
{
    if ( isEmpty() ) return uiRect();

    uiRect ret( getUiItem(0)->boundingRect() );
    for ( int idx=1; idx<items_.size(); idx++ )
	ret.include( items_[idx]->boundingRect() );
    return ret;
}


void uiGraphicsItemGroup::translateText()
{
    uiGraphicsItem::translateText();
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->translateText();
}


bool uiGraphicsItemGroup::isPresent( const uiGraphicsItem& itm ) const
{ return items_.isPresent( &itm ); }


uiGraphicsItem* uiGraphicsItemGroup::findItem( QGraphicsItem* qitm )
{
    if ( qitm == qgraphicsitemgrp_ )
	return this;

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	if ( qitm == items_[idx]->qGraphicsItem() )
	    return items_[idx];
    }

    return 0;
}
