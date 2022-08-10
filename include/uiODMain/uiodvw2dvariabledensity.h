#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "datapack.h"
#include "menuhandler.h"

class uiMenuHandler;
namespace ColTab { class Sequence; }
namespace View2D { class Seismic; }


mExpClass(uiODMain) uiODVW2DVariableDensityTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVW2DVariableDensityTreeItem);
public:
				uiODVW2DVariableDensityTreeItem();
				~uiODVW2DVariableDensityTreeItem();

    bool			select();
    bool			showSubMenu();

protected:

    bool			init();
    const char*			iconName() const;
    void			initColTab();
    void			displayMiniCtab(const ColTab::Sequence*);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool			isSelectable() const	{ return true; }

    View2D::Seismic*		dummyview_;
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;
    bool			coltabinitialized_;

    void			createSelMenu(MenuItem&);
    bool    			handleSelMenu(int mnuid);

    DataPack::ID		createDataPack(Attrib::SelSpec&,
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


mExpClass(uiODMain) uiODVW2DVariableDensityTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DVariableDensityTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};
