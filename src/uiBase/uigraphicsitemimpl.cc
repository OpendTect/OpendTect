/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
________________________________________________________________________

-*/

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"

#include "angles.h"
#include "odgraphicsitem.h"
#include "mousecursor.h"
#include "polygon.h"

#include "uifont.h"
#include "uigeom.h"
#include "uigroup.h"
#include "uiobj.h"
#include "uipixmap.h"
#include "uirgbarray.h"

#include "q_uiimpl.h"

#include <QMutex>
#include <QBrush>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsProxyWidget>
#include <QPen>
#include <QTextDocument>
#include <QWidget>

mUseQtnamespace

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    #define mGetTextWidth(qfm,textstring) qfm.horizontalAdvance( textstring )
#else
    #define mGetTextWidth(qfm,textstring) qfm.width( textstring )
#endif

uiObjectItem::uiObjectItem( uiObject* obj )
    : uiGraphicsItem(mkQtObj())
    , obj_(0)
    , grp_(0)
{
    setObject( obj );
}


uiObjectItem::uiObjectItem( uiGroup* grp )
    : uiGraphicsItem(mkQtObj())
    , obj_(0)
    , grp_(0)
{
    setGroup( grp );
}


uiObjectItem::~uiObjectItem()
{
}


uiObject* uiObjectItem::getObject()
{ return obj_; }


void uiObjectItem::setObject( uiObject* obj )
{
    obj_ = obj;
    qwidgetitem_->setWidget( obj_ ? obj_->getWidget(0) : 0 );
}


uiGroup* uiObjectItem::getGroup()
{ return grp_; }

void uiObjectItem::setGroup( uiGroup* grp )
{
    grp_ = grp;
    if ( !grp_ ) return;

    setObject( grp->attachObj() );
}


QGraphicsItem* uiObjectItem::mkQtObj()
{
    qwidgetitem_ = new QGraphicsProxyWidget();
    return qwidgetitem_;
}


void uiObjectItem::setObjectSize( int szx, int szy )
{
    if ( grp_ )
	grp_->setGeometry( uiSize(szx,szy) );
    else if ( obj_ && obj_->getWidget(0) )
    {
	obj_->getWidget(0)->setMinimumSize( szx, szy );
	obj_->getWidget(0)->setMaximumSize( szx, szy );
    }
}


const uiSize uiObjectItem::objectSize() const
{
    return ( obj_ && obj_->getWidget(0)
	    ? uiSize(obj_->getWidget(0)->size().width(),
			   obj_->getWidget(0)->size().height() )
	    : uiSize(0,0) );
}


// uiEllipseItem
uiEllipseItem::uiEllipseItem()
    : uiGraphicsItem(mkQtObj())
{}


uiEllipseItem::uiEllipseItem( const uiPoint& center, const uiSize& sz )
    : uiGraphicsItem(mkQtObj())
{
    setPos( center );
    setSize( sz );
}


uiEllipseItem::uiEllipseItem( const uiSize& size )
    : uiGraphicsItem(mkQtObj())
{
    setSize( size );
}


uiEllipseItem::~uiEllipseItem()
{
}


QGraphicsItem* uiEllipseItem::mkQtObj()
{
    qellipseitem_ = new QGraphicsEllipseItem();
    return qellipseitem_;
}


void uiEllipseItem::setSize( const uiSize& size )
{
    const int width = size.hNrPics();
    const int height = size.vNrPics();
    qellipseitem_->setRect( -width/2, -height/2, width, height );
}


uiCircleItem::uiCircleItem()
    : uiEllipseItem()
{}


uiCircleItem::uiCircleItem( const uiPoint& center, int r )
    : uiEllipseItem(center,uiSize(2*r,2*r))
{}


uiCircleItem::uiCircleItem( int r )
    : uiEllipseItem(uiSize(2*r,2*r))
{}


void uiCircleItem::setRadius( int r )
{
    setSize( uiSize(2*r,2*r) );
}


// uiLineItem

#define mInit \
      uiGraphicsItem(mkQtObj()) \
    , qpen_(* new QPen)

uiLineItem::uiLineItem()
    : mInit
{}


uiLineItem::uiLineItem( const uiPoint& startpos, const uiPoint& endpos )
    : mInit
{
    setLine( startpos, endpos );
}


uiLineItem::uiLineItem( float x1, float y1, float x2, float y2 )
    : mInit
{
    setLine( x1, y1, x2, y2 );
}


uiLineItem::uiLineItem( int x1, int y1, int x2, int y2 )
   : mInit
{
    setLine( x1, y1, x2, y2 );
}


uiLineItem::uiLineItem( const uiPoint& pt, float angle, float len )
    : mInit
{
    Geom::Point2D<float> ptf( pt.x_, pt.y_ );
    Geom::Point2D<float> endpt( ptf );
    float delta = len * cos( angle );
    endpt.x_ += delta;
    delta = -len * sin( angle );
    endpt.y_ += delta;

    setLine( ptf, endpt );
}


uiLineItem::~uiLineItem()
{
    delete &qpen_;
}


QGraphicsItem* uiLineItem::mkQtObj()
{
    qlineitem_ = new ODGraphicsLineItem();
    return qlineitem_;
}


void uiLineItem::setLine( const uiPoint& start, const uiPoint& end )
{ setLine( start.x_, start.y_, end.x_, end.y_ ); }

void uiLineItem::setLine( const Geom::Point2D<float>& start,
			  const Geom::Point2D<float>& end )
{ setLine( start.x_, start.y_, end.x_, end.y_ ); }

void uiLineItem::setLine( const uiWorldPoint& start, const uiWorldPoint& end )
{ setLine( (float)start.x_, (float)start.y_, (float)end.x_, (float)end.y_ ); }

void uiLineItem::setLine( int x1, int y1, int x2, int y2 )
{ setLine( (float)x1, (float)y1, (float)x2, (float)y2 ); }


void uiLineItem::setLine( float x1, float y1, float x2, float y2 )
{
    qlineitem_->setLine( x1, y1, x2, y2 );
}


void uiLineItem::setLine( const Geom::Point2D<float>& centerpos,
	      float dx1, float dy1, float dx2, float dy2 )
{
    setLine( centerpos.x_ - dx1, centerpos.y_ + dy1,
	     centerpos.x_ + dx2, centerpos.y_ - dy2 );
}


void uiLineItem::setLine( const Geom::Point2D<int>& centerpos,
	      int dx1, int dy1, int dx2, int dy2 )
{
    setLine( centerpos.x_-dx1, centerpos.y_+dy1,
	     centerpos.x_+dx2, centerpos.y_-dy2 );
}


uiRect uiLineItem::lineRect() const
{
    const QRectF rect = qlineitem_->boundingRect();
    return uiRect( rect.top(), rect.left(), rect.bottom(), rect.right() );
}


void uiLineItem::setPenColor( const Color& col, bool )
{
    QPen qpen = qlineitem_->getQpen();
    qpen.setColor( QColor(col.rgb()) );
    qpen_ = qpen;
    qlineitem_->setQPen( qpen );
}


void uiLineItem::setPenStyle( const OD::LineStyle& ls, bool )
{
    QBrush qbrush( QColor(QRgb(ls.color_.rgb())) );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qpen_ = qpen;
    qlineitem_->setQPen( qpen );
}


void uiLineItem::highlight()
{
    qlineitem_->highlight();
}


void uiLineItem::unHighlight()
{
    qlineitem_->unHighlight();
}


//uiDynamicImageItem
uiDynamicImageItem::uiDynamicImageItem()
    : uiGraphicsItem( mkQtObj() )
{}


uiDynamicImageItem::~uiDynamicImageItem()
{}


void uiDynamicImageItem::setImage( bool isdynamic, const uiRGBArray& image,
				   const uiWorldRect& wr )
{
    item_->setImage( isdynamic, image.qImage(),
	      QRectF( wr.left(), wr.top(), wr.width(), wr.height() ) );
}


NotifierAccess& uiDynamicImageItem::wantsData()
{ return item_->wantsData; }


bool uiDynamicImageItem::isSnapshot() const
{ return item_->isSnapshot(); }


uiWorldRect uiDynamicImageItem::wantedWorldRect() const
{
    QRectF res = item_->wantedWorldRect();
    return uiWorldRect( res.left(), res.top(), res.right(), res.bottom() );
}


uiSize uiDynamicImageItem::wantedScreenSize() const
{
    QSize sz = item_->wantedScreenSize();
    return uiSize( sz.width(), sz.height() );
}


QGraphicsItem* uiDynamicImageItem::mkQtObj()
{
    item_ = new ODGraphicsDynamicImageItem();
    return item_;
}

// uiPixmapItem
uiPixmapItem::uiPixmapItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPixmapItem::uiPixmapItem( const uiPixmap& pm )
    : uiGraphicsItem(mkQtObj())
{
    setPixmap( pm );
}


uiPixmapItem::uiPixmapItem( const uiPoint& pos, const uiPixmap& pm )
    : uiGraphicsItem(mkQtObj())
{
    setPos( pos );
    setPixmap( pm );
}


uiPixmapItem::~uiPixmapItem()
{
}


QGraphicsItem* uiPixmapItem::mkQtObj()
{
    qpixmapitem_ = new ODGraphicsPixmapItem();
    return qpixmapitem_;
}


void uiPixmapItem::setOffset( int left, int top )
{
    qpixmapitem_->setOffset( QPointF(left,top) );
}


void uiPixmapItem::setPixmap( const uiPixmap& pixmap )
{
    qpixmapitem_->setPixmap( *pixmap.qpixmap());
}


void uiPixmapItem::setPaintInCenter( bool yn )
{
    qpixmapitem_->setPaintInCenter( yn );
}



uiSize uiPixmapItem::pixmapSize() const
{
    QPixmap pm( qpixmapitem_->pixmap() );
    return uiSize( pm.width(), pm.height() );
}


void uiPixmapItem::scaleToScene()
{
    if ( !scene_ )
	{ pErrMsg("Cannot scale to scene without scene"); return; }
    setPaintInCenter( false );

    QPixmap pm( qpixmapitem_->pixmap() );
    const int curwdth = pm.width(); const int curhght = pm.height();
    const int newwdth = scene_->nrPixX(); const int newhght = scene_->nrPixY();

    if ( curwdth == newwdth && curhght == newhght )
	return;

    QPixmap newpm = pm.scaled( newwdth, newhght,
		       Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    qpixmapitem_->setPixmap( newpm );
}


// uiPolygonItem
uiPolygonItem::uiPolygonItem()
    : uiGraphicsItem(mkQtObj())
{}


#define mImplPolygonConstructor( type ) \
uiPolygonItem::uiPolygonItem( type polygon, bool dofill ) \
    : uiGraphicsItem(mkQtObj()) \
{ \
    setPolygon( polygon ); \
    if ( dofill ) \
	fill(); \
}


mImplPolygonConstructor( const ODPolygon<int>& )
mImplPolygonConstructor( const TypeSet<uiPoint>& );
mImplPolygonConstructor( const TypeSet<uiWorldPoint>& );

uiPolygonItem::~uiPolygonItem()
{
}


QGraphicsItem* uiPolygonItem::mkQtObj()
{
    qpolygonitem_ = new ODGraphicsPolyLineItem();
    return qpolygonitem_;
}


void uiPolygonItem::fill()
{
    qpolygonitem_->setFillRule( Qt::OddEvenFill );
}


#define mImplSetPolygon( type, ptaccess ) \
void uiPolygonItem::setPolygon( type ptlist ) \
{ \
    QPolygonF qpolygonf;\
    for ( unsigned int idx=0; idx<ptlist.size(); idx++ )\
    { \
	if ( ptaccess[idx].isDefined() ) \
	    qpolygonf += QPointF( (float) ptaccess[idx].x_, \
				  (float) ptaccess[idx].y_ ); \
    } \
    if ( !qpolygonf.isEmpty() && !qpolygonf.isClosed() ) \
	qpolygonf += qpolygonf.first(); \
    qpolygonitem_->setPolyLine( qpolygonf, true ); \
}


mImplSetPolygon( const TypeSet<uiPoint>&, ptlist )
mImplSetPolygon( const ODPolygon<int>&, ptlist.data() )
mImplSetPolygon( const TypeSet<uiWorldPoint>&, ptlist )


// uiPolyLineItem
uiPolyLineItem::uiPolyLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPolyLineItem::uiPolyLineItem( const TypeSet<uiPoint>& ptlist )
    : uiGraphicsItem(mkQtObj())
{
    setPolyLine( ptlist );
}


uiPolyLineItem::uiPolyLineItem( const TypeSet<uiWorldPoint>& ptlist )
    : uiGraphicsItem(mkQtObj())
{
    setPolyLine( ptlist );
}


uiPolyLineItem::~uiPolyLineItem()
{}


#define mImplSetPolyline( type ) \
void uiPolyLineItem::setPolyLine( type ptlist ) \
{ \
    QPainterPath path; \
    bool newpt = true; \
    for ( int idx=0; idx<ptlist.size(); idx++ ) \
    { \
	if ( !ptlist[idx].isDefined() ) \
	{  \
	    newpt = true; \
	    continue; \
	} \
	if ( newpt ) \
	{ \
	    path.moveTo( ptlist[idx].x_, ptlist[idx].y_ ); \
	    newpt = false; \
	} \
	else \
	    path.lineTo( ptlist[idx].x_, ptlist[idx].y_ ); \
    } \
 \
    odgraphicspath_->set( path ); \
}

mImplSetPolyline( const TypeSet<uiPoint>& );
mImplSetPolyline( const TypeSet<uiWorldPoint>& );
#undef mImplSetPolyline


QGraphicsItem* uiPolyLineItem::mkQtObj()
{
    odgraphicspath_ = new ODGraphicsPathItem();
    return odgraphicspath_;
}


void uiPolyLineItem::highlight()
{
    odgraphicspath_->highlight();
}


void uiPolyLineItem::unHighlight()
{
    odgraphicspath_->unHighlight();
}


//uiMultiColorPolyLineItem
uiMultiColorPolyLineItem::uiMultiColorPolyLineItem()
    : uiGraphicsItem(mkQtObj())
    , penwidth_(2)
{}


uiMultiColorPolyLineItem::uiMultiColorPolyLineItem(
					const TypeSet<uiPoint>& pts )
    : uiGraphicsItem(mkQtObj())
    , penwidth_(2)
{
    setPolyLine( pts );
}


uiMultiColorPolyLineItem::uiMultiColorPolyLineItem(
					const TypeSet<uiWorldPoint>& pts )
    : uiGraphicsItem(mkQtObj())
    , penwidth_(2)
{
    setPolyLine( pts );
}


uiMultiColorPolyLineItem::~uiMultiColorPolyLineItem()
{}


QGraphicsItem* uiMultiColorPolyLineItem::mkQtObj()
{
    odmulticoloritem_ = new ODGraphicsMultiColorPolyLineItem();
    return odmulticoloritem_;
}


#define mImplSetPolyline( type ) \
void uiMultiColorPolyLineItem::setPolyLine( type ptlist ) \
{ \
    QPolygonF qpolygonf; \
    for ( int idx=0; idx<ptlist.size(); idx++ ) \
    { \
	if ( !ptlist[idx].isDefined() ) \
	    qpolygonf += QPointF( mUdf(double), mUdf(double) ); \
	else \
	    qpolygonf += QPointF( mCast(double,ptlist[idx].x_), \
				  mCast(double,ptlist[idx].y_) ); \
    } \
    odmulticoloritem_->setPolyLine( qpolygonf ); \
}

mImplSetPolyline( const TypeSet<uiPoint>& );
mImplSetPolyline( const TypeSet<uiWorldPoint>& );
#undef mImplSetPolyline


void uiMultiColorPolyLineItem::setColors(
		const TypeSet<Color>& colors, bool usetransparency )
{
    QVector<QPen> qpens( colors.size() );
    for ( int idx=0; idx<colors.size(); idx++ )
    {
	qpens[idx] = QPen( QColor(QRgb(colors[idx].rgb())), penwidth_ );
	if ( usetransparency )
	    qpens[idx].color().setAlpha( 255-colors[idx].t() );
    }
    odmulticoloritem_->setQPens( qpens );
}


void uiMultiColorPolyLineItem::setPenWidth( int sz )
{
    penwidth_ = sz;
    odmulticoloritem_->setPenWidth( sz );
}


int uiMultiColorPolyLineItem::getPenWidth() const
{
    return penwidth_;
}


// uiRectItem
uiRectItem::uiRectItem()
    : uiGraphicsItem(mkQtObj())
{}


uiRectItem::uiRectItem( QGraphicsRectItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qrectitem_(qtobj)
{}


uiRectItem::uiRectItem( int x, int y, int width, int height )
    : uiGraphicsItem(mkQtObj())
{
    setRect( x, y, width, height );
}


uiRectItem::~uiRectItem()
{
}


QGraphicsItem* uiRectItem::mkQtObj()
{
    qrectitem_ = new QGraphicsRectItem();
    return qrectitem_;
}


void uiRectItem::setRect( int x, int y, int width, int height )
{
    qrectitem_->setRect( 0, 0, width, height );
    qrectitem_->setPos( x, y );
}


// uiTextItem
uiTextItem::uiTextItem()
    : uiGraphicsItem( mkODObj() )
{
    setAlignment( OD::Alignment(OD::Alignment::Left,OD::Alignment::Top) );
}


uiTextItem::uiTextItem( const uiString& txt, const OD::Alignment& al )
    : uiGraphicsItem(mkODObj())
{
    setText( txt );
    setAlignment( al );
}


uiTextItem::uiTextItem( const uiPoint& pos, const uiString& txt,
			const OD::Alignment& al )
    : uiGraphicsItem(mkODObj())
{
    setText( txt );
    setPos( pos );
    setAlignment( al );
}


uiTextItem::~uiTextItem()
{
}


ODGraphicsTextItem* uiTextItem::mkODObj()
{
    qtextitem_ = new ODGraphicsTextItem();
    return qtextitem_;
}


#define mExtraSpace 10

uiString uiTextItem::getText() const
{
    return text_;
}


uiSize uiTextItem::getTextSize() const
{
    QFontMetrics qfm( qtextitem_->getFont() );
    // Extra space is added to avoid clipping on some platforms and the value is
    // arbitrarily chosen.
    return uiSize( mGetTextWidth(qfm,text_.getOriginalString())+mExtraSpace,
		   qfm.height()+mExtraSpace );
}


void uiTextItem::setText( const uiString& txt )
{
    text_ = txt;
    qtextitem_->setText( toQString(text_) );
}


void uiTextItem::translateText()
{
    uiGraphicsItem::translateText();
    qtextitem_->setText( toQString(text_) );
}


void uiTextItem::setFont( const uiFont& font )
{
    qtextitem_->setFont( font.qFont() );
}


void uiTextItem::setFontData( const FontData& fd )
{
    QFont font = qtextitem_->getFont();
    uiFont::setFontData( font, fd );
    qtextitem_->setFont( font );
}


void uiTextItem::setAlignment( const OD::Alignment& al )
{

    switch ( al.hPos() )
    {
	case OD::Alignment::Right:
	    qtextitem_->setHAlignment( Qt::AlignRight );
	    break;
	case OD::Alignment::HCenter:
	    qtextitem_->setHAlignment( Qt::AlignHCenter );
	    break;
	case OD::Alignment::Left:
	    qtextitem_->setHAlignment( Qt::AlignLeft );
	    break;
    }

    switch ( al.vPos() )
    {
	case OD::Alignment::Bottom:
	    qtextitem_->setVAlignment( Qt::AlignBottom );
	    break;
	case OD::Alignment::VCenter:
	    qtextitem_->setVAlignment( Qt::AlignVCenter );
	    break;
	case OD::Alignment::Top:
	    qtextitem_->setVAlignment( Qt::AlignTop );
	    break;
    }
}


void uiTextItem::stPos( float x, float y )
{
    qtextitem_->setPos( x, y );
}


void uiTextItem::setTextColor( const Color& col )
{
    qtextitem_->setPen( QPen(QColor(col.r(),col.g(), col.b())) );
}


void uiTextItem::fitIn( const uiRect& rect, bool verttxt )
{
    if ( text_.isEmpty() )
	return;

    QFont qfont = qtextitem_->getFont();
    const int txtwidth = verttxt ? rect.height() : rect.width();
    const int txtheight = verttxt ? rect.width() : rect.height();
    int resizedir = 0;
    const QString qtxt( toQString(text_) );
    float curptsz = qfont.pointSizeF();
    float prevptsz = curptsz;
    while ( true )
    {
	QFontMetrics qfm( qfont );
	const int wdth = mGetTextWidth(qfm,qtxt);
	const int hght = qfm.height();
	const bool istoobig = wdth > txtwidth || hght > txtheight;
	if ( resizedir == 0 )
	    resizedir = istoobig ? -1 : 1;
	else if ( (resizedir < 0 && !istoobig) || (resizedir > 0 && istoobig) )
	    break;

	prevptsz = curptsz;
	curptsz += (resizedir > 0 ? 0.2f : -0.2f);
	qfont.setPointSizeF( curptsz );
    }

    qfont.setPointSizeF( prevptsz );
    QFontMetrics qfm( qfont );
    const int wdth = mGetTextWidth(qfm,qtxt);
    int txtdirshift = (txtwidth - wdth) / 2;
    if ( txtdirshift < 0 )
	txtdirshift = 0;
    const int hght = qfm.height();
    int updirshift = (txtheight - hght) / 2;
    if ( updirshift < 0 )
	updirshift = 0;

    qtextitem_->setFont( qfont );
    int xshft = txtdirshift - 1;
    int yshft = updirshift - 1;
    if ( verttxt )
    {
	std::swap( xshft, yshft );
	xshft += 4; // Hack, probably fontmetrics related
	yshft -= 15; // Hack, seems to be consistent for vertical display
    }
    qtextitem_->setPos( rect.left()+xshft, rect.top()+yshft );
}


// uiAdvancedTextItem
uiAdvancedTextItem::uiAdvancedTextItem( bool centered )
    : uiGraphicsItem(0)
    , al_(OD::Alignment::Left,OD::Alignment::Top)
    , textiscentered_(centered)
{
    qgraphicsitem_ = mkQtObj();
}


uiAdvancedTextItem::uiAdvancedTextItem( const uiString& txt,
					const OD::Alignment& al,
					bool centered )
    : uiGraphicsItem(0)
    , al_(al)
    , textiscentered_(centered)
{
    qgraphicsitem_ = mkQtObj();
    setPlainText( txt );
}


uiAdvancedTextItem::~uiAdvancedTextItem()
{
}


OD::Alignment uiAdvancedTextItem::getAlignment() const
{
    return al_;
}


Color uiAdvancedTextItem::getDefaultTextColor() const
{
    QColor qcol = qtextitem_->defaultTextColor();
    return Color( qcol.red(), qcol.green(), qcol.blue(), 255-qcol.alpha() );
}


uiFont& uiAdvancedTextItem::getFont() const
{
    uiFontList fontlist;
    QFont qfont = qtextitem_->font();
    return fontlist.getFromQfnt( &qfont );
}


uiString uiAdvancedTextItem::getPlainText() const
{
    QString str = qtextitem_->toPlainText();
    uiString ret;
    ret.setFrom( str );
    return ret;
}


float uiAdvancedTextItem::getTextWidth() const
{ return qtextitem_->textWidth(); }


void uiAdvancedTextItem::setAlignment( const OD::Alignment& al )
{
    al_ = al;
}


void uiAdvancedTextItem::setDefaultTextColor( const Color& col )
{
    QColor qcol( col.r(), col.g(), col.b(), 255-col.t() );
    qtextitem_->setDefaultTextColor( qcol );
}


void uiAdvancedTextItem::setFont( const FontData& fd )
{
    QFont qfont;
    uiFont::setFontData( qfont, fd );
    qtextitem_->setFont( qfont );
}


void uiAdvancedTextItem::setPlainText( const uiString& txt )
{
    qtextitem_->setPlainText( toQString(txt) );
}


void uiAdvancedTextItem::setTextWidth( float width )
{ qtextitem_->setTextWidth( width ); }



void uiAdvancedTextItem::setTextIteraction( bool yn )
{
    if ( yn )
	qtextitem_->setTextInteractionFlags( Qt::TextEditorInteraction );
    else
	qtextitem_->setTextInteractionFlags( Qt::NoTextInteraction );
}


QGraphicsItem* uiAdvancedTextItem::mkQtObj()
{
    qtextitem_ = new ODGraphicsAdvancedTextItem( textiscentered_ );
    return qtextitem_;
}


void uiAdvancedTextItem::stPos( float x, float y )
{
    if ( !isItemIgnoresTransformationsEnabled() )
    {
	QRectF boundrec = qtextitem_->boundingRect();
	switch( al_.hPos() )
	{
	case OD::Alignment::Left:
	    boundrec.translate( 0., 0. );
	    break;
	case OD::Alignment::HCenter:
	    boundrec.translate( -boundrec.width()/2., 0. );
	    break;
	case OD::Alignment::Right:
	    boundrec.translate( -boundrec.width(), 0. );
	    break;
	}

	switch( al_.vPos() )
	{
	case OD::Alignment::Top:
	    boundrec.translate( 0., 0. );
	    break;
	case OD::Alignment::VCenter:
	    boundrec.translate( 0., -boundrec.height()/2. );
	    break;
	case OD::Alignment::Bottom:
	    boundrec.translate( 0., -boundrec.height() );
	    break;
	}

	const QPointF p00 = qtextitem_->mapToParent( QPointF(0,0) );
	const QPointF d01 = qtextitem_->mapToParent( QPointF(0,1) )-p00;
	const QPointF d10 = qtextitem_->mapToParent( QPointF(1,0) )-p00;

	const float xdist = Math::Sqrt(d10.x()*d10.x()+d10.y()*d10.y() );
	const float ydist = Math::Sqrt(d01.x()*d01.x()+d01.y()*d01.y() );

	const float xlin = x+mCast(float,boundrec.left())*xdist;
	const float ylin = y+mCast(float,boundrec.top())*ydist;
	uiGraphicsItem::stPos( xlin, ylin );
    }
    else
	uiGraphicsItem::stPos( x, y );
}


// uiMarkerItem
uiMarkerItem::uiMarkerItem( bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setFill( fill );
}


uiMarkerItem::uiMarkerItem( const uiPoint& pos, const OD::MarkerStyle2D& mstyle,
			    bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setPos( pos );
    setMarkerStyle( mstyle );
    setFill( fill );
}


uiMarkerItem::uiMarkerItem( const OD::MarkerStyle2D& mstyle, bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setMarkerStyle( mstyle );
    if ( fill )
	setFillColor( mstyle.color_ );
}


QGraphicsItem* uiMarkerItem::mkQtObj()
{
    qmarkeritem_ = new ODGraphicsMarkerItem();
    return qmarkeritem_;
}


uiMarkerItem::~uiMarkerItem()
{
}


void uiMarkerItem::setMarkerStyle( const OD::MarkerStyle2D& mstyle )
{
    qmarkeritem_->setMarkerStyle( mstyle );
}


const OD::MarkerStyle2D* uiMarkerItem::getMarkerStyle()
{
    return qmarkeritem_->getMarkerStyle();
}


void uiMarkerItem::setFill( bool fill )
{
    qmarkeritem_->setFill( fill );
}



void uiMarkerItem::setFillColor( const Color& col, bool )
{
    qmarkeritem_->setFill( true );
    qmarkeritem_->setFillColor( col );
}


// uiPointItem
uiPointItem::uiPointItem( const uiPoint& pos )
    : uiGraphicsItem(mkQtObj())
{
    setPos( pos );
}


uiPointItem::uiPointItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPointItem::~uiPointItem()
{
}


QGraphicsItem* uiPointItem::mkQtObj()
{
    qpointitem_ = new ODGraphicsPointItem();
    return qpointitem_;
}


// uiArrowItem
uiArrowItem::uiArrowItem()
    : uiGraphicsItem(mkQtObj())
{}


uiArrowItem::uiArrowItem( const uiPoint& tail, const uiPoint& head,
			  const OD::ArrowStyle& style )
    : uiGraphicsItem(mkQtObj())
{
    tailpos_ = tail;
    headpos_ = head;
    setArrowStyle( style );
    update();
}


uiArrowItem::~uiArrowItem()
{
}


QGraphicsItem* uiArrowItem::mkQtObj()
{
    qarrowitem_ = new ODGraphicsArrowItem();
    return qarrowitem_;
}


void uiArrowItem::setHeadPos( const uiPoint& pt )
{
    headpos_ = pt;
    update();
}


void uiArrowItem::setTailPos( const uiPoint& pt )
{
    tailpos_ = pt;
    update();
}


void uiArrowItem::setTailHeadPos( const uiPoint& tail, const uiPoint& head )
{
    tailpos_ = tail;
    headpos_ = head;
    update();
}


void uiArrowItem::setArrowStyle( const OD::ArrowStyle& arrowstyle )
{
    qarrowitem_->setArrowStyle( arrowstyle );
}


void uiArrowItem::setArrowSize( int arrowsz )
{
    qarrowitem_->setArrowSize( arrowsz );
}


void uiArrowItem::update()
{
    qarrowitem_->resetTransform();

    float diffx = headpos_.x_-tailpos_.x_;
    float diffy = headpos_.y_-tailpos_.y_;
    const float arrsz = Math::Sqrt( diffx*diffx + diffy*diffy );
    setArrowSize( mNINT32(arrsz) );
    setPos( headpos_ );
    const uiPoint relvec( mNINT32(diffx), mNINT32(diffy) );
    const float ang = Math::Atan2( (float)relvec.y_, (float)relvec.x_);
    setRotation( Math::toDegrees(ang) );
}


// uiCurvedItem
uiCurvedItem::uiCurvedItem( const uiPoint& pt )
    : uiGraphicsItem(mkQtObj())
{
    qppath_->moveTo( QPointF(pt.x_,pt.y_) );
}


uiCurvedItem::uiCurvedItem( const Geom::Point2D<float>& pt )
    : uiGraphicsItem(mkQtObj())
{
    qppath_->moveTo( QPointF(pt.x_,pt.y_) );
    qpathitem_->setPath( *qppath_ );
}


uiCurvedItem::~uiCurvedItem()
{
    delete qppath_;
}


void uiCurvedItem::drawTo( const uiPoint& pt )
{
    drawTo( Geom::Point2D<float>(pt.x_,pt.y_) );
}


void uiCurvedItem::drawTo( const Geom::Point2D<float>& pt )
{
    qppath_->lineTo( QPointF(pt.x_,pt.y_) );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::drawTo( const ArcSpec& as )
{
    Interval<float> angs( Math::toDegrees(as.angles_.start),
			  Math::toDegrees(as.angles_.stop) );
    QRectF qr( as.center_.x_ - as.radius_, as.center_.y_ - as.radius_,
	       2*as.radius_, 2*as.radius_ );
    qppath_->arcTo( qr, angs.start, angs.stop - angs.start );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::drawTo( const SplineSpec& ss )
{
    if ( ss.cubic_ )
	qppath_->cubicTo( QPointF(ss.cp1_.x_,ss.cp1_.y_),
			  QPointF(ss.cp2_.x_,ss.cp2_.y_),
			  QPointF(ss.end_.x_,ss.end_.y_) );
    else
	qppath_->quadTo( QPointF(ss.cp1_.x_,ss.cp1_.y_),
			 QPointF(ss.end_.x_,ss.end_.y_) );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::closeCurve()
{
    qppath_->closeSubpath();
    qpathitem_->setPath( *qppath_ );
}


QGraphicsItem* uiCurvedItem::mkQtObj()
{
    qppath_ = new QPainterPath();
    qpathitem_ = new QGraphicsPathItem();
    return qpathitem_;
}


#define mAddItm( typ, itm, zvaladd ) \
    itm = new typ; itm->setZValue( setup_.zval_ + zvaladd ); add( itm )

uiManipHandleItem::uiManipHandleItem( const Setup& su, bool ishor )
    : uiGraphicsItemGroup(true)
    , setup_(su)
    , ishor_(ishor)
    , pixpos_(0)
{
    centeritm_ = addLine( 0, 1 );
    bodyitm_ = addLine( -1, setup_.thickness_ );
    shadeitm1_ = addLine( -2, setup_.thickness_ );
    shadeitm2_ = addLine( -3, setup_.thickness_ );
    mAddItm( uiRectItem, rectitm_, -1 );
    setPenColor( su.color_ );
}


uiLineItem* uiManipHandleItem::addLine( int zvaladd, int lwdth )
{
    uiLineItem* mAddItm( uiLineItem, li, zvaladd );
    li->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,lwdth) );
    return li;
}


void uiManipHandleItem::setIsHorizontal( bool yn )
{
    if ( ishor_ != yn )
	{ ishor_ = yn; updatePos(); }
}


void uiManipHandleItem::setPixPos( int pp )
{
    pixpos_ = pp;
    updatePos();
}


void uiManipHandleItem::setPixPos( double pp )
{
    setPixPos( mNINT32(pp) );
}


void uiManipHandleItem::setPenColor( const Color& basecol, bool )
{
    setup_.color_ = basecol;
    centeritm_->setPenColor( basecol.complementaryColor() );
    bodyitm_->setPenColor( basecol );
    shadeitm1_->setPenColor( basecol.lighter( 1.5f ) );
    shadeitm2_->setPenColor( basecol.lighter( 4.0f ) );
    rectitm_->setPenColor( basecol );
    rectitm_->setFillColor( basecol );
}


void uiManipHandleItem::setLine( uiLineItem* li, int pos, bool setcursor )
{
    if ( ishor_ )
	li->setLine( 0, pos, scene_->nrPixX()-1, pos );
    else
	li->setLine( pos, 0, pos, scene_->nrPixY()-1 );
    if ( setcursor )
	li->setCursor( ishor_ ? MouseCursor::SizeVer : MouseCursor::SizeHor );
}


void uiManipHandleItem::updatePos()
{
    if ( !scene_ )
	{ pErrMsg("No scene, set it"); return; }

    setLine( centeritm_, pixpos_ );
    setLine( bodyitm_, pixpos_, true );
    setLine( shadeitm1_, pixpos_+1 );
    setLine( shadeitm2_, pixpos_+2 );

    const int sz = setup_.thickness_;
    const int wdth = 2 * sz + 1;
    const int x0 = ishor_ ? scene_->nrPixX()-1-sz : pixpos_-sz;
    const int y0 = ishor_ ? pixpos_-sz : 0;
    rectitm_->setRect( x0, y0, wdth, wdth );
}
