#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
namespace Pick		{ class Set; }


mDefineItem( PickSetParent, TreeItem, TreeTop, \
    ~uiODPickSetParentTreeItem(); \
    virtual bool init(); \
    virtual void removeChild(uiTreeItem*); \
    void setAdd(CallBacker*); \
    void setRm(CallBacker*); \
    bool display_on_add; \
    mShowMenu mMenuOnAnyButton );


mClass(uiODMain) uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;

};


mClass(uiODMain) uiODPickSetTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODPickSetTreeItem(int dispid,Pick::Set&);
    			~uiODPickSetTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    void		showAllPicks(bool yn);
    Pick::Set&		getSet()			{ return set_; }

protected:

    bool		init(); 
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChg(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
    			{ return typeid(uiODPickSetParentTreeItem).name(); }

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		storepolyasfaultmnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		convertbodymnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
};



#endif

