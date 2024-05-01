#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "emposid.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vispolygonbodydisplay.h"
#include "visrandomposbodydisplay.h"


mExpClass(uiODMain) uiODBodyDisplayParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODBodyDisplayParentTreeItem)
public:
			uiODBodyDisplayParentTreeItem();

    static CNotifier<uiODBodyDisplayParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODBodyDisplayParentTreeItem();

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
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODBodyDisplayTreeItem)
public:
			uiODBodyDisplayTreeItem(const VisID&,bool dummy);
			uiODBodyDisplayTreeItem(const EM::ObjectID&);

    bool		isOK() const;
    EM::ObjectID	emObjectID() const	{ return emid_; }

    void		setOnlyAtSectionsDisplay(bool) override;

protected:
			~uiODBodyDisplayTreeItem();

private:
			uiODBodyDisplayTreeItem();

    static uiString	sCalcVolume() { return tr("Calculate Volume"); }
    static uiString	sPickedPolygons() { return tr("Picked Polygons"); }

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
    WeakPtr<visSurvey::PolygonBodyDisplay>	plg_;
    WeakPtr<visSurvey::MarchingCubesDisplay>	mcd_;
    WeakPtr<visSurvey::RandomPosBodyDisplay>	rpb_;

    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				displaybodymnuitem_;
    MenuItem				displaypolygonmnuitem_;
    MenuItem				displayintersectionmnuitem_;
    MenuItem				singlecolormnuitem_;
    MenuItem				volcalmnuitem_;

    ConstRefMan<visSurvey::PolygonBodyDisplay> getPBDisplay() const;
    ConstRefMan<visSurvey::MarchingCubesDisplay> getMCDisplay() const;
    ConstRefMan<visSurvey::RandomPosBodyDisplay> getRPBDisplay() const;
    RefMan<visSurvey::PolygonBodyDisplay> getPBDisplay();
    RefMan<visSurvey::MarchingCubesDisplay> getMCDisplay();
    RefMan<visSurvey::RandomPosBodyDisplay> getRPBDisplay();

public:
    void		displayAtSections(bool yn)
			{ setOnlyAtSectionsDisplay( yn ); }
};


mExpClass(uiODMain) uiODBodyDisplayDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODBodyDisplayDataTreeItem)
public:
			uiODBodyDisplayDataTreeItem(const char* parenttype);

protected:
			~uiODBodyDisplayDataTreeItem();

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    uiString		createDisplayName() const override;

    MenuItem		depthattribmnuitem_;
    MenuItem		isochronmnuitem_;
};
