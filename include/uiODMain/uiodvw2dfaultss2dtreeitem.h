#ifndef uiodvw2dfaultss2dtreeitem_h
#define uiodvw2dfaultss2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dfaultss2dtreeitem.h,v 1.1 2010-06-24 08:54:11 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DFautSS2D;


mClass uiODVw2DFaultSS2DParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DFaultSS2DParentTreeItem();
				~uiODVw2DFaultSS2DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mClass uiODVw2DFaultSS2DTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DFaultSS2DParentTreeItem(); }
};


mClass uiODVw2DFaultSS2DTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DFaultSS2DTreeItem(const EM::ObjectID&);
			~uiODVw2DFaultSS2DTreeItem();

    bool		showSubMenu();
    bool		select();

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultSS2DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);

    EM::ObjectID	emid_;
    VW2DFautSS2D*	fssview_;
};


#endif
