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
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class MarchingCubesDisplay; class PolygonBodyDisplay;
		      class RandomPosBodyDisplay; }


mExpClass(uiODMain) uiODBodyDisplayParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODBodyDisplayParentTreeItem)
public:
			uiODBodyDisplayParentTreeItem();
			~uiODBodyDisplayParentTreeItem();

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
    virtual uiTreeItem*	createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODBodyDisplayTreeItem)
public:
			uiODBodyDisplayTreeItem(int,bool dummy);
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
    void		colorChCB(CallBacker*);
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

