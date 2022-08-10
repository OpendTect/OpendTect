#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
#include "integerid.h"

class IOObj;
class uiRandomLinePolyLineDlg;
namespace Geometry { class RandomLineSet; }


mExpClass(uiODMain) uiODRandomLineParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODRandomLineParentTreeItem)
public:
			uiODRandomLineParentTreeItem();
			~uiODRandomLineParentTreeItem();

    static CNotifier<uiODRandomLineParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
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
    void		loadRandLineFromWell(CallBacker*);
    void		genFromPicks();
    void		rdlPolyLineDlgCloseCB(CallBacker*);
    void		removeChild(uiTreeItem*) override;
    uiRandomLinePolyLineDlg*	rdlpolylinedlg_;
};


namespace visSurvey { class RandomTrackDisplay; }


mExpClass(uiODMain) uiODRandomLineTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODRandomLineTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		createForVis(VisID visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODRandomLineTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODRandomLineTreeItem)
public:
    enum Type		{ Empty, Select, Default, RGBA };

			uiODRandomLineTreeItem(VisID displayid,Type tp=Empty,
				    RandomLineID id=RandomLineID::udf());
			~uiODRandomLineTreeItem();

    bool		init();
    bool		displayDefaultData();
    void		setRandomLineID(RandomLineID);

protected:

    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		changeColTabCB(CallBacker*);
    void		remove2DViewerCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		saveas2dmnuitem_;
    MenuItem		create2dgridmnuitem_;
    Type		type_;
    RandomLineID	rlid_;
};
