#ifndef uiodhortreeitem_h
#define uiodhortreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodhortreeitem.h,v 1.3 2008-07-01 10:04:35 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uiodemsurftreeitem.h"


mDefineItem( HorizonParent, TreeItem, TreeTop, mShowMenu );


class uiODHorizonTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODHorizonParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODHorizonTreeItem : public uiODEarthModelSurfaceTreeItem
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

    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filterhormnuitem_;
    MenuItem		snapeventmnuitem_;
};


mDefineItem( Horizon2DParent, TreeItem, TreeTop, mShowMenu );


class uiODHorizon2DTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODHorizon2DParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODHorizon2DTreeItem : public uiODEarthModelSurfaceTreeItem
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
    
    MenuItem		derive3dhormnuitem_;
};


#endif
