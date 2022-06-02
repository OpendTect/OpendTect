#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigraphicsitem.h"
#include "uigeom.h"
#include "draw.h"

class uiPixmap;
class uiFont;
class uiGroup;
class uiObject;
class uiSize;
class uiRGBArray;
class FontData;

class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsPixmapItem;
class QGraphicsProxyWidget;
class QGraphicsRectItem;
class QPainterPath;
class QPolygonF;
class QSize;

class ODGraphicsAdvancedTextItem;
class ODGraphicsArrowItem;
class ODGraphicsMarkerItem;
class ODGraphicsPathItem;
class ODGraphicsPixmapItem;
class ODGraphicsPointItem;
class ODGraphicsPolyLineItem;
class ODGraphicsMultiColorPolyLineItem;
class ODGraphicsDynamicImageItem;
class ODGraphicsTextItem;
template <class T> class ODPolygon;


mExpClass(uiBase) uiObjectItem : public uiGraphicsItem
{
public:
				uiObjectItem(uiObject* obj);
				uiObjectItem(uiGroup* obj);
				~uiObjectItem();

    uiObject*			getObject();
    void			setObject(uiObject*);

    uiGroup*			getGroup();
    void			setGroup(uiGroup*);

    virtual void		setObjectSize(int,int);
    const uiSize		objectSize() const;

    QGraphicsProxyWidget*	qWidgetItem()	{ return qwidgetitem_; }

protected:

    QGraphicsItem*		mkQtObj();
    QGraphicsProxyWidget*	qwidgetitem_;
    uiObject*			obj_;
    uiGroup*			grp_;
};


mExpClass(uiBase) uiEllipseItem : public uiGraphicsItem
{
public:
				uiEllipseItem();
				uiEllipseItem(const uiSize&);
				uiEllipseItem(const uiPoint& center,
					      const uiSize&);
				~uiEllipseItem();

    QGraphicsEllipseItem*	qEllipseItem()	{ return qellipseitem_; }
    void			setSize(const uiSize&);

protected:

    QGraphicsItem*		mkQtObj();
    QGraphicsEllipseItem*	qellipseitem_;
};


mExpClass(uiBase) uiCircleItem : public uiEllipseItem
{
public:
				uiCircleItem();
				uiCircleItem(int r);
    				uiCircleItem(const uiPoint& center,int r);

    void			setRadius(int);
};



mExpClass(uiBase) uiLineItem : public uiGraphicsItem
{
public:
			uiLineItem();
			uiLineItem(int x1,int y1,int x2,int y2);
			uiLineItem(float x1,float y1,float x2,float y2);
			uiLineItem(const uiPoint& start,const uiPoint& stop);
			uiLineItem(const uiPoint&,float angle,float len);
			~uiLineItem();

    QGraphicsLineItem*	qLineItem()	{ return qlineitem_; }
    void		setPenStyle(const OD::LineStyle&,bool withalpha=false);
    void		setPenColor(const Color&,bool withalpha=false);
    void		setLine(const uiPoint& start,const uiPoint& end);
    void		setLine(const uiWorldPoint&,const uiWorldPoint&);
    void		setLine(int x1,int y1,int x2,int y2);
    void		setLine(float x1,float y1,float x2,float y2);
    void		setLine(const Geom::Point2D<float>&,
				const Geom::Point2D<float>&);
    void		setLine(const Geom::Point2D<float>& centerpos,
				float dx1,float dy1,float dx2,float dy2);
    void		setLine(const Geom::Point2D<int>& centerpos,
				int dx1,int dy1,int dx2,int dy2);
    uiRect		lineRect() const;

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsLineItem*	qlineitem_;
};


/*!Displays an image tied to a rectangle. There is one basic image (not dynamic)
  that provides a background model. The notifier will trigger if object
  wants a higher resolution version of the image. If so, that can be set
  by callint setImage with dynamic==true. */

mExpClass(uiBase) uiDynamicImageItem : public uiGraphicsItem
{
public:
				uiDynamicImageItem();
				~uiDynamicImageItem();

    void			setImage(bool dynamic,const uiRGBArray&,
					 const uiWorldRect&);
				/*!<If dynamic==false, worldrect will define
				    the bounding box of the item. */
    NotifierAccess&		wantsData();
    bool			isSnapshot() const;
				/*!<If set during a wantsData trigger, the
				  delivered image must have the exact size of
				  requested image.*/

    uiWorldRect			wantedWorldRect() const;
    uiSize			wantedScreenSize() const;

protected:
    QGraphicsItem*		mkQtObj();
    ODGraphicsDynamicImageItem*	item_;
};


mExpClass(uiBase) uiPixmapItem : public uiGraphicsItem
{
public:
				uiPixmapItem();
				uiPixmapItem(const uiPixmap&);
				uiPixmapItem(const uiPoint&,const uiPixmap&);
				~uiPixmapItem();

    ODGraphicsPixmapItem*	qPixmapItem()	{ return qpixmapitem_; }
    void			setOffset(int left,int top);
    void			setPixmap(const uiPixmap&);
    void			setPaintInCenter(bool);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPixmapItem*	qpixmapitem_;
};


mExpClass(uiBase) uiPolygonItem : public uiGraphicsItem
{
public:
			uiPolygonItem();
			uiPolygonItem(const TypeSet<uiPoint>&,bool fill);
			uiPolygonItem(const TypeSet<uiWorldPoint>&,
				      bool fill);
			uiPolygonItem(const ODPolygon<int>&,bool fill);
			~uiPolygonItem();

    void		fill();
    void		setPolygon(const TypeSet<uiPoint>&);
    void		setPolygon(const TypeSet<uiWorldPoint>&);
    void		setPolygon(const ODPolygon<int>&);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPolyLineItem*	qpolygonitem_;
};


mExpClass(uiBase) uiPolyLineItem : public uiGraphicsItem
{
public:
    			uiPolyLineItem();
    			uiPolyLineItem(const TypeSet<uiPoint>&);
    			uiPolyLineItem(const TypeSet<uiWorldPoint>&);
			~uiPolyLineItem();

    void		setPolyLine(const TypeSet<uiPoint>&);
    void		setPolyLine(const TypeSet<uiWorldPoint>&);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPathItem*		odgraphicspath_;
};


mExpClass(uiBase) uiMultiColorPolyLineItem : public uiGraphicsItem
{
public:
			uiMultiColorPolyLineItem();
			uiMultiColorPolyLineItem(const TypeSet<uiPoint>&);
			uiMultiColorPolyLineItem(const TypeSet<uiWorldPoint>&);
			~uiMultiColorPolyLineItem();

    void		setPolyLine(const TypeSet<uiPoint>&);
    void		setPolyLine(const TypeSet<uiWorldPoint>&);
    void		setColors(const TypeSet<Color>&,bool usetransp=false);

    void		setPenWidth(int);
    int			getPenWidth() const;

protected:

    QGraphicsItem*			mkQtObj();
    ODGraphicsMultiColorPolyLineItem*	odmulticoloritem_;
    int					penwidth_;
};


mExpClass(uiBase) uiRectItem : public uiGraphicsItem
{
public:
    			uiRectItem();
    			uiRectItem(int x,int y,int width,int height);
    			uiRectItem(QGraphicsRectItem*);
			~uiRectItem();

    QGraphicsRectItem*  qRectItem()	{ return qrectitem_; }
    void		setRect(int x,int y,int width,int height);
    void		setRect(float x,float y,float width,float height);
    void		setRect(const uiRect&);
    void		setRect(const Geom::RectF&);

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsRectItem*	qrectitem_;
};


mExpClass(uiBase) uiTextItem : public uiGraphicsItem
{
public:
			uiTextItem();
			uiTextItem(const uiString&,
				   const Alignment& al=Alignment());
			uiTextItem(const uiPoint&,const uiString&,
				   const Alignment& al=Alignment());
			~uiTextItem();

    void 		setFont(const uiFont&);
    void		setFontData(const FontData&);
    const uiString	getText() const;
    uiSize		getTextSize() const;
    void 		setAlignment(const Alignment&);
    void		setText(const uiString&);
    void		setTextColor(const Color&);

    void		setTextRotation(float angle);

protected:
			uiTextItem(QGraphicsItem*);
    void		translateText();

    ODGraphicsTextItem*	mkODObj();
    ODGraphicsTextItem*	qtextitem_;
    uiString		text_;

    virtual void	stPos(float,float);
};


mExpClass(uiBase) uiAdvancedTextItem : public uiGraphicsItem
{
public:
			uiAdvancedTextItem(bool centered=false);
			uiAdvancedTextItem(const uiString&,
					   const Alignment& =
				    Alignment(Alignment::Left,Alignment::Top),
					   bool centered=false);
			~uiAdvancedTextItem();

    Alignment		getAlignment() const;
    Color		getDefaultTextColor() const;
    uiFont&		getFont() const;
    uiString		getPlainText() const;
    float		getTextWidth() const;

    void		setAlignment(const Alignment&);
    void		setDefaultTextColor(const Color&);
    void		setFont(const uiFont&);
    void		setFont(const FontData&);
    void		setPlainText(const uiString&);
    void		setTextWidth(float);

    void		setTextIteraction(bool);

protected:
    Alignment		al_;
    bool		textiscentered_;
    QGraphicsItem*	mkQtObj();
    ODGraphicsAdvancedTextItem* qtextitem_;

    virtual void	stPos(float,float);
};


mExpClass(uiBase) uiMarkerItem : public uiGraphicsItem
{
public:
    				uiMarkerItem(bool fill=true);
				uiMarkerItem(const MarkerStyle2D&,
					     bool fill=true);
				uiMarkerItem(const uiPoint&,
					     const MarkerStyle2D&,
					     bool fill=true);
				~uiMarkerItem();

    ODGraphicsMarkerItem*	qMarkerItem()	{ return qmarkeritem_; }
    void			setMarkerStyle(const MarkerStyle2D&);
    void			setFill(bool);
    void			setFillColor(const Color&,bool withalpha=false);
    const MarkerStyle2D*	getMarkerStyle();

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsMarkerItem*	qmarkeritem_;
};


mExpClass(uiBase) uiPointItem : public uiGraphicsItem
{
public:
    				uiPointItem();
				uiPointItem(const uiPoint&);
				~uiPointItem();

    ODGraphicsPointItem*	qPointItem()		{ return qpointitem_; }

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPointItem*	qpointitem_;
};


mExpClass(uiBase) uiArrowItem : public uiGraphicsItem
{
public:
    				uiArrowItem();
				uiArrowItem(const uiPoint& tail,
					    const uiPoint& head,
					    const ArrowStyle&);
    				uiArrowItem(ODGraphicsArrowItem*);
				~uiArrowItem();

    void			setHeadPos(const uiPoint&);
    void			setTailPos(const uiPoint&);
    void			setTailHeadPos(const uiPoint& tail,
					       const uiPoint& head);
    void			setArrowStyle(const ArrowStyle&);
    void 			setArrowSize(int);

    ODGraphicsArrowItem*  	qArrowItem()	{ return qarrowitem_; }

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsArrowItem*	qarrowitem_;

    uiPoint			tailpos_;
    uiPoint			headpos_;

    void			update();
};


mExpClass(uiBase) uiCurvedItem : public uiGraphicsItem
{
public:
			uiCurvedItem(const uiPoint& startpt);
			uiCurvedItem(const Geom::Point2D<float>& startpt);
			~uiCurvedItem();

    mExpClass(uiBase) ArcSpec
    {
    public:
			ArcSpec( const uiPoint& c, float r,
				 const Interval<float>& angs )
			    : center_((float)c.x,(float)c.y), radius_(r)
			    , angles_(angs), yratio_(1.0f)		{}
			ArcSpec( const Geom::Point2D<float>& c, float r,
				 const Interval<float>& angs )
			    : center_(c), radius_(r)
			    , angles_(angs), yratio_(1.0f)		{}

	Geom::Point2D<float> center_;
	float		radius_;	//!< X radius. Yrad = radius_ * yratio_
	Interval<float>	angles_;
	float		yratio_;	//!< < 1 means: X size > Y size
    };

    mExpClass(uiBase) SplineSpec
    {
    public:
			SplineSpec( const uiPoint& endp, const uiPoint& cp )
			    : end_((float)endp.x,(float)endp.y)
			    , cp1_((float)cp.x,(float)cp.y)
			    , cubic_(false)				{}
			SplineSpec( const Geom::Point2D<float>& endp,
				    const Geom::Point2D<float>& cp )
			    : end_(endp), cp1_(cp), cubic_(false)	{}
			SplineSpec( const uiPoint& endp, const uiPoint& p1,
				    const uiPoint& p2 )
			    : end_((float)endp.x,(float)endp.y)
			    , cp1_((float)p1.x,(float)p1.y)
			    , cp2_((float)p2.x,(float)p2.y), cubic_(true)  {}
			SplineSpec( const Geom::Point2D<float>& endp,
				    const Geom::Point2D<float>& p1,
				    const Geom::Point2D<float>& p2 )
			    : end_(endp), cp1_(p1), cp2_(p2), cubic_(true) {}

	Geom::Point2D<float>	end_;
	Geom::Point2D<float>	cp1_;
	Geom::Point2D<float>	cp2_;	//!< only for cubic_
	bool			cubic_;	//!< otherwise quadratic
    };

    void		drawTo(const ArcSpec&);
    void		drawTo(const SplineSpec&);
    void		drawTo(const Geom::Point2D<float>&);	//!< line
    void		drawTo(const uiPoint&);

    void		closeCurve();				//!< line

    QGraphicsPathItem*	qGraphicsPathItem()	{ return qpathitem_; }
    QPainterPath*	qPainterPath()		{ return qppath_; }

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsPathItem*	qpathitem_;
    QPainterPath*	qppath_;

};


