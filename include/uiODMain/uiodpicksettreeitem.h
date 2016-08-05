#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodparenttreeitem.h"
#include "uioddisplaytreeitem.h"

namespace Pick		{ class Set; }



mExpClass(uiODMain) uiODPickSetParentTreeItem : public uiODParentTreeItem
{   mODTextTranslationClass(uiODPickSetParentTreeItem);
    mDefineItemMembers( PickSetParent, TreeItem, TreeTop );
    mShowMenu;
    mMenuOnAnyButton;
		~uiODPickSetParentTreeItem();
    void	addPickSet(Pick::Set*);
    void	removeSet(Pick::Set&);
    void	addChildItem(const MultiID&);
};


mExpClass(uiODMain) uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPickSetTreeItemFactory)
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPickSetTreeItem)
public:
			uiODPickSetTreeItem(int dispid,Pick::Set&);
			~uiODPickSetTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    Pick::Set&		getSet()			{ return set_; }
    const Pick::Set&	getSet() const			{ return set_; }

protected:

    bool		init();
    void		handleItemCheck();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChgCB(CallBacker*);
    void		selChangedCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODPickSetParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);
    virtual void	keyPressCB(CallBacker*);

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		convertbodymnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		undomnuitem_;
    MenuItem		redomnuitem_;
};


mExpClass(uiODMain) uiODPolygonParentTreeItem : public uiODParentTreeItem
{   mODTextTranslationClass(uiODPolygonParentTreeItem);
    mDefineItemMembers( PolygonParent, TreeItem, TreeTop );
    mShowMenu;
    mMenuOnAnyButton;

		~uiODPolygonParentTreeItem();

    void	addPolygon(Pick::Set*);
    void	removeSet(Pick::Set&);
    void	addChildItem(const MultiID&);
};


mExpClass(uiODMain) uiODPolygonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPolygonTreeItemFactory)
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPolygonParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPolygonTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPolygonTreeItem)
public:
			uiODPolygonTreeItem(int dispid,Pick::Set&);
			~uiODPolygonTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    Pick::Set&		getSet()			{ return set_; }
    const Pick::Set&	getSet() const			{ return set_; }

protected:

    bool		init();
    void		prepareForShutdown();
    void		handleItemCheck();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChgCB(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		selChangedCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODPolygonParentTreeItem).name(); }

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
};


#endif
