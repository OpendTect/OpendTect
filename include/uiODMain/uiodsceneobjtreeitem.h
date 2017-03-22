#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodscenetreeitem.h"

class uiMenuHandler;

mExpClass(uiODMain) uiODSceneObjTreeItem : public uiODSceneTreeItem
{ mODTextTranslationClass(uiODSceneObjTreeItem);
public:
			uiODSceneObjTreeItem(const uiString&,int);
			~uiODSceneObjTreeItem();

    void		updateColumnText(int);
    virtual int		sceneID() const;

protected:

    virtual bool	init();
    void		createMenu(MenuHandler*,bool istb);
    bool		showSubMenu();

    bool		isSelectable() const		{ return false; }
    bool		isExpandable() const		{ return false; }
    const char*		parentType() const
			{ return typeid(uiODSceneTreeTop).name(); }
    int			selectionKey() const		{ return displayid_; }

    void		createMenuCB(CallBacker*);
    void		addToToolBarCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    int			displayid_;

    uiMenuHandler*	menu_;
    MenuItem		propitem_;
    MenuItem		imageitem_;
    MenuItem		coltabitem_;
    MenuItem		dumpivitem_;
};
