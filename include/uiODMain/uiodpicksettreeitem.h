#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodpicksettreeitem.h,v 1.2 2006-05-16 16:28:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"
namespace Pick		{ class Set; }
namespace visSurvey	{ class PickSetDisplay; }


mDefineItem( PickSetParent, TreeItem, TreeTop, mShowMenu );


class uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODPickSetTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODPickSetTreeItem(const Pick::Set&);
    			uiODPickSetTreeItem(int displayid);
    			~uiODPickSetTreeItem();
    virtual bool	actModeWhenSelected() const { return true; }
    void		showAllPicks(bool yn);

protected:
    bool		init();
    bool		askContinueAndSaveIfNeeded();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
    			{ return typeid(uiODPickSetParentTreeItem).name(); }
    void		saveCurSet(visSurvey::PickSetDisplay*);

    const Pick::Set*	ps_;

    MenuItem		renamemnuitem_;
    MenuItem		storemnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		showallmnuitem_;
    MenuItem		propertymnuitem_;
};


#endif
