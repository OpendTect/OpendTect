#ifndef uiodpseventstreeitem_h
#define uiodpseventstreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          November 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"
#include "multiid.h"

class uiODPSEventsTreeItem;
namespace PreStack { class EventManager; }
namespace visSurvey { class PSEventDisplay; }

mClass(uiODMain) uiODPSEventsParentTreeItem : public uiODTreeItem
{
public:
				uiODPSEventsParentTreeItem();
				~uiODPSEventsParentTreeItem();

    int				sceneID() const;

protected:
    bool			init();
    const char*			parentType() const;
    virtual bool		showSubMenu();
    bool			loadPSEvent(MultiID&,BufferString&);
    uiODPSEventsTreeItem*	child_;
};


mClass(uiODMain) uiODPSEventsTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const   { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPSEventsParentTreeItem;}
    uiTreeItem*		create(int visid,uiTreeItem*) const 
			{ return new uiODPSEventsParentTreeItem; }
};


mClass(uiODMain) uiODPSEventsTreeItem : public uiODDisplayTreeItem
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

#endif
