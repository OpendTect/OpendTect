#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodtreeitem.h"

class uiMenuHandler;

mExpClass(uiODMain) uiODSceneTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODSceneTreeItem);
public:
			uiODSceneTreeItem(const uiString&,int);
			~uiODSceneTreeItem();

    void		updateColumnText(int);

protected:

    virtual bool	init();
    void		createMenu(MenuHandler*,bool istb);
    bool		showSubMenu();

    bool		isSelectable() const		{ return false; }
    bool		isExpandable() const		{ return false; }
    const char*		parentType() const
			{ return typeid(uiODTreeTop).name(); }
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

