#ifndef uiodvw2dfaulttreeitem_h
#define uiodvw2dfaulttreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: uiodvw2dfaulttreeitem.h,v 1.1 2010-06-24 08:54:11 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DFaut;


mClass uiODVw2DFaultParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DFaultParentTreeItem();
				~uiODVw2DFaultParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mClass uiODVw2DFaultTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*         name() const		{ return typeid(*this).name(); }
    uiTreeItem*         create() const
    			{ return new uiODVw2DFaultParentTreeItem(); }
};


mClass uiODVw2DFaultTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DFaultTreeItem(const EM::ObjectID&);
			~uiODVw2DFaultTreeItem();

    bool		showSubMenu();
    bool		select();

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		updateCS(const CubeSampling&,bool upd);
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);

    EM::ObjectID        emid_;
    VW2DFaut*		faultview_;
};


#endif
