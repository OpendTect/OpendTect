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
class SeisFlatDataPack;
namespace View2D { class Seismic; }


mExpClass(uiODMain) uiODView2DWiggleVarAreaTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DWiggleVarAreaTreeItem);
public:
				uiODView2DWiggleVarAreaTreeItem();
				~uiODView2DWiggleVarAreaTreeItem();

    bool			select() override;
    bool			showSubMenu() override;

protected:

    bool			init() override;
    const char*			iconName() const override;
    const char*			parentType() const override
				{ return typeid(uiODView2DTreeTop).name(); }
    bool			isSelectable() const override	{ return true; }

    View2D::Seismic*		dummyview_;
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;

    void			createSelMenu(MenuItem&);
    bool			handleSelMenu(int mnuid);

    mDeprecated("use createDataPackRM")
    DataPackID			createDataPack(Attrib::SelSpec&,
					       const BufferString& attribnm="",
					       const bool steering=false,
					       const bool stored=false);
    RefMan<SeisFlatDataPack>	createDataPackRM(Attrib::SelSpec&,
					       const BufferString& attribnm="",
					       const bool steering=false,
					       const bool stored=false);

    void			checkCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			dataTransformCB(CallBacker*) override;
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
};


mExpClass(uiODMain) uiODView2DWiggleVarAreaTreeItemFactory
				: public uiODView2DTreeItemFactory
{
public:
    const char*		name() const override	{ return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODView2DWiggleVarAreaTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,
				     const Vis2DID&) const override;
};
