#ifndef uiodvw2dpicksettreeitem_h
#define uiodvw2dpicksettreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id: uiodvw2dpicksettreeitem.h,v 1.5 2012-08-03 13:01:05 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DPickSet;

namespace Pick{ class Set; class SetMgr; }



mClass(uiODMain) uiODVw2DPickSetParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DPickSetParentTreeItem();
				~uiODVw2DPickSetParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			pickSetAdded( CallBacker* );
    Pick::SetMgr&		picksetmgr_;
};


mClass(uiODMain) uiODVw2DPickSetTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*         name() const		{ return typeid(*this).name(); }
    uiTreeItem*         create() const
    			{ return new uiODVw2DPickSetParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mClass(uiODMain) uiODVw2DPickSetTreeItem : public uiODVw2DTreeItem
{
public:
			uiODVw2DPickSetTreeItem(int picksetid);
			~uiODVw2DPickSetTreeItem();

    bool		showSubMenu();
    bool		select();

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DPickSetParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		displayChangedCB(CallBacker*);
    void		displayMiniCtab();
    void		removePickSetCB(CallBacker*);

    const int 		cPixmapWidth()				{ return 16; }
    const int		cPixmapHeight()				{ return 10; }
    
    Pick::SetMgr&	picksetmgr_;
    Pick::Set&		pickset_;
    VW2DPickSet*	vw2dpickset_;
    int			setidx_;
};


#endif

