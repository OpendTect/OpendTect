#ifndef pseventstreeitem_h
#define pseventstreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          November 2011
 RCS:           $Id: uipseventstreeitem.h,v 1.4 2011-11-16 04:55:51 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uioddisplaytreeitem.h"


class PSEventsTreeItem;
namespace PreStack{ class EventManager; }
namespace visSurvey{ class PSEventDisplay; }

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
    bool		loadPSEvent(MultiID&,BufferString&);
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
			    PSEventsTreeItem( MultiID key,
					      const char*);
			    ~PSEventsTreeItem();
    void		    updateScaleFactor(float);
    void		    updateColorMode(int mode);

protected:
    virtual const char*	    parentType() const 
			    { return typeid (PSEventsParentTreeItem).name(); }
    virtual const char*	    managerName() const { return "PreStackEvents"; }
    virtual void	    createMenuCB(CallBacker*);
    virtual void	    handleMenuCB(CallBacker*);
    bool		    init();
    void		    updateDisplay();

    PreStack::EventManager& psem_;
    const char*		    eventname_;
    float		    scalefactor_;
    Coord		    dir_;
    visSurvey::PSEventDisplay* eventdisplay_;
    MenuItem*		    zerooffset_;
    MenuItem*		    sticksfromsection_;
    MenuItem*		    zerooffsetonsection_;
    MenuItem*		    properties_;
    const MultiID&	    key_;
};

#endif
