#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodsceneparenttreeitem.h"
#include "uioddisplaytreeitem.h"

namespace Pick		{ class Set; }



mExpClass(uiODMain) uiODPickSetParentTreeItem : public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODPickSetParentTreeItem);
    mDefineItemMembers( PickSetParent, SceneParentTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
		~uiODPickSetParentTreeItem();
    void	addChildItem(const DBKey&);
    const char* childObjTypeKey() const;
};


mExpClass(uiODMain) uiODPickSetTreeItemFactory : public uiODSceneTreeItemFactory
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
    const char*		objectTypeKey() const;

protected:

    bool		init();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChgCB(CallBacker*);
    void		selChangedCB(CallBacker*);
    bool		doubleClick(uiTreeViewItem*);
    const char*		parentType() const
			{ return typeid(uiODPickSetParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);
    virtual void	keyPressCB(CallBacker*);

    OD::ObjPresentationInfo*	getObjPRInfo();

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


mExpClass(uiODMain) uiODPolygonParentTreeItem : public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODPolygonParentTreeItem);
    mDefineItemMembers( PolygonParent, SceneParentTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;

		~uiODPolygonParentTreeItem();

    void	addChildItem(const DBKey&);
    const char* childObjTypeKey() const;
    OD::ObjPresentationInfo*	getObjPRInfo();
};


mExpClass(uiODMain) uiODPolygonTreeItemFactory : public uiODSceneTreeItemFactory
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
    const char*		objectTypeKey() const;

protected:

    bool		init();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChgCB(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		selChangedCB(CallBacker*);
    bool		doubleClick(uiTreeViewItem*);
    const char*		parentType() const
			{ return typeid(uiODPolygonParentTreeItem).name(); }

    OD::ObjPresentationInfo*	getObjPRInfo();

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
};
