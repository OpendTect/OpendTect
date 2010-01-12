#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodpicksettreeitem.h,v 1.17 2010-01-12 09:25:00 cvsranojay Exp $
________________________________________________________________________


-*/

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


mClass uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;

};


mClass uiODPickSetTreeItem : public uiODDisplayTreeItem
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
    bool		askContinueAndSaveIfNeeded();
    void		setChg(CallBacker*);
    void		createMenuCB(CallBacker*);
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
    MenuItem		removeselectionmnuitem_;
};



#endif
