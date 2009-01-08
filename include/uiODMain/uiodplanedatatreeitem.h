#ifndef uiodplanedatatreeitem_h
#define uiodplanedatatreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodplanedatatreeitem.h,v 1.9 2009-01-08 10:47:25 cvsranojay Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

class uiSliceSelDlg;
class CubeSampling;


mClass uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODPlaneDataTreeItem(int displayid,int dim,
					      bool rgba);
			~uiODPlaneDataTreeItem();
    bool		init();

protected:
    //uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;
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

    const int		dim_;
    const bool		rgba_;
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
    			uiODInlineTreeItem( int displayid, bool rgba );

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
    			uiODCrosslineTreeItem( int displayid, bool rgba );

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
    			uiODTimesliceTreeItem( int displayid, bool rgba );

protected:
    const char*		parentType() const
			{ return typeid(uiODTimesliceParentTreeItem).name(); }
};


#endif
