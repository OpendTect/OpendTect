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
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
    			{ return new uiODBodyDisplayParentTreeItem; }

    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODBodyDisplayTreeItem)
public:
			uiODBodyDisplayTreeItem(VisID,bool dummy);
			uiODBodyDisplayTreeItem(const EM::ObjectID&);
			~uiODBodyDisplayTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }
    void		setOnlyAtSectionsDisplay(bool) override;

protected:
    static uiString	sCalcVolume() { return tr("Calculate Volume"); }
    static uiString	sPickedPolygons() { return tr("Picked Polygons"); }

    void		prepareForShutdown() override;
    bool		askContinueAndSaveIfNeeded(bool withcancel) override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		keyPressedCB(CallBacker*);
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);
    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const override;
    bool		createUiVisObj();

    bool		init() override;
    const char*		parentType() const override
			{return typeid(uiODBodyDisplayParentTreeItem).name();}

    EM::ObjectID			emid_;
    visSurvey::MarchingCubesDisplay*	mcd_			= nullptr;
    visSurvey::PolygonBodyDisplay*	plg_			= nullptr;
    visSurvey::RandomPosBodyDisplay*	rpb_			= nullptr;

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
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    uiString		createDisplayName() const override;

    MenuItem		depthattribmnuitem_;
    MenuItem		isochronmnuitem_;
};
