#ifndef uiodhortreeitem_h
#define uiodhortreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodhortreeitem.h,v 1.16 2011-02-24 14:57:09 cvsbert Exp $
________________________________________________________________________


-*/

#include "uiodemsurftreeitem.h"

class uEMHorizonShiftDialog;
mDefineItem( HorizonParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton \
	     void sort(); bool addChild(uiTreeItem*,bool below,bool) );


mClass uiODHorizonTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODHorizonParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODHorizonTreeItem : public uiODEarthModelSurfaceTreeItem
{
public:
    			uiODHorizonTreeItem( int, bool rgba, bool dummy );
    			uiODHorizonTreeItem( const EM::ObjectID& emid,
					     bool rgba );

protected:
    bool		init();
    void		initMenuItems();
    void                initNotify();
    BufferString	createDisplayName() const;
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizonParentTreeItem).name(); }

    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded(bool withcancel);

    uEMHorizonShiftDialog* horshiftdlg_;
    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filterhormnuitem_;
    MenuItem		geom2attrmnuitem_;
    MenuItem		positionmnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		shiftmnuitem_;
    MenuItem		removeselectionmnuitem_;
    bool		rgba_;
};


mDefineItem( Horizon2DParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton \
	     void sort(); bool addChild(uiTreeItem*,bool below,bool) );


mClass uiODHorizon2DTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODHorizon2DParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODHorizon2DTreeItem : public uiODEarthModelSurfaceTreeItem
{
public:
    			uiODHorizon2DTreeItem( int, bool dummy );
    			uiODHorizon2DTreeItem( const EM::ObjectID& emid );

protected:
    void		initMenuItems();
    void                initNotify();
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizon2DParentTreeItem).name(); }

    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);
    
    bool		askContinueAndSaveIfNeeded(bool withcancel);

    MenuItem		derive3dhormnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		interploatemnuitem_;
};


#endif
