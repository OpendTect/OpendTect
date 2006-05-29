#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodpicksettreeitem.h,v 1.3 2006-05-29 08:02:32 cvsbert Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"
namespace Pick		{ class Set; }


mDefineItem( PickSetParent, TreeItem, TreeTop, \
    ~uiODPickSetParentTreeItem(); \
    bool init(); \
    void setAdd(CallBacker*); \
    void setRm(CallBacker*); \
    bool display_on_add; \
    mShowMenu );


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
    			uiODPickSetTreeItem(int dispid,Pick::Set&);
    			~uiODPickSetTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    void		showAllPicks(bool yn);
    Pick::Set&		getSet()			{ return set_; }

protected:

    bool		init();
    bool		askContinueAndSaveIfNeeded();
    void		setChg(CallBacker*);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
    			{ return typeid(uiODPickSetParentTreeItem).name(); }

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		showallmnuitem_;
    MenuItem		propertymnuitem_;
};


#endif
