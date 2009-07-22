#ifndef uiodplanedatatreeitem_h
#define uiodplanedatatreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodplanedatatreeitem.h,v 1.11 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

class uiSliceSelDlg;
class CubeSampling;


mClass uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{
public:
    enum Type		{ Default, Empty, RGBA };
    enum Orientation	{ Inline, Crossline, ZSlice };

    			uiODPlaneDataTreeItem(int displayid,Orientation,Type);
			~uiODPlaneDataTreeItem();
    bool		init();

protected:
    BufferString	createDisplayName() const;

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		posDlgClosed(CallBacker*);
    void		keyPressCB(CallBacker*);
    void		movePlane(bool forward);

    void		selChg(CallBacker*);
    void		posChange(CallBacker*);
    void		movePlaneAndCalcAttribs(const CubeSampling&);

    const Orientation	orient_;
    const Type		type_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    uiSliceSelDlg*	positiondlg_;
};



mDefineItem( InlineParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass uiODInlineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODInlineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODInlineTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODInlineTreeItem(int displayid,Type);

protected:
    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


mDefineItem( CrosslineParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass uiODCrosslineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODCrosslineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODCrosslineTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODCrosslineTreeItem(int displayid,Type);

protected:
    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};


mDefineItem( TimesliceParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass uiODTimesliceTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODTimesliceParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODTimesliceTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODTimesliceTreeItem(int displayid,Type);

protected:
    const char*		parentType() const
			{ return typeid(uiODTimesliceParentTreeItem).name(); }
};


#endif
