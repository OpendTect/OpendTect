#ifndef uiodvolrentreeitem_h
#define uiodvolrentreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id$
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
    uiTreeItem*		createForVis(int,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODVolrenTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODVolrenTreeItem)
public:
			uiODVolrenTreeItem(int displayid_=-1,bool rgba=false);
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
protected:

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    bool		hasTransparencyMenu() const;

    MenuItem            statisticsmnuitem_;
    MenuItem            amplspectrummnuitem_;
    MenuItem		addmnuitem_;
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

#endif
