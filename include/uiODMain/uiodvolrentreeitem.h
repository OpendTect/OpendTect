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
    const char*		name() const override	{ return getName(); }
    static const char*	getName();
    uiTreeItem*		create() const override
			{ return new uiODVolrenParentTreeItem; }

    uiTreeItem*		createForVis(VisID,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODVolrenTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolrenTreeItem)
public:
			uiODVolrenTreeItem(VisID displayid=VisID::udf(),
					   bool rgba=false);

    bool		showSubMenu() override;

protected:
			~uiODVolrenTreeItem();

    bool		init() override;
    uiString		createDisplayName() const override;
    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    bool		isExpandable() const override		{ return true; }
    const char*		parentType() const override;

    MenuItem		positionmnuitem_;
    bool		rgba_;
};


mExpClass(uiODMain) uiODVolrenAttribTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODVolrenAttribTreeItem);
public:
			uiODVolrenAttribTreeItem(const char* parenttype);
			~uiODVolrenAttribTreeItem();

protected:

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    bool		hasTransparencyMenu() const override;

    MenuItem		addmnuitem_;
    MenuItem		addisosurfacemnuitem_;
};



mExpClass(uiODMain) uiODVolrenSubTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolrenSubTreeItem);
public:
			uiODVolrenSubTreeItem(VisID displayid);

    bool		isIsoSurface() const;
    void		updateColumnText(int col) override;

protected:
			~uiODVolrenSubTreeItem();

    VisID		getParentDisplayID() const;
    int			getParentAttribNr() const;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		posChangeCB(CallBacker*);
    void		selChgCB(CallBacker*);

    bool		init() override;
    const char*		parentType() const override;

    MenuItem		resetisosurfacemnuitem_;
    MenuItem		convertisotobodymnuitem_;
};
