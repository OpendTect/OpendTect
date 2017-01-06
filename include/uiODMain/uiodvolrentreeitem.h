#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodprobeparenttreeitem.h"
#include "uiodattribtreeitem.h"

mExpClass(uiODMain) uiODVolrenParentTreeItem
	: public uiODSceneProbeParentTreeItem
{   mODTextTranslationClass(uiODVolrenParentTreeItem);
    mDefineItemMembers(VolrenParent,SceneProbeParentTreeItem,SceneTreeTop);
    mMenuOnAnyButton;
public:
			~uiODVolrenParentTreeItem();
    virtual Probe*	createNewProbe() const;
    uiODPrManagedTreeItem* addChildItem(const OD::ObjPresentationInfo&);
    bool		setProbeToBeAddedParams(int mnuid);

protected:

    bool		canAddVolumeToScene();
};


mExpClass(uiODMain) uiODVolrenTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODVolrenTreeItemFactory);
public:
    const char*		name() const   { return getName(); }
    static const char*	getName();
    uiTreeItem*		create() const { return new uiODVolrenParentTreeItem; }
    uiTreeItem*		createForVis(int,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODVolrenTreeItem : public uiODSceneProbeTreeItem
{ mODTextTranslationClass(uiODVolrenTreeItem);
public:
			uiODVolrenTreeItem(Probe&,int displayid_=-1);
    bool		showSubMenu();

protected:
			~uiODVolrenTreeItem();
    bool		init();
    uiString		createDisplayName() const;
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    bool		isExpandable() const		{ return true; }
    const char*		parentType() const;

    MenuItem		positionmnuitem_;
};


mExpClass(uiODMain) uiODVolrenAttribTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODVolrenAttribTreeItem);
public:
			uiODVolrenAttribTreeItem(const char* parenttype);
    static void		initClass();
    static uiODDataTreeItem* create(ProbeLayer&);

protected:

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    bool		hasTransparencyMenu() const;

    MenuItem            statisticsmnuitem_;
    MenuItem            amplspectrummnuitem_;
    MenuItem		addisosurfacemnuitem_;
};



mExpClass(uiODMain) uiODVolrenSubTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolrenSubTreeItem);
public:
			uiODVolrenSubTreeItem(int displayid);

    bool		isIsoSurface() const;
    void		updateColumnText(int col);

protected:
			~uiODVolrenSubTreeItem();

    int			getParentDisplayID() const;
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
