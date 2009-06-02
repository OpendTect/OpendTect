#ifndef uiodhortreeitem_h
#define uiodhortreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodhortreeitem.h,v 1.9 2009-06-02 13:32:30 cvsnanne Exp $
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
    			uiODHorizonTreeItem( int, bool dummy );
    			uiODHorizonTreeItem( const EM::ObjectID& emid );

protected:
    void		initMenuItems();
    void                initNotify();
    BufferString	createDisplayName() const;
    void		dispChangeCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODHorizonParentTreeItem).name(); }

    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);

    bool		askContinueAndSaveIfNeeded();

    uEMHorizonShiftDialog* horshiftdlg_;
    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filterhormnuitem_;
    MenuItem		snapeventmnuitem_;
    MenuItem		shiftmnuitem_;
    MenuItem		removeselectionmnuitem_;
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
    
    bool		askContinueAndSaveIfNeeded();

    MenuItem		derive3dhormnuitem_;
};


#endif
