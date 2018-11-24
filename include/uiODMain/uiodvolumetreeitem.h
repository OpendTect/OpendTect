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
#include "trckeyzsampling.h"

mExpClass(uiODMain) uiODVolumeParentTreeItem
	: public uiODSceneProbeParentTreeItem
{   mODTextTranslationClass(uiODVolumeParentTreeItem);
    mDefineItemMembers(VolumeParent,SceneProbeParentTreeItem,SceneTreeTop);
    mMenuOnAnyButton;

public:

			~uiODVolumeParentTreeItem();

    virtual Probe*	createNewProbe() const;
    uiPresManagedTreeItem* addChildItem(const Presentation::ObjInfo&);
    bool		setProbeToBeAddedParams(int mnuid);

protected:

    bool		canAddVolumeToScene();
    TrcKeyZSampling	cs_;

};


mExpClass(uiODMain) uiODVolumeTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODVolumeTreeItemFactory);
public:
    const char*		name() const   { return getName(); }
    static const char*	getName();
    uiTreeItem*		create() const { return new uiODVolumeParentTreeItem; }
};


mExpClass(uiODMain) uiODVolumeTreeItem : public uiODSceneProbeTreeItem
{ mODTextTranslationClass(uiODVolumeTreeItem);
public:
			uiODVolumeTreeItem(Probe&,int displayid_=-1);
    bool		showSubMenu();

protected:

			~uiODVolumeTreeItem();

    bool		init();
    uiString		createDisplayName() const;
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    bool		isExpandable() const		{ return true; }
    const char*		parentType() const;

    MenuItem		positionmnuitem_;
};


mExpClass(uiODMain) uiODVolumeAttribTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODVolumeAttribTreeItem);
public:

			uiODVolumeAttribTreeItem(const char* parenttype);
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



mExpClass(uiODMain) uiODVolumeSubTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolumeSubTreeItem);
public:

			uiODVolumeSubTreeItem(int displayid);

    bool		isIsoSurface() const;
    void		updateColumnText(int col);

protected:
			~uiODVolumeSubTreeItem();

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
