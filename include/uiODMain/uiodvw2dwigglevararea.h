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
namespace View2D { class Seismic; }


mExpClass(uiODMain) uiODVW2DWiggleVarAreaTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVW2DWiggleVarAreaTreeItem);
public:
    				uiODVW2DWiggleVarAreaTreeItem();
				~uiODVW2DWiggleVarAreaTreeItem();

    bool			select();
    bool			showSubMenu();

protected:

    bool			init();
    const char*			iconName() const;
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool			isSelectable() const	{ return true; }

    View2D::Seismic*		dummyview_;
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;

    void			createSelMenu(MenuItem&);
    bool			handleSelMenu(int mnuid);

    DataPack::ID		createDataPack(Attrib::SelSpec&,
					       const BufferString& attribnm="",
					       const bool steering=false,
					       const bool stored=false);

    void			checkCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			dataTransformCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
};


mExpClass(uiODMain) uiODVW2DWiggleVarAreaTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DWiggleVarAreaTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;
};
