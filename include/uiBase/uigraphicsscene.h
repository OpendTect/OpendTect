#ifndef uigraphicsscene_h
#define uigraphicsscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.h,v 1.39 2012-08-01 10:23:50 cvsmahant Exp $
________________________________________________________________________

-*/

#include "uigraphicsitem.h"
#include "task.h"
#include "bufstringset.h"
#include "color.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "namedobj.h"


class QGraphicsScene;
class QGraphicsLinearLayout;
class QGraphicsWidget;
class ODGraphicsScene;

class ArrowStyle;
class Alignment;
class MarkerStyle2D;

class uiPolygonItem;
class uiPolyLineItem;
class uiRectItem;
class uiObjectItem;

mClass uiGraphicsScene : public NamedObject
{
public:
				uiGraphicsScene(const char*);
				~uiGraphicsScene();

    void			removeAllItems();
    uiGraphicsItem*		removeItem(uiGraphicsItem*);
    				/*!<Gives object back to caller (not deleted) */
    void			removeItems(uiGraphicsItemSet&);
    				/*!<Does not delete the items*/

    template <class T> T*	addItem(T*);
    				//!<Item becomes mine
    uiGraphicsItemGroup*	addItemGrp(uiGraphicsItemGroup*);
    int				nrItems() const;
    uiGraphicsItem*		getItem(int id);
    const uiGraphicsItem*	getItem(int id) const;

    uiRectItem*			addRect(float x,float y,float w,float h);

    uiPolygonItem*		addPolygon(const TypeSet<uiPoint>&,bool fill);
    uiPolyLineItem*		addPolyLine(const TypeSet<uiPoint>&);

    void			useBackgroundPattern(bool);
    void 			setBackGroundColor(const Color&);
    const Color			backGroundColor() const;

    int				getSelItemSize() const;
    uiRect			getSelectedArea() const;
    void			setSelectionArea(const uiRect&);

    MouseEventHandler&		getMouseEventHandler()	
    				{ return mousehandler_; }

    Notifier<uiGraphicsScene>	ctrlPPressed;
    Notifier<uiGraphicsScene>	ctrlCPressed;
    double			width() const;
    double			height() const;

    int				getDPI() const;
    void			saveAsImage(const char*,int,int,int);
    void			saveAsPDF(const char*,int w,int h,int r);
    void			saveAsPS(const char*,int w,int h,int r);
    void			saveAsPDF_PS(const char*,bool pdf_or_ps,int w,
	    				     int h,int r);
    void			setSceneRect(float x,float y,float w,float h);
    uiRect			sceneRect();

    bool			isMouseEventActive() const	
    				{ return ismouseeventactive_; }
    void			setMouseEventActive( bool yn )
    				{ ismouseeventactive_ = yn; }
    QGraphicsScene*		qGraphicsScene()
    				{ return (QGraphicsScene*)odgraphicsscene_; }
    void			copyToClipBoard();


    void			addUpdateToQueue(Task*);
    bool			executePendingUpdates();
protected:

    ObjectSet<uiGraphicsItem>	items_;
    ODGraphicsScene*		odgraphicsscene_;

    void			CtrlCPressedCB(CallBacker*);
    MouseEventHandler		mousehandler_;
    bool			ismouseeventactive_;
    friend class		uiGraphicsItem;
    uiGraphicsItem*		doAddItem(uiGraphicsItem*);
    int				indexOf(int id) const;

    int				queueid_;
};


template <class T>
inline T* uiGraphicsScene::addItem( T* itm )
{
    return (T*) doAddItem( itm );
}


mClass uiGraphicsObjectScene : public uiGraphicsScene
{
public:
				uiGraphicsObjectScene(const char*);
    
    void                        addObjectItem(uiObjectItem*);
    void                        insertObjectItem(int,uiObjectItem*);
    void                        removeObjectItem(uiObjectItem*);
    void			setItemStretch(uiObjectItem*,int stretch);
    int 			stretchFactor(uiObjectItem*) const;
    
    void			setLayoutPos(const uiPoint&);
    const uiPoint		layoutPos() const;
    const uiSize		layoutSize() const;
     
protected:

    void 			resizeLayoutToContent();

    QGraphicsLinearLayout*      layout_;
    QGraphicsWidget*		layoutitem_;
};


class uiGraphicsSceneChanger : public Task
{
public:
    uiGraphicsSceneChanger( uiGraphicsScene& scene, uiGraphicsItem& itm,
			    bool remove );
    uiGraphicsSceneChanger( uiGraphicsItemGroup& scene, uiGraphicsItem& itm,
			    bool remove );

    bool execute();

protected:
    uiGraphicsScene*    	scene_;
    uiGraphicsItemGroup*	group_;
    uiGraphicsItem&     	itm_;
    bool                	remove_;
};



#endif
