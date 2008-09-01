#ifndef uigraphicsscene_h
#define uigraphicsscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.h,v 1.3 2008-09-01 07:41:24 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "namedobj.h"
#include "uigeom.h"
#include "uigraphicsitem.h"

class QGraphicsScene;
class ODGraphicsScene;

class ArrowStyle;
class ioPixmap;
class MarkerStyle2D;

class uiArrowItem;
class uiEllipseItem;
class uiLineItem;
class uiMarkerItem;
class uiPointItem;
class uiPolygonItem;
class uiPixmapItem;
class uiRectItem;
class uiTextItem;

class uiGraphicsScene : public NamedObject
{
public:
				uiGraphicsScene(const char*);
				~uiGraphicsScene();

    void			addItem(uiGraphicsItem*);
    void			removeItem(uiGraphicsItem*);
    void			addItemGrp(uiGraphicsItemGroup*);
    uiGraphicsItemGroup*	addItemGrp(ObjectSet<uiGraphicsItem>);
    uiTextItem*			addText(const char*);
    uiPixmapItem*		addPixmap(const ioPixmap&);
    uiRectItem*			addRect(float x,float y,float w,float h);
    uiEllipseItem*		addEllipse(float x,float y,float w,float h);
    uiEllipseItem*		addEllipse(const uiPoint& center,
	    				   const uiSize& sz);
    uiEllipseItem*		addCircle( const uiPoint& p, int r )
	                        { return addEllipse( p, uiSize(r,r) ); };

    uiLineItem*			addLine(float x1,float y1,float x2,float y2);
    uiLineItem*	                addLine(const uiPoint& pt1,const uiPoint& pt2);
    uiLineItem*        	        addLine(const uiPoint&,double angle,double len);
    uiPolygonItem*		addPolygon(const TypeSet<uiPoint>&,bool fill);
    uiPointItem*		addPoint(bool);
    uiMarkerItem*       	addMarker(const MarkerStyle2D&,int side=0);

    /*uiLineItem*               	addLine(const TypeSet<uiPoint>&,
	    				bool close);*/
    uiArrowItem*		addArrow( const uiPoint& head,
	    				  const uiPoint& tail,
					  const ArrowStyle& );

    void			useBackgroundPattern(bool);
    void			removeAllItems();
    void 			setBackGroundColor(const Color&);
    const Color			backGroundColor() const;

    MouseEventHandler&		getMouseEventHandler()	
    				{ return mousehandler_; }
    KeyboardEventHandler&	getKeyboardEventHandler()
    				{ return keyboardhandler_; }


    QGraphicsScene*		qGraphicsScene()
    				{ return qgraphicsscene_; }
    int				sceneitemsz();		
protected:

    QGraphicsScene*		qgraphicsscene_;
    ODGraphicsScene*		odgraphicsscene_;

    MouseEventHandler		mousehandler_;
    KeyboardEventHandler	keyboardhandler_;
};

#endif
