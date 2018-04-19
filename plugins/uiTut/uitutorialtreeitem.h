#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Asif Bahrainwala
 Date:          April 2018
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uiodsceneparenttreeitem.h"
#include "uioddisplaytreeitem.h"
#include "vistutorialdisplay.h"

class uiODTutorialParentTreeItem : public uiODSceneParentTreeItem
{
    mODTextTranslationClass(uiODTutorialParentTreeItem)
	/* to define tr(...) */

    mDefineItemMembers( TutorialParent, SceneParentTreeItem,
							SceneTreeTop);
    mShowMenu;
    mMenuOnAnyButton;

    const char*			childObjTypeKey() const;
    void			addWells();
    bool			handleSubMenu(int);
    DBKeySet			wellids_;
};


class uiODTutorialParentTreeItemfactory : public uiODSceneTreeItemFactory
{
public:
    const char*			name() const;
    uiTreeItem*			create() const;
};


class uiODTutorialTreeItem : public uiODDisplayTreeItem
{
public:
				uiODTutorialTreeItem(const DBKey&);

    DBKey			key_;

    mODTextTranslationClass(uiODTutorialParentTreeItem)
    mDefineItemMembers(Tutorial,DisplayTreeItem,TutorialParentTreeItem);

protected:

    virtual bool		init();
};
