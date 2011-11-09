#ifndef pseventstreeitem_h
#define pseventstreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          November 2011
 RCS:           $Id: uipseventstreeitem.h,v 1.2 2011-11-09 04:42:23 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"


class PSEventsTreeItem;
namespace PreStack{ class EventManager; }
namespace visSurvey{ class visSeedPolyLine; }

class PSEventsParentTreeItem : public uiODTreeItem
{
public:
			PSEventsParentTreeItem();
			~PSEventsParentTreeItem();

    int			sceneID() const;

protected:
    bool		init();
    const char*		parentType() const;
    virtual bool	showSubMenu();
    bool		loadPSEvent(BufferString&);
    RefMan<PreStack::EventManager> psem_;
    PSEventsTreeItem*		child_;
};


class PSEventsTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const   { return getName(); }
    static const char*	getName()
			    { return typeid(PSEventsTreeItemFactory).name();}
    uiTreeItem*		create() const { return new PSEventsParentTreeItem; }
    uiTreeItem*		create(int,uiTreeItem*) const 
			{ return new PSEventsParentTreeItem; }
};


class PSEventsTreeItem : public uiODDisplayTreeItem
{
public:
			    PSEventsTreeItem(const PreStack::EventManager&,
					      const char*);
			    ~PSEventsTreeItem();
protected:
    virtual const char*	    parentType() const 
			    { return typeid (PSEventsParentTreeItem).name(); }
    virtual const char*	    managerName() const { return "PreStackEvents"; }
    virtual void	    createMenuCB(CallBacker*);
    virtual void	    handleMenuCB(CallBacker*);
    bool		    init();
    void		    updateDisplay();

    const PreStack::EventManager& psem_;
    const char*		    eventname_;
    visSurvey::visSeedPolyLine*  eventlinedisplay_;
    MenuItem*		    sticksfromsection_;
    MenuItem*		    zerooffset_;
    MenuItem*		    properties_;
};

#endif
