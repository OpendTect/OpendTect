#ifndef uiodvw2dfaulttreeitem_h
#define uiodvw2dfaulttreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DFault;
class uiODViewer2D;


mClass(uiODMain) uiODVw2DFaultParentTreeItem : public uiODVw2DTreeItem
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


mClass(uiODMain) uiODVw2DFaultTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*         name() const		{ return typeid(*this).name(); }
    uiTreeItem*         create() const
    			{ return new uiODVw2DFaultParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mClass(uiODMain) uiODVw2DFaultTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DFaultTreeItem(const EM::ObjectID&);
    			uiODVw2DFaultTreeItem(int dispid,bool dummy);
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
    void		displayMiniCtab();

    int 		cPixmapWidth()				{ return 16; }
    int			cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);

    EM::ObjectID        emid_;
    VW2DFault*		faultview_;
};


#endif

