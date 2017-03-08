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
class VW2DSeis;
class AttribProbeLayer;
namespace ColTab { class Sequence; };


mExpClass(uiODMain) uiODVW2DVariableDensityTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVW2DVariableDensityTreeItem);
public:

				uiODVW2DVariableDensityTreeItem();
				~uiODVW2DVariableDensityTreeItem();

    bool	select();
    bool                        showSubMenu();
    void			setAttribProbeLayer(AttribProbeLayer*);

protected:

    bool			init();
    const char*			iconName() const;
    void			initColTab();
    void			displayMiniCtab(const ColTab::Sequence*);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool			isSelectable() const            { return true; }

    RefMan<AttribProbeLayer>	attrlayer_;
    VW2DSeis*			dummyview_;
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;
    bool			coltabinitialized_;

    void			createSelMenu(MenuItem&);
    bool			handleSelMenu(int mnuid);

    void			checkCB(CallBacker*);
    void			colTabChgCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			attrLayerChangedCB(CallBacker*);
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
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;

};
