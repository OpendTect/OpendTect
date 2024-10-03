#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "integerid.h"
#include "uioddisplaytreeitem.h"
#include "visrandomtrackdisplay.h"

class IOObj;
class uiRandomLinePolyLineDlg;
namespace Geometry { class RandomLineSet; }


mExpClass(uiODMain) uiODRandomLineParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODRandomLineParentTreeItem)
public:
			uiODRandomLineParentTreeItem();

    static CNotifier<uiODRandomLineParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODRandomLineParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
    bool		load(const IOObj&,int);
    bool		addStored(int);
    void		genRandLine(int);
    void		genFromContours();
    void		genFromExisting();
    void		genFromPolygon();
    void		genFromTable();
    void		genFromWell();
    void		exportToGIS();
    void		loadRandLineFromWell(CallBacker*);
    void		genFromPicks();
    void		rdlPolyLineDlgCloseCB(CallBacker*);
    void		removeChild(uiTreeItem*) override;
    uiRandomLinePolyLineDlg*	rdlpolylinedlg_	= nullptr;
};


namespace visSurvey { class RandomTrackDisplay; }


mExpClass(uiODMain) uiODRandomLineTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODRandomLineTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODRandomLineTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODRandomLineTreeItem)
public:
    enum Type		{ Empty, Select, Default, RGBA };

			uiODRandomLineTreeItem(const VisID&,Type =Empty,
				    const RandomLineID& =RandomLineID::udf());

    bool		init() override;
    bool		displayDefaultData();
    bool		displayData(const Attrib::SelSpec*);
    void		setRandomLineID(const RandomLineID&);

    ConstRefMan<visSurvey::RandomTrackDisplay> getDisplay() const;

protected:
			~uiODRandomLineTreeItem();

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		changeColTabCB(CallBacker*);
    void		remove2DViewerCB(CallBacker*);
    const char*		parentType() const override
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		saveas2dmnuitem_;
    MenuItem		create2dgridmnuitem_;

    RefMan<visSurvey::RandomTrackDisplay> getDisplay();

    WeakPtr<visSurvey::RandomTrackDisplay> rtd_;
    Type		type_;
    RandomLineID	rlid_;
};
