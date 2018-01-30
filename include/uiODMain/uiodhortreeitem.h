#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodemsurftreeitem.h"

class uEMHorizonShiftDialog;

mExpClass(uiODMain) uiODHorizonParentTreeItem : public uiODSceneTreeItem
{ mODTextTranslationClass(uiODHorizonParentTreeItem)
    typedef uiODSceneTreeItem inheritedClass;
public:
			uiODHorizonParentTreeItem();
			~uiODHorizonParentTreeItem();

    virtual void	removeChild(uiTreeItem*);
    MenuItem		newmenu_;
			//!<Returns scene id
    CNotifier<uiODHorizonParentTreeItem,int> handleMenu;

protected:
			mMenuOnAnyButton
    const char*		iconName() const;
    bool		showSubMenu();
    virtual bool	addChld(uiTreeItem*,bool,bool);
    const char* parentType() const
			{ return typeid(uiODSceneTreeTop).name(); }

    void		sort();
    MenuItem		trackitem_;
    MenuItem		constzitem_;
};



mExpClass(uiODMain) uiODHorizonTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODHorizonTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODHorizonParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODHorizonTreeItem : public uiODEarthModelSurfaceTreeItem
{ mODTextTranslationClass(uiODHorizonTreeItem)
public:
			uiODHorizonTreeItem(int visid,bool rgba,
					    bool atsect,bool dummy);
			uiODHorizonTreeItem(const DBKey&,
					    bool rgba,bool atsect);

protected:
    bool		init();
    void		initMenuItems();
    void		initNotify();
    uiString	createDisplayName() const;
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizonParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

    uEMHorizonShiftDialog* horshiftdlg_;
    MenuItem		hordatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filterhormnuitem_;
    MenuItem		geom2attrmnuitem_;
    MenuItem		positionmnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		shiftmnuitem_;

    MenuItem		parentsmnuitem_;
    MenuItem		parentsrdlmnuitem_;
    MenuItem		childrenmnuitem_;
    MenuItem		delchildrenmnuitem_;
    MenuItem		lockmnuitem_;
    MenuItem		unlockmnuitem_;

    MenuItem		addinlitem_;
    MenuItem		addcrlitem_;
    MenuItem		addzitem_;

    bool		rgba_;
    bool		atsections_;
};


mExpClass(uiODMain) uiODHorizon2DParentTreeItem : public uiODSceneTreeItem
{   mODTextTranslationClass(uiODHorizon2DParentTreeItem);
    mDefineItemMembers( Horizon2DParent, SceneTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
    void            sort();
    virtual void    removeChild(uiTreeItem*);
    bool            addChld(uiTreeItem* child, bool below, bool downwards );

};


mExpClass(uiODMain) uiODHorizon2DTreeItemFactory
    : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODHorizon2DTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODHorizon2DParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODHorizon2DTreeItem : public uiODEarthModelSurfaceTreeItem
{ mODTextTranslationClass(uiODHorizon2DTreeItem)
public:
			uiODHorizon2DTreeItem(int visid,bool dummy);
			uiODHorizon2DTreeItem(const DBKey&);

protected:
    void		initMenuItems();
    void		initNotify();
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizon2DParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		derive3dhormnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		interpolatemnuitem_;
};
