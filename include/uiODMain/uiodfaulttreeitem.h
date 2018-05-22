#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"

class DataPointSet;


namespace visSurvey { class FaultDisplay; class FaultStickSetDisplay; }


mExpClass(uiODMain) uiODFaultParentTreeItem : public uiODSceneTreeItem
{ mODTextTranslationClass(uiODFaultParentTreeItem)
    typedef uiODSceneTreeItem inheritedClass;
public:
			uiODFaultParentTreeItem();
			~uiODFaultParentTreeItem();

protected:
			mMenuOnAnyButton
    const char*		iconName() const;
    bool		showSubMenu();
    const char*	parentType() const
			{ return typeid(uiODSceneTreeTop).name(); }
};


mExpClass(uiODMain) uiODFaultTreeItemFactory
    : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODFaultTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODFaultParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODFaultTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultTreeItem)
public:
			uiODFaultTreeItem(int,bool dummy);
			uiODFaultTreeItem(const DBKey&);
			~uiODFaultTreeItem();

    const DBKey&	emObjectID() const	{ return emid_; }

    static uiString	sFaultPlanes() { return tr("Fault Planes" ); }
    static uiString	sFaultSticks() { return tr("Fault Sticks" ); }
    static uiString	sOnlyAtSections() { return tr("Only at Sections"); }
    static uiString	sOnlyAtHorizons() { return tr("Only at Horizons"); }

protected:
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		prepareForShutdown();
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultParentTreeItem).name();}

    DBKey		emid_;

    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displaystickmnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			displayintersecthorizonmnuitem_;
    MenuItem			singlecolmnuitem_;
    visSurvey::FaultDisplay*	faultdisplay_;
};


mExpClass(uiODMain) uiODFaultStickSetParentTreeItem : public uiODSceneTreeItem
{   mODTextTranslationClass(uiODFaultStickSetParentTreeItem)
    mDefineItemMembers( FaultStickSetParent, SceneTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
};


mExpClass(uiODMain) uiODFaultStickSetTreeItemFactory
    : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODFaultStickSetTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODFaultStickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODFaultStickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultStickSetTreeItem)
public:
			uiODFaultStickSetTreeItem(int,bool dummy);
			uiODFaultStickSetTreeItem(const DBKey&);
			~uiODFaultStickSetTreeItem();

    const DBKey&	emObjectID() const	{ return emid_; }

protected:
    bool		askContinueAndSaveIfNeeded( bool withcancel );
    void		prepareForShutdown();
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultStickSetParentTreeItem).name();}


    DBKey		emid_;
    MenuItem				onlyatsectmnuitem_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    visSurvey::FaultStickSetDisplay*	faultsticksetdisplay_;
};


mExpClass(uiODMain) uiODFaultSurfaceDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODFaultSurfaceDataTreeItem)
public:
			uiODFaultSurfaceDataTreeItem(const DBKey&,
				const char* parenttype);

    void		setDataPointSet(const DataPointSet&);

protected:

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    uiString		createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;

    bool		changed_;
    DBKey	emid_;
};
