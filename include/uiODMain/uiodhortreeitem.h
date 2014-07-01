#ifndef uiodhortreeitem_h
#define uiodhortreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodemsurftreeitem.h"

class uEMHorizonShiftDialog;

mDefineItem( HorizonParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton \
             void sort(); virtual bool addChld(uiTreeItem*,bool,bool);
             virtual void removeChild(uiTreeItem*) );


mExpClass(uiODMain) uiODHorizonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODHorizonTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODHorizonParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODHorizonTreeItem : public uiODEarthModelSurfaceTreeItem
{
public:
			uiODHorizonTreeItem(int visid,bool rgba,
					    bool atsect,bool dummy);
			uiODHorizonTreeItem(const EM::ObjectID&,
					    bool rgba,bool atsect);

protected:
    bool		init();
    void		initMenuItems();
    void                initNotify();
    BufferString	createDisplayName() const;
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizonParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

    uEMHorizonShiftDialog* horshiftdlg_;
    MenuItem		hordatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filterhormnuitem_;
    MenuItem		geom2attrmnuitem_;
    MenuItem		positionmnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		shiftmnuitem_;
    bool		rgba_;
    bool		atsections_;
};


mDefineItem( Horizon2DParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton \
             void sort(); virtual bool addChld(uiTreeItem*,bool,bool);
             virtual void removeChild(uiTreeItem*) );


mExpClass(uiODMain) uiODHorizon2DTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODHorizon2DParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODHorizon2DTreeItem : public uiODEarthModelSurfaceTreeItem
{
public:
			uiODHorizon2DTreeItem(int visid,bool dummy);
			uiODHorizon2DTreeItem(const EM::ObjectID&);

protected:
    void		initMenuItems();
    void                initNotify();
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizon2DParentTreeItem).name(); }

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

    MenuItem		algomnuitem_;
    MenuItem		workflowsmnuitem_;
    MenuItem		derive3dhormnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		interpolatemnuitem_;
};

#endif
