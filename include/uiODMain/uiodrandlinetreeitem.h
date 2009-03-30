#ifndef uiodrandlinetreeitem_h
#define uiodrandlinetreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodrandlinetreeitem.h,v 1.11 2009-03-30 07:05:30 cvsraman Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

class IOObj;

mDefineItem( RandomLineParent, TreeItem, TreeTop, mShowMenu \
    bool load(const IOObj&); \
    const IOObj* selRandomLine(); \
    void genRandLine(int); \
    void genRandLineFromWell();\
    void loadRandLineFromWell(CallBacker*);\
    mMenuOnAnyButton
);

namespace visSurvey { class RandomTrackDisplay; };


mClass uiODRandomLineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODRandomLineTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODRandomLineTreeItem( int );
    bool		init();

protected:

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void                changeColTabCB(CallBacker*);
    void		remove2DViewerCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		saveas2dmnuitem_;
    MenuItem		create2dgridmnuitem_;
};


#endif
