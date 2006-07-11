#ifndef uiodplanedatatreeitem_h
#define uiodplanedatatreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodplanedatatreeitem.h,v 1.2 2006-07-11 07:35:20 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"
class uiSliceSel;
class CubeSampling;


class uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODPlaneDataTreeItem( int displayid, int dim );
			~uiODPlaneDataTreeItem();
    bool		init();

protected:
    BufferString	createDisplayName() const;
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		movePlane(const CubeSampling&);
    void		posDlgClosed(CallBacker*);
    void		moveForwdCB(CallBacker*);
    void		moveBackwdCB(CallBacker*);
    void		movePlane(bool forward);

    int			dim_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    uiSliceSel*		positiondlg_;
};



mDefineItem( InlineParent, TreeItem, TreeTop, mShowMenu );


class uiODInlineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODInlineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODInlineTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODInlineTreeItem( int displayid );

protected:
    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


mDefineItem( CrosslineParent, TreeItem, TreeTop, mShowMenu );


class uiODCrosslineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODCrosslineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODCrosslineTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODCrosslineTreeItem( int displayid );

protected:
    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};


mDefineItem( TimesliceParent, TreeItem, TreeTop, mShowMenu );


class uiODTimesliceTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODTimesliceParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODTimesliceTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODTimesliceTreeItem( int displayid );

protected:
    const char*		parentType() const
			{ return typeid(uiODTimesliceParentTreeItem).name(); }
};


#endif
