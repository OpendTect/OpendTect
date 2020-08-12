#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2010
RCS:           $Id: uiobjectitemview.h,v 1.1 2010-01-19 13:02:33 cvsbruno Exp
$
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
				~uiObjectItemView(){};

    virtual void 		addItem(uiObjectItem*,int stretch=1);
    virtual void		insertItem(uiObjectItem*,int pos,int stretch=1);

    virtual void 		removeItem(uiObjectItem*);
    virtual void		removeAllItems();

    int				nrItems() const { return objectitems_.size(); }
    
    uiObjectItem*		getItem(int idx);
    void			reSizeItem(int idx,const uiSize&);
    static void			reSizeItem(uiObjectItem*,const uiSize&);

    int 			stretchFactor(uiObjectItem*);
    void 			setStretchFactor(uiObjectItem*,int sf);
    
    void			resetViewArea(CallBacker*);
    virtual void		setSceneLayoutPos(uiPoint);
    virtual uiPoint		sceneLayoutPos() const;

    void			enableScrollBars(bool yn);

    uiObjectItem*		getItemFromPos(const Geom::Point2D<int>&);
    void			getItemsFromRect(const uiRect&,
					       ObjectSet<uiObjectItem>&);

    void			setCursor(const MouseCursor&);

    Notifier<uiObjectItemView> 	viewareareset;

protected:

    ObjectSet<uiObjectItem>	objectitems_;
};

