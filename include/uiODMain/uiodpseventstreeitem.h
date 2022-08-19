#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"
#include "multiid.h"

class uiODPSEventsTreeItem;
namespace PreStack { class EventManager; }
namespace visSurvey { class PSEventDisplay; }

mExpClass(uiODMain) uiODPSEventsParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODPSEventsParentTreeItem)
public:
				uiODPSEventsParentTreeItem();
				~uiODPSEventsParentTreeItem();

    SceneID			sceneID() const;

    static CNotifier<uiODPSEventsParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
    bool			init();
    const char*			parentType() const;
    const char*			iconName() const;
    virtual bool		showSubMenu();
    bool			loadPSEvent(MultiID&,BufferString&);
    uiODPSEventsTreeItem*	child_;
};


mExpClass(uiODMain) uiODPSEventsTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const   { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPSEventsParentTreeItem;}
    uiTreeItem*		create(VisID visid,uiTreeItem*) const
			{ return new uiODPSEventsParentTreeItem; }
};


mExpClass(uiODMain) uiODPSEventsTreeItem : public uiODDisplayTreeItem
{
public:
			uiODPSEventsTreeItem(const MultiID& key,const char*);
			~uiODPSEventsTreeItem();

    void		updateScaleFactor(float);
    void		updateColorMode(int mode);

protected:
    virtual const char*	parentType() const
			{ return typeid(uiODPSEventsParentTreeItem).name();}

    virtual const char*	managerName() const { return "PreStackEvents"; }
    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);
    virtual bool	anyButtonClick(uiTreeViewItem*);
    virtual void	updateColumnText(int);

    void		coltabChangeCB(CallBacker*);
    bool		init();
    void		updateDisplay();
    void		displayMiniColTab();

    PreStack::EventManager&	psem_;
    BufferString		eventname_;
    float			scalefactor_;
    Coord			dir_;
    visSurvey::PSEventDisplay*	eventdisplay_;
    MenuItem*			coloritem_;
    MultiID			key_;
    int				coloridx_;
    int				dispidx_;
};
