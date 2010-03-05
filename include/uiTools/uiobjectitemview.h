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

#include "uigraphicsview.h"

class uiParent;
class uiAttrItem;
class uiLogItem;
class uiObjectItem;

mClass uiObjectItemView : public uiGraphicsView
{
public:
			    	uiObjectItemView(uiParent*);
				~uiObjectItemView(){};

    void 			addItem(uiObjectItem*,int stretch);
    void 			insertItem(uiObjectItem*,int pos,int stretch);
    void 			removeItem(uiObjectItem*);
    
    uiObjectItem*		getItemFromPos(const Geom::Point2D<int>&);
    uiObjectItem*		getItem(int idx); 
    int				nrItems() const { return objectitems_.size(); }
    int 			stretchFactor(uiObjectItem*);
    void 			setStretchFactor(uiObjectItem*,int sf);
    void			resetViewArea(CallBacker*);

protected:

    ObjectSet<uiObjectItem>	objectitems_;
};

#endif
