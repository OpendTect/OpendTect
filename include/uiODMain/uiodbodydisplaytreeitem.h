#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey
{
class MarchingCubesDisplay;
class PolygonBodyDisplay;
class RandomPosBodyDisplay;
}


mExpClass(uiODMain) uiODBodyDisplayParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODBodyDisplayParentTreeItem)
public:
			uiODBodyDisplayParentTreeItem();
			~uiODBodyDisplayParentTreeItem();

    static CNotifier<uiODBodyDisplayParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
    const char*		iconName() const override;
    bool		showSubMenu() override;
    void		loadBodies();
};


mExpClass(uiODMain) uiODBodyDisplayTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODBodyDisplayTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODBodyDisplayParentTreeItem; }
    virtual uiTreeItem*	createForVis(VisID visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODBodyDisplayTreeItem)
public:
			uiODBodyDisplayTreeItem(VisID,bool dummy);
			uiODBodyDisplayTreeItem(const EM::ObjectID&);
			~uiODBodyDisplayTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }
    void		setOnlyAtSectionsDisplay(bool);

protected:
    static uiString	sCalcVolume() { return tr("Calculate Volume"); }
    static uiString	sPickedPolygons() { return tr("Picked Polygons"); }

    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		keyPressedCB(CallBacker*);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);
    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;
    bool		createUiVisObj();

    bool		init();
    const char*		parentType() const
			{return typeid(uiODBodyDisplayParentTreeItem).name();}

    EM::ObjectID			emid_;
    visSurvey::MarchingCubesDisplay*	mcd_;
    visSurvey::PolygonBodyDisplay*	plg_;
    visSurvey::RandomPosBodyDisplay*	rpb_;

    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				displaybodymnuitem_;
    MenuItem				displaypolygonmnuitem_;
    MenuItem				displayintersectionmnuitem_;
    MenuItem				singlecolormnuitem_;
    MenuItem				volcalmnuitem_;

public:
    void		displayAtSections(bool yn)
			{ setOnlyAtSectionsDisplay( yn ); }
};


mExpClass(uiODMain) uiODBodyDisplayDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODBodyDisplayDataTreeItem)
public:
			uiODBodyDisplayDataTreeItem(const char* parenttype);
			~uiODBodyDisplayDataTreeItem();

protected:
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    uiString		createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		isochronmnuitem_;
};
