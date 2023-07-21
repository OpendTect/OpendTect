#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"

mExpClass(uiODMain) uiODVolrenParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODVolrenParentTreeItem)
public:
			uiODVolrenParentTreeItem();
			~uiODVolrenParentTreeItem();

    static CNotifier<uiODVolrenParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
    bool		canAddVolumeToScene();
};


mExpClass(uiODMain) uiODVolrenTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODVolrenTreeItemFactory)
public:
    const char*		name() const   { return getName(); }
    static const char*	getName();
    uiTreeItem*		create() const { return new uiODVolrenParentTreeItem; }
    uiTreeItem*		createForVis(VisID,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODVolrenTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolrenTreeItem)
public:
			uiODVolrenTreeItem(VisID displayid=VisID::udf(),
					   bool rgba=false);

    bool		showSubMenu();

protected:
			~uiODVolrenTreeItem();

    bool		init();
    uiString    	createDisplayName() const;
    uiODDataTreeItem*	createAttribItem( const Attrib::SelSpec* ) const;
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    bool		isExpandable() const		{ return true; }
    const char*		parentType() const;

    MenuItem		positionmnuitem_;
    bool		rgba_;
};


mExpClass(uiODMain) uiODVolrenAttribTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODVolrenAttribTreeItem);
public:
			uiODVolrenAttribTreeItem(const char* parenttype);
			~uiODVolrenAttribTreeItem();

protected:

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    bool		hasTransparencyMenu() const;

    MenuItem		addmnuitem_;
    MenuItem		addisosurfacemnuitem_;
};



mExpClass(uiODMain) uiODVolrenSubTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolrenSubTreeItem);
public:
			uiODVolrenSubTreeItem(VisID displayid);

    bool		isIsoSurface() const;
    void		updateColumnText(int col);

protected:
			~uiODVolrenSubTreeItem();

    VisID		getParentDisplayID() const;
    int			getParentAttribNr() const;

    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		posChangeCB(CallBacker*);
    void		selChgCB(CallBacker*);

    bool		init();
    const char*		parentType() const;

    MenuItem		resetisosurfacemnuitem_;
    MenuItem		convertisotobodymnuitem_;
};
