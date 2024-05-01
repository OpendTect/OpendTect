#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"

#include "multiid.h"
#include "prestackevents.h"
#include "vispseventdisplay.h"

class uiODPSEventsTreeItem;

mExpClass(uiODMain) uiODPSEventsParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODPSEventsParentTreeItem)
public:
				uiODPSEventsParentTreeItem();

    SceneID			sceneID() const;

    static CNotifier<uiODPSEventsParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
				~uiODPSEventsParentTreeItem();

    bool			init() override;
    const char*			parentType() const override;
    const char*			iconName() const override;
    bool			showSubMenu() override;
    bool			loadPSEvent(MultiID&,BufferString&);

    uiODPSEventsTreeItem*	child_ = nullptr;
};


mExpClass(uiODMain) uiODPSEventsTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const override	{ return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODPSEventsParentTreeItem;}
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override
			{ return new uiODPSEventsParentTreeItem; }
};


mExpClass(uiODMain) uiODPSEventsTreeItem : public uiODDisplayTreeItem
{
public:
			uiODPSEventsTreeItem(const MultiID& key,const char*);

    void		updateScaleFactor(float);
    void		updateColorMode(int mode);

protected:
			~uiODPSEventsTreeItem();

private:
    const char*		parentType() const override
			{ return typeid(uiODPSEventsParentTreeItem).name();}

    virtual const char*	managerName() const { return "PreStackEvents"; }
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    bool		anyButtonClick(uiTreeViewItem*) override;
    void		updateColumnText(int) override;

    void		coltabChangeCB(CallBacker*);
    bool		init() override;
    void		updateDisplay();
    void		displayMiniColTab();

    RefMan<PreStack::EventManager> psem_;
    BufferString		eventname_;
    float			scalefactor_	= 1.f;
    Coord			dir_;
    MenuItem*			coloritem_;
    MultiID			key_;
    int				coloridx_	= 0;
    int				dispidx_	= 0;

    WeakPtr<visSurvey::PSEventDisplay>	eventdisplay_;

    ConstRefMan<visSurvey::PSEventDisplay> getDisplay() const;
    RefMan<visSurvey::PSEventDisplay> getDisplay();
};
