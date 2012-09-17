#ifndef uigraphicsitemimpl_h
#define uigraphicsitemimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: uigraphicsitemimpl.h,v 1.33 2011/04/26 14:18:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsitem.h"
#include "uigeom.h"
#include "draw.h"

class Color;
class ioPixmap;
class uiFont;
class uiGroup;
class uiObject;
class uiSize;

class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsPixmapItem;
class QGraphicsPolygonItem;
class QGraphicsProxyWidget;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QPainterPath;
class QPolygon;
class QSize;

class ODGraphicsArrowItem;
class ODGraphicsMarkerItem;
class ODGraphicsPixmapItem;
class ODGraphicsPointItem;
class ODGraphicsPolyLineItem;
template <class T> class ODPolygon;


mClass uiObjectItem : public uiGraphicsItem
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


mClass uiEllipseItem : public uiGraphicsItem
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


mClass uiCircleItem : public uiEllipseItem
{
public:
				uiCircleItem();
				uiCircleItem(int r);
    				uiCircleItem(const uiPoint& center,int r);

    void			setRadius(int);
};


mClass uiLineItem : public uiGraphicsItem
{
public:
    			uiLineItem();
			uiLineItem(float x1,float y1,float x2,float y2,
				   bool abspos);
    			uiLineItem(const uiPoint& start,const uiPoint& stop,
				   bool abspos);
			uiLineItem(const uiPoint&,double angle,double len,
				   bool abspos);
			~uiLineItem();

    QGraphicsLineItem*	qLineItem()	{ return qlineitem_; }
    void 		setPenStyle(const LineStyle&,bool withalpha=false);
    void		setPenColor(const Color&,bool withalpha=false);
    void		setLine(const uiPoint& start,const uiPoint& end,
	    			bool abspos=true);
    void		setLine(int x1,int y1,int x2,int y2,bool abspos=true);
    void		setStartPos(const uiPoint&,bool abspos);
    void		setEndPos(const uiPoint&,bool abspos);
    uiRect		lineRect() const;

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsLineItem*	qlineitem_;
};


mClass uiPixmapItem : public uiGraphicsItem
{
public:
    				uiPixmapItem();
    				uiPixmapItem(const ioPixmap&);
    				uiPixmapItem(const uiPoint&,const ioPixmap&);
				~uiPixmapItem();

    ODGraphicsPixmapItem*	qPixmapItem()	{ return qpixmapitem_; }
    void			setOffset(int left,int top);
    void			setPixmap(const ioPixmap&);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsPixmapItem*	qpixmapitem_;
};


mClass uiPolygonItem : public uiGraphicsItem
{
public:
    			uiPolygonItem();
    			uiPolygonItem(const TypeSet<uiPoint>&,bool fill);
    			uiPolygonItem(const ODPolygon<int>&,bool fill);
    			uiPolygonItem(QGraphicsPolygonItem*);
			~uiPolygonItem();

    QGraphicsPolygonItem* qPolygonItem()	{ return qpolygonitem_; }
    void		fill();
    void		setPolygon(const TypeSet<uiPoint>&);
    void		setPolygon(const ODPolygon<int>&);

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsPolygonItem* qpolygonitem_;
};


mClass uiPolyLineItem : public uiGraphicsItem
{
public:
    				uiPolyLineItem();
    				uiPolyLineItem(const TypeSet<uiPoint>&);
				~uiPolyLineItem();

    int				nrSegments() const 
    					{ return polylines_.size(); }
    ODGraphicsPolyLineItem*	getSegment(int idx) const
    					{ return gtSegment(idx); }

    void			setPolyLine(const TypeSet<uiPoint>&);

    //TODO remove this when qgraphicsitemgroup can support it :
    void        		setPenStyle(const LineStyle&,bool alpha=false);
    void        		setPenColor(const Color&,bool withalpha=false);
    void        		setFillColor(const Color&,bool withalpha=false);

    QGraphicsItemGroup* 	qPolyLineItemGroup()
					{ return qgraphicsitemgrp_; }

protected:

    void			setPolyLine(const QPolygon&);
    ODGraphicsPolyLineItem*	getEmptyPolyLine();
    ODGraphicsPolyLineItem*	gtSegment(int idx) const
				{
				    return !polylines_.validIdx(idx) ? 0 
				    : const_cast<ODGraphicsPolyLineItem*>( 
					    polylines_[idx] );
				}


    QGraphicsItem*		mkQtObj();
    QGraphicsItemGroup*		qgraphicsitemgrp_;
    ObjectSet<ODGraphicsPolyLineItem> polylines_;
};



mClass uiRectItem : public uiGraphicsItem
{
public:
    			uiRectItem();
    			uiRectItem(int x,int y,int width,int height);
    			uiRectItem(QGraphicsRectItem*);
			~uiRectItem();

    QGraphicsRectItem*  qRectItem()	{ return qrectitem_; }
    void		setRect(int x,int y,int width,int height); 

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsRectItem*	qrectitem_;
};


mClass uiTextItem : public uiGraphicsItem
{
public:
			uiTextItem();
    			uiTextItem(const char*,const Alignment& al=Alignment());
    			uiTextItem(const uiPoint&,const char*,
				   const Alignment& al=Alignment());
			~uiTextItem();

    void 		setFont(const uiFont&);
    uiSize		getTextSize() const;
    void 		setAlignment(const Alignment&);
    void 		setText(const char*); 
    void		setHtmlText(const char*);
    void		setTextColor(const Color&);

    void		enableBackground(bool);
    bool		backgroundEnabled() const;
    void		setBackgroundColor(const Color&);
    Color		getBackgroundColor() const;

    QGraphicsTextItem*  qTextItem()	{ return qtextitem_; }

protected:

    QGraphicsItem*	mkQtObj();
    QGraphicsTextItem*	qtextitem_;

    Alignment		al_;
    uiPoint		pos_;

    void		updatePos();
    virtual void	stPos(int,int);
};


mClass uiMarkerItem : public uiGraphicsItem
{
public:
    				uiMarkerItem(bool fill=true);
				uiMarkerItem(const MarkerStyle2D&,
					     bool fill=true);
    				uiMarkerItem(const uiPoint&,
					     const MarkerStyle2D&,
					     bool fill=true);
				~uiMarkerItem();

    ODGraphicsMarkerItem*  	qMarkerItem()	{ return qmarkeritem_; }
    void			setMarkerStyle(const MarkerStyle2D&);
    void			setFill(bool);
    void			setFillColor(const Color&,bool withalpha=false);

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsMarkerItem*	qmarkeritem_;
};


mClass uiPointItem : public uiGraphicsItem
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


mClass uiArrowItem : public uiGraphicsItem
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
    void                        setArrowStyle(const ArrowStyle&);
    void 			setArrowSize(int);

    ODGraphicsArrowItem*  	qArrowItem()	{ return qarrowitem_; }

protected:

    QGraphicsItem*		mkQtObj();
    ODGraphicsArrowItem*	qarrowitem_;

    uiPoint			tailpos_;
    uiPoint			headpos_;

    void			update();
};


mClass uiCurvedItem : public uiGraphicsItem
{
public:
			uiCurvedItem(const uiPoint& startpt);
			uiCurvedItem(const Geom::Point2D<float>& startpt);
			~uiCurvedItem();

    mClass ArcSpec
    {
    public:
			ArcSpec( const uiPoint& c, float r,
				 const Interval<float>& angs )
			    : center_(c.x,c.y), radius_(r)
			    , angles_(angs), yratio_(1)		{}
			ArcSpec( const Geom::Point2D<float>& c, float r,
				 const Interval<float>& angs )
			    : center_(c), radius_(r)
			    , angles_(angs), yratio_(1)		{}

	Geom::Point2D<float> center_;
	float		radius_;	//!< X radius. Yrad = radius_ * yratio_
	Interval<float>	angles_;
	float		yratio_;	//!< < 1 means: X size > Y size
    };

    mClass SplineSpec
    {
    public:
			SplineSpec( const uiPoint& endp, const uiPoint& cp )
			    : end_(endp.x,endp.y), cp1_(cp.x,cp.y)
			    , cubic_(false)				{}
			SplineSpec( const Geom::Point2D<float>& endp,
				    const Geom::Point2D<float>& cp )
			    : end_(endp), cp1_(cp), cubic_(false)	{}
			SplineSpec( const uiPoint& endp, const uiPoint& p1,
				    const uiPoint& p2 )
			    : end_(endp.x,endp.y), cp1_(p1.x,p1.y)
			    , cp2_(p2.x,p2.y), cubic_(true)		{}
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


#endif
