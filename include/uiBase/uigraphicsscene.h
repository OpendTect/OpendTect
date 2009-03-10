#ifndef uigraphicsscene_h
#define uigraphicsscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.h,v 1.15 2009-03-10 06:33:51 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "color.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "namedobj.h"
#include "uigeom.h"


class QGraphicsScene;
class ODGraphicsScene;

class ArrowStyle;
class Alignment;
class ioPixmap;
class MarkerStyle2D;

class uiArrowItem;
class uiDialog;
class uiEllipseItem;
class uiGraphicsItem;
class uiGraphicsItemGroup;
class uiLineItem;
class uiMarkerItem;
class uiPixmapItem;
class uiPointItem;
class uiPolygonItem;
class uiPolyLineItem;
class uiRect;
class uiRectItem;
class uiTextItem;

mClass uiGraphicsScene : public NamedObject
{
public:
				uiGraphicsScene(const char*);
				~uiGraphicsScene();

    void			removeAllItems();
    void			removeItem(uiGraphicsItem*);

    void			addItem(uiGraphicsItem*);
    void			addItemGrp(uiGraphicsItemGroup*);
    int				nrItems() const;
    int				getDPI() const;

    uiTextItem*			addText(const char*);
    uiTextItem*                 addText(int x,int y,const char*,
	                                const Alignment&);
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
    uiPolyLineItem*		addPolyLine(const TypeSet<uiPoint>&);
    uiPointItem*		addPoint(bool);
    uiMarkerItem*       	addMarker(const MarkerStyle2D&,int side=0);

    uiArrowItem*		addArrow(const uiPoint& head,
	    				 const uiPoint& tail,
					 const ArrowStyle&);

    void			useBackgroundPattern(bool);
    void 			setBackGroundColor(const Color&);
    const Color			backGroundColor() const;

    int				getSelItemSize() const;
    uiRect			getSelectedArea() const;
    void			setSelectionArea(const uiRect&);

    MouseEventHandler&		getMouseEventHandler()	
    				{ return mousehandler_; }
    KeyboardEventHandler&	getKeyboardEventHandler()
    				{ return keyboardhandler_; }

    double			width() const;
    double			height() const;

    void			saveAsImage(const char*,int,int,int);
    void			saveAsPDF(const char*,int);
    void			saveAsPS(const char*,int);
    void			saveAsPDF_PS(const char*,bool pdf_or_ps,int);
    void			setSceneRect(float x,float y,float w,float h);
    uiRect			sceneRect();

    const bool			isMouseEventActive() const	
    				{ return ismouseeventactive_; }
    void			setMouseEventActive( bool yn )
    				{ ismouseeventactive_ = yn; }
    QGraphicsScene*		qGraphicsScene()
    				{ return (QGraphicsScene*)odgraphicsscene_; }

    BufferStringSet		supportedImageFormat();
    void			save();
    void			setSaveDialog( uiDialog* dlg )
				{ savedlg_ = dlg; }

protected:

    ObjectSet<uiGraphicsItem>	items_;
    ODGraphicsScene*		odgraphicsscene_;

    MouseEventHandler		mousehandler_;
    KeyboardEventHandler	keyboardhandler_;
    bool			ismouseeventactive_;

    uiDialog*			savedlg_;
};
#endif
