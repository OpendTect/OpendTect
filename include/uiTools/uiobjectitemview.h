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

/*brief embeds some uiObjects in a graphicsview following a horrizontal layout
 !\only works with QT 4.5 or higher*/
#include "uigraphicsview.h"

class uiParent;
class uiObjectItem;

mClass uiObjectItemView : public uiGraphicsView
{
public:
			    	uiObjectItemView(uiParent*);
				~uiObjectItemView(){};

    void 			addItem(uiObjectItem*,int stretch=1);
    void 			insertItem(uiObjectItem*,int pos,int stretch=1);
    void 			removeItem(uiObjectItem*);

    int				nrItems() const { return objectitems_.size(); }
    
    uiObjectItem*		getItem(int idx);
    uiObjectItem*		getItemFromPos(const Geom::Point2D<int>&);
    void			getItemsFromRect(const uiRect&,
					       ObjectSet<uiObjectItem>&);

    int 			stretchFactor(uiObjectItem*);
    void 			setStretchFactor(uiObjectItem*,int sf);
    
    void			resetViewArea(CallBacker*);
    void			setSceneLayoutPos(float,float);
    
protected:

    ObjectSet<uiObjectItem>	objectitems_;
};

#endif
