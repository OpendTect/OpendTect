#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsview.h"

class uiParent;
class uiObjectItem;

/*!
\brief Embeds some uiObjects in a uiGraphicsView following a horizontal layout.
Objects can be dynamically removed and added from the graphicsview.
*/

mExpClass(uiTools) uiObjectItemView : public uiGraphicsView
{
public:
				uiObjectItemView(uiParent*);
				~uiObjectItemView();

    virtual void		addItem(uiObjectItem*,int stretch=1);
    virtual void		insertItem(uiObjectItem*,int pos,int stretch=1);

    virtual void		removeItem(uiObjectItem*);
    virtual void		removeAllItems();

    int				nrItems() const { return objectitems_.size(); }
    
    uiObjectItem*		getItem(int idx);
    void			reSizeItem(int idx,const uiSize&);
    static void			reSizeItem(uiObjectItem*,const uiSize&);

    int				stretchFactor(uiObjectItem*);
    void			setStretchFactor(uiObjectItem*,int sf);
    
    void			resetViewArea(CallBacker*);
    virtual void		setSceneLayoutPos(uiPoint);
    virtual uiPoint		sceneLayoutPos() const;

    void			enableScrollBars(bool yn);

    uiObjectItem*		getItemFromPos(const Geom::Point2D<int>&);
    void			getItemsFromRect(const uiRect&,
					       ObjectSet<uiObjectItem>&);

    void			setCursor(const MouseCursor&) override;

    Notifier<uiObjectItemView>	viewareareset;

protected:

    ObjectSet<uiObjectItem>	objectitems_;
};
