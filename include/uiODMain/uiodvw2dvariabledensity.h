#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "datapack.h"
#include "menuhandler.h"

class uiMenuHandler;
namespace ColTab { class Sequence; }
namespace View2D { class Seismic; }


mExpClass(uiODMain) uiODView2DVariableDensityTreeItem
				: public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DVariableDensityTreeItem);
public:
				uiODView2DVariableDensityTreeItem();
				~uiODView2DVariableDensityTreeItem();

    bool			select();
    bool			showSubMenu();

protected:

    bool			init();
    const char*			iconName() const;
    void			initColTab();
    void			displayMiniCtab(const ColTab::Sequence*);
    const char*			parentType() const
				{ return typeid(uiODView2DTreeTop).name(); }
    bool			isSelectable() const	{ return true; }

    View2D::Seismic*		dummyview_;
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;
    bool			coltabinitialized_;

    void			createSelMenu(MenuItem&);
    bool    			handleSelMenu(int mnuid);

    DataPackID			createDataPack(Attrib::SelSpec&,
					       const BufferString& attribnm="",
					       const bool steering=false,
					       const bool stored=false);

    void			checkCB(CallBacker*);
    void			colTabChgCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			dataTransformCB(CallBacker*);
    void			deSelectCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
};


mExpClass(uiODMain) uiODView2DVariableDensityTreeItemFactory
				: public uiODView2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODView2DVariableDensityTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,Vis2DID) const;
};
