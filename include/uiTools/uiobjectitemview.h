#ifndef uiobjectitemview_h
#define uiobjectitemview_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2010
RCS:           $Id: uiobjectitemview.h,v 1.1 2010-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/


/*! brief embeds some uiObjects in a graphicsview following a horizontal layout.
    Objects can be dynamically removed and added from the graphicsview !*/

#include "uigraphicsview.h"

class uiParent;
class uiObjectItem;

mClass uiObjectItemView : public uiGraphicsView
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

    int 			stretchFactor(uiObjectItem*);
    void 			setStretchFactor(uiObjectItem*,int sf);
    
    void			resetViewArea(CallBacker*);
    void			setSceneLayoutPos(float,float);

    void			enableScrollBars(bool yn);

    uiObjectItem*		getItemFromPos(const Geom::Point2D<int>&);
    void			getItemsFromRect(const uiRect&,
					       ObjectSet<uiObjectItem>&);
    
protected:

    ObjectSet<uiObjectItem>	objectitems_;
};

#endif
