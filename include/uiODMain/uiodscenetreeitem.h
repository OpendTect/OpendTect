#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uimenuhandler.h"
#include "uiodtreeitem.h"

class uiMenuHandler;

mExpClass(uiODMain) uiODSceneTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODSceneTreeItem);
public:
			uiODSceneTreeItem(const uiString&,const SceneID&);

    void		updateColumnText(int) override;

protected:
			~uiODSceneTreeItem();

    bool		init() override;
    void		createMenu(MenuHandler*,bool istb);
    bool		showSubMenu() override;

    bool		isSelectable() const override	{ return false; }
    bool		isExpandable() const override	{ return false; }
    const char*		parentType() const override
			{ return typeid(uiODTreeTop).name(); }
    int			selectionKey() const override
			{ return displayid_.asInt(); }

    void		createMenuCB(CallBacker*);
    void		addToToolBarCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    SceneID		displayid_;

    RefMan<uiMenuHandler> menu_;
    MenuItem		propitem_;
    MenuItem		imageitem_;
    MenuItem		coltabitem_;
    MenuItem		dumpivitem_;
    MenuItem		workareaitem_;
};
