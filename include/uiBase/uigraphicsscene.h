#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigraphicsitem.h"
#include "task.h"
#include "color.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "namedobj.h"


mFDQtclass(QGraphicsScene)
mFDQtclass(QGraphicsLinearLayout)
mFDQtclass(QGraphicsWidget)
mFDQtclass(ODGraphicsScene)

namespace OD { class ArrowStyle; class Alignment; class MarkerStyle2D; }

class uiPolygonItem;
class uiPolyLineItem;
class uiRectItem;
class uiObjectItem;

mExpClass(uiBase) uiGraphicsScene : public NamedCallBacker
{
public:
				uiGraphicsScene(const char*);
				~uiGraphicsScene();

    double			maxX() const;
    double			maxY() const;
    int				nrPixX() const;
    int				nrPixY() const;

    inline void			setEmpty()	    { removeAllItems(); }
    void			removeAllItems();
    uiGraphicsItem*		removeItem(uiGraphicsItem*);
				/*!<Gives object back to caller (not deleted) */
    uiGraphicsItemSet*		removeItems(uiGraphicsItemSet*);
				/*!<Does not delete the items*/

    template <class T> T*	addItem(T*);
				//!<Item becomes mine
    uiGraphicsItemSet*		addItems(uiGraphicsItemSet*);
    uiGraphicsItemGroup*	addItemGrp(uiGraphicsItemGroup*);
    int				nrItems() const;
    uiGraphicsItem*		getItem(int id);
    const uiGraphicsItem*	getItem(int id) const;
    uiGraphicsItem*		itemAt(const Geom::Point2D<float>&);
    const uiGraphicsItem*	itemAt(const Geom::Point2D<float>&) const;

    uiRectItem*			addRect(float x,float y,float w,float h);

    uiPolygonItem*		addPolygon(const TypeSet<uiPoint>&,bool fill);
    uiPolyLineItem*		addPolyLine(const TypeSet<uiPoint>&);

    void			useBackgroundPattern(bool);
    void			setBackGroundColor(const Color&);
    const Color			backGroundColor() const;

    int				getSelItemSize() const;
    uiRect			getSelectedArea() const;
    void			setSelectionArea(const uiRect&);

    MouseEventHandler&		getMouseEventHandler()
				{ return mousehandler_; }

    Notifier<uiGraphicsScene>	ctrlPPressed;
    Notifier<uiGraphicsScene>	ctrlCPressed;

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
    mQtclass(QGraphicsScene*)	qGraphicsScene()
			{return (mQtclass(QGraphicsScene*))odgraphicsscene_;}
    void			copyToClipBoard();


    void			addUpdateToQueue(Task*);
    bool			executePendingUpdates();

    void			setPixelDensity(float);
    float			getPixelDensity() const {return pixeldensity_;}
    static float		getDefaultPixelDensity();

    Notifier<uiGraphicsScene>	pixelDensityChange;
    mDeclInstanceCreatedNotifierAccess(uiGraphicsScene);

    void			translateText();

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
    float			pixeldensity_;

public:

    mDeprecated double		width() const	    { return maxX(); }
    mDeprecated double		height() const	    { return maxY(); }

};


template <class T>
inline T* uiGraphicsScene::addItem( T* itm )
{
    return (T*)doAddItem( itm );
}


mExpClass(uiBase) uiGraphicsObjectScene : public uiGraphicsScene
{
public:
				uiGraphicsObjectScene(const char*);

    void			addObjectItem(uiObjectItem*);
    void			insertObjectItem(int,uiObjectItem*);
    void			removeObjectItem(uiObjectItem*);
    void			setItemStretch(uiObjectItem*,int stretch);
    int			stretchFactor(uiObjectItem*) const;

    void			setLayoutPos(const uiPoint&);
    const uiPoint		layoutPos() const;
    const uiSize		layoutSize() const;

protected:

    void			resizeLayoutToContent();

    mQtclass(QGraphicsLinearLayout*)	layout_;
    mQtclass(QGraphicsWidget*)		layoutitem_;
};


mClass(uiBase) uiGraphicsSceneChanger : public Task
{
public:
    uiGraphicsSceneChanger(uiGraphicsScene&,uiGraphicsItem&,bool remove);
    uiGraphicsSceneChanger(uiGraphicsItemGroup&,uiGraphicsItem&,bool remove);

    bool execute();

protected:
    uiGraphicsScene*		scene_;
    uiGraphicsItemGroup*	group_;
    uiGraphicsItem&		itm_;
    bool			remove_;
};
